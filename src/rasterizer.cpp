#include "rasterizer.h"

#include <cstring>
#include <cstdint>

constexpr uint32_t RED = 0x00FF0000; // BGRA little-endian RED

struct BoundingBox {
	Vec2 top_left;
	Vec2 bottom_right;
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

// Computes the signed area of the parallelogram formed by the edge vector and position
// vector with respect to the origin. It is the third component of the result of
// (x-X, y-Y) x (dX, dY) = (0, 0, (y-Y)dX - (x-X)dY) and has the property that it
// encodes the orientation of the two vectors with respect to each other.
inline static float edge_function(Vec2 origin, Vec2 point, float dX, float dY) {
	return (point.x - origin.x) * dY - (point.y - origin.y) * dX;
}

static BoundingBox findBoundingBox(const Triangle& triangle) {
	return {
		{min3(triangle.v0.x, triangle.v1.x, triangle.v2.x), min3(triangle.v0.y, triangle.v1.y, triangle.v2.y)},
		{max3(triangle.v0.x, triangle.v1.x, triangle.v2.x), max3(triangle.v0.y, triangle.v1.y, triangle.v2.y)}
	};
}

static void clipBoundingBox(BoundingBox& bb, const FramebufferInfo& buffer_info) {
	bb.top_left.x = max2(bb.top_left.x, 0);
	bb.top_left.y = max2(bb.top_left.y, 0);
	bb.bottom_right.x = min2(bb.bottom_right.x, static_cast<float>(buffer_info.width - 1));
	bb.bottom_right.y = min2(bb.bottom_right.y, static_cast<float>(buffer_info.height - 1));
}

// The assumed convention is that vertices are provided with counterclockwise winding in object space
// with respect to the front face. This means that for the screen space with Y increasing downward the
// front-facing triangles will have clockwise winding after projection. This is the convention that
// DirectX uses.
void rasterize(const FramebufferInfo& buffer_info, const Triangle& triangle) {
	BoundingBox bb = findBoundingBox(triangle);
	clipBoundingBox(bb, buffer_info);

	uint32_t* framebuffer = buffer_info.buf;
	int fb_width = buffer_info.width;

	int i_init = floor(bb.top_left.x);
	int j_init = floor(bb.top_left.y);
	int max_i = ceil(bb.bottom_right.x);
	int max_j = ceil(bb.bottom_right.y);
	
	float dX[3];
	float dY[3];
	dX[0] = triangle.v1.x - triangle.v0.x;
	dX[1] = triangle.v2.x - triangle.v1.x;
	dX[2] = triangle.v0.x - triangle.v2.x;
	dY[0] = triangle.v1.y - triangle.v0.y;
	dY[1] = triangle.v2.y - triangle.v1.y;
	dY[2] = triangle.v0.y - triangle.v2.y;
	float e_j[3];  // Contains E(x, y) at the left-most column for the current row

	// Addition of vector (0.5, 0.5) makes sure that the center of pixels will be tested
	Vec2 point = { static_cast<float>(i_init) + 0.5f, static_cast<float>(j_init) + 0.5f};
	e_j[0] = edge_function(triangle.v0, point, dX[0], dY[0]);
	e_j[1] = edge_function(triangle.v1, point, dX[1], dY[1]);
	e_j[2] = edge_function(triangle.v2, point, dX[2], dY[2]);

	for (int j = j_init; j <= max_j; ++j) {
		float e_i[3];  // Contains E(x, y) at the current row and column
		std::memcpy(e_i, e_j, sizeof(e_j));  // Initialize values for the left-most column

		for (int i = i_init; i <= max_i; ++i) {
			// Testing whether pixel is inside the triangle
			if (e_i[0] <= 0 && e_i[1] <= 0 && e_i[2] <= 0) {
				// Fill the pixel using RED
				framebuffer[i + j * fb_width] = RED;
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
