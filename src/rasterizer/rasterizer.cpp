#include <cascade/rasterizer.h>

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <cascade/common/vec2.h>

#include "detail/ptr_utils.h"

constexpr uint32_t VERTEX_COORD_SIZE = 4 * sizeof(float);                      // (x, y, z, w)
constexpr uint32_t FRAGMENT_COORD_SIZE = 2 * sizeof(uint32_t) + sizeof(float); // (x, y, z)
constexpr float DEGENERATE_EPS = 1e-5f;

struct BoundingBox {
    Vec2<float> top_left;
    Vec2<float> bottom_right;
};

inline static float min3(float a, float b, float c) {
    float result = a;
    if (b < result) {
        result = b;
    }
    if (c < result) {
        result = c;
    }
    return result;
}

inline static float max3(float a, float b, float c) {
    float result = a;
    if (b > result) {
        result = b;
    }
    if (c > result) {
        result = c;
    }
    return result;
}

inline static float min2(float a, float b) {
    return a < b ? a : b;
}

inline static float max2(float a, float b) {
    return a > b ? a : b;
}

inline static int floor(float x) {
    int tmp = static_cast<int>(x);
    return tmp - (x < tmp);
}

inline static int ceil(float x) {
    int tmp = static_cast<int>(x);
    return tmp + (x > tmp);
}

// Computes the signed area of the parallelogram formed by the edge vector and
// position vector with respect to the origin. It is the third component of the
// result of (x-X, y-Y) x (dX, dY) = (0, 0, (y-Y)dX - (x-X)dY) and has the
// property that it encodes the orientation of the two vectors with respect to
// each other.
inline static float edge_function(Vec2<float> origin, Vec2<float> point, float dX, float dY) {
    return (point.x - origin.x) * dY - (point.y - origin.y) * dX;
}

static BoundingBox findBoundingBox(Vec2<float> v0, Vec2<float> v1, Vec2<float> v2) {
    return {{min3(v0.x, v1.x, v2.x), min3(v0.y, v1.y, v2.y)}, {max3(v0.x, v1.x, v2.x), max3(v0.y, v1.y, v2.y)}};
}

static void clipBoundingBox(BoundingBox& bb, const ViewportBounds& vb) {
    bb.top_left.x = max2(bb.top_left.x, static_cast<float>(vb.top_left.x));
    bb.top_left.y = max2(bb.top_left.y, static_cast<float>(vb.top_left.y));
    bb.bottom_right.x = min2(bb.bottom_right.x, static_cast<float>(vb.bottom_right.x));
    bb.bottom_right.y = min2(bb.bottom_right.y, static_cast<float>(vb.bottom_right.y));
}

