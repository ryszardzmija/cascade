#ifndef RASTERIZER_H_
#define RASTERIZER_H_

#include <cstdint>

#include "vec2.h"

struct ViewportBounds {
	Vec2<int32_t> top_left;
	Vec2<int32_t> bottom_right;
};

struct RasterizerInput {
	float* vertex_data;
	uint32_t* indices;
	uint32_t index_count;
	uint32_t stride_bytes;
	ViewportBounds bounds;
};

struct FragmentBufferInfo {
	void* buffer;
	uint32_t size_bytes;
	void (*flush)(void* buffer, uint32_t used_bytes);
};

void rasterize(const RasterizerInput& input, const FragmentBufferInfo& fbi);

#endif