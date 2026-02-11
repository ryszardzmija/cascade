#ifndef CASCADE_RASTERIZER_H_
#define CASCADE_RASTERIZER_H_

#include <cstdint>

#include <cascade/common/vec2.h>

namespace cascade {

struct ViewportBounds {
    Vec2<int32_t> top_left;
    Vec2<int32_t> bottom_right;
};

struct RasterizerInput {
    const float* vertex_data;
    const uint32_t* indices;
    uint32_t index_count;
    uint32_t stride_bytes;
    ViewportBounds bounds;
};

struct FragmentBufferInfo {
    void* buffer;
    uint32_t size_bytes;
    void (*flush)(const void* buffer, uint32_t used_bytes, const void* context);
    void* context;
};

void rasterize(const RasterizerInput& input, const FragmentBufferInfo& fbi);

} // namespace cascade

#endif