// The assumed convention is that vertices are provided with counterclockwise
// winding in object space with respect to the front face. This means that for
// the screen space with Y increasing downward the front-facing triangles will
// have clockwise winding after projection. This is the convention that DirectX
// uses.
void rasterize(const RasterizerInput& input, const FragmentBufferInfo& fbi) {
    const uint32_t index_count = input.index_count;
    const uint32_t* indices = input.indices;
    const uint32_t stride = input.stride_bytes;
    const uint32_t attribute_size = input.stride_bytes - VERTEX_COORD_SIZE;
    const uint32_t num_attributes = attribute_size / sizeof(float);
    const uint32_t fragment_stride = FRAGMENT_COORD_SIZE + attribute_size;
    const uint32_t fragment_buffer_size = fbi.size_bytes;
    const float* vertex_data = input.vertex_data;
    const void* output_context = fbi.context;

    uint32_t used_bytes = 0;
    void* frag_buf = fbi.buffer;
    void (*flush)(const void*, uint32_t, const void*) = fbi.flush;

    // For storing precomputed values for perspective-correct
    // interpolation
    float* A_over_w = static_cast<float*>(std::malloc(static_cast<size_t>(3) * num_attributes * sizeof(float)));
    assert(A_over_w != nullptr);

    for (uint32_t i = 0; i < index_count; i += 3) {
        // Get indices
        uint32_t v0_ind = indices[i];
        uint32_t v1_ind = indices[i + 1];
        uint32_t v2_ind = indices[i + 2];

        const float* v0_ptr = float_ptr(char_ptr(vertex_data) + stride * v0_ind);
        const float* v1_ptr = float_ptr(char_ptr(vertex_data) + stride * v1_ind);
        const float* v2_ptr = float_ptr(char_ptr(vertex_data) + stride * v2_ind);

        // Get vertex coordinates in screen space
        float v0_x = v0_ptr[0];
        float v0_y = v0_ptr[1];

        float v1_x = v1_ptr[0];
        float v1_y = v1_ptr[1];

        float v2_x = v2_ptr[0];
        float v2_y = v2_ptr[1];

        // Get z and w components in clip space
        float v0_z = v0_ptr[2];
        float v0_w = v0_ptr[3];

        float v1_z = v1_ptr[2];
        float v1_w = v1_ptr[3];

        float v2_z = v2_ptr[2];
        float v2_w = v2_ptr[3];

        // Find and clip the bounding box to the viewport
        BoundingBox bb = findBoundingBox({v0_x, v0_y}, {v1_x, v1_y}, {v2_x, v2_y});
        clipBoundingBox(bb, input.bounds);

        // Find pixel boundaries of the viewport
        int i_init = floor(bb.top_left.x);
        int j_init = floor(bb.top_left.y);
        int max_i = ceil(bb.bottom_right.x);
        int max_j = ceil(bb.bottom_right.y);

        // Set up state variables for Pineda's algorithm
        float dX[3];
        float dY[3];
        dX[0] = v1_x - v0_x;
        dX[1] = v2_x - v1_x;
        dX[2] = v0_x - v2_x;
        dY[0] = v1_y - v0_y;
        dY[1] = v2_y - v1_y;
        dY[2] = v0_y - v2_y;
        float e_j[3]; // Contains E(x, y) at the left-most column for the current row

        // Addition of vector (0.5, 0.5) makes sure that the center of pixels will
        // be tested
        Vec2<float> point = {static_cast<float>(i_init) + 0.5f, static_cast<float>(j_init) + 0.5f};
        e_j[0] = edge_function({v0_x, v0_y}, point, dX[0], dY[0]);
        e_j[1] = edge_function({v1_x, v1_y}, point, dX[1], dY[1]);
        e_j[2] = edge_function({v2_x, v2_y}, point, dX[2], dY[2]);

        // Twice the signed area of the triangle. Needed for the computation
        // of barycentric coordinates for interpolation
        float area2 = e_j[0] + e_j[1] + e_j[2];

        // If the triangle is degenerate skip it to avoid Inf for inv_area2
        if (std::abs(area2) < DEGENERATE_EPS) {
            continue;
        }

        float inv_area2 = 1.0f / area2; // Precompute the factor for efficiency

        // Precompute values for perspective-correct divison for the triangle
        // for efficiency
        float inv_w0 = 1 / v0_w;
        float inv_w1 = 1 / v1_w;
        float inv_w2 = 1 / v2_w;

        // Depth ratio precomputation
        float v0_z_over_w = v0_z * inv_w0;
        float v1_z_over_w = v1_z * inv_w1;
        float v2_z_over_w = v2_z * inv_w2;

        // Ratio precomputation for the rest of the attributes
        const float* v0_attrib_ptr = float_ptr(char_ptr(vertex_data) + stride * v0_ind + VERTEX_COORD_SIZE);
        const float* v1_attrib_ptr = float_ptr(char_ptr(vertex_data) + stride * v1_ind + VERTEX_COORD_SIZE);
        const float* v2_attrib_ptr = float_ptr(char_ptr(vertex_data) + stride * v2_ind + VERTEX_COORD_SIZE);
        for (uint32_t attrib = 0; attrib < num_attributes; ++attrib) {
            // Get attribute values at each vertex
            float v0_attrib = v0_attrib_ptr[attrib];
            float v1_attrib = v1_attrib_ptr[attrib];
            float v2_attrib = v2_attrib_ptr[attrib];

            // Precompute ratio for each vertex
            A_over_w[3 * attrib] = v0_attrib * inv_w0;
            A_over_w[3 * attrib + 1] = v1_attrib * inv_w1;
            A_over_w[3 * attrib + 2] = v2_attrib * inv_w2;
        }

        for (int j = j_init; j <= max_j; ++j) {
            float e_i[3]; // Contains E(x, y) at the current row and column
            std::memcpy(e_i, e_j,
                        sizeof(e_j)); // Initialize values for the left-most column

            for (int i = i_init; i <= max_i; ++i) {
                // Testing whether pixel is inside the triangle
                // TODO: Make sure to verify the edge case where triangles have a shared
                // edge
                if (e_i[0] <= 0 && e_i[1] <= 0 && e_i[2] <= 0) {
                    // Test if the fragment buffer must be flushed to make space
                    if (used_bytes + fragment_stride > fragment_buffer_size) {
                        flush(frag_buf, used_bytes, output_context);
                        used_bytes = 0;
                    }
                    // Write pixel coordinates for the fragment
                    *uint32_ptr(char_ptr(frag_buf) + used_bytes) = i;
                    *uint32_ptr(char_ptr(frag_buf) + used_bytes + sizeof(uint32_t)) = j;

                    // Compute screen-space barycentric coordinates for the center of the
                    // fragment
                    float lambda0 = e_i[1] * inv_area2; // Coordinate for v0 computed
                                                        // based on the edge v1 -> v2
                    float lambda1 = e_i[2] * inv_area2; // Coordinate for v1 computed
                                                        // based on the edge v2 -> v0
                    float lambda2 = e_i[0] * inv_area2; // Coordinate for v2 computed
                                                        // based on the edge v0 -> v1

                    // Precompute 1/w interpolation in screen-space for
                    // perspective-correct interpolation
                    float one_over_w_interp = lambda0 * inv_w0 + lambda1 * inv_w1 + lambda2 * inv_w2;
                    float inv_one_over_w_interp = 1.0f / one_over_w_interp;

                    // Compute and write perspective-correct depth
                    float z_over_w_interp = lambda0 * v0_z_over_w + lambda1 * v1_z_over_w + lambda2 * v2_z_over_w;
                    *float_ptr(char_ptr(frag_buf) + used_bytes + 2 * sizeof(uint32_t)) =
                        z_over_w_interp * inv_one_over_w_interp;

                    // Compute and write perspective-correct value for the rest of the
                    // attributes
                    float* attrib_ptr = float_ptr(char_ptr(frag_buf) + used_bytes + FRAGMENT_COORD_SIZE);

                    for (uint32_t attrib = 0; attrib < num_attributes; ++attrib) {
                        float A_over_w_interp = lambda0 * A_over_w[3 * attrib] + lambda1 * A_over_w[3 * attrib + 1] +
                                                lambda2 * A_over_w[3 * attrib + 2];
                        attrib_ptr[attrib] = A_over_w_interp * inv_one_over_w_interp;
                    }

                    used_bytes += fragment_stride;
                }

                e_i[0] += dY[0];
                e_i[1] += dY[1];
                e_i[2] += dY[2];
            }

            e_j[0] -= dX[0];
            e_j[1] -= dX[1];
            e_j[2] -= dX[2];
        }
    }
    flush(frag_buf, used_bytes, output_context);
    std::free(A_over_w);
}
