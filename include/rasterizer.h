#ifndef RASTERIZER_H_
#define RASTERIZER_H_

#include <cstdint>

struct Vec2 {
	float x;
	float y;
};

struct Triangle {
	Vec2 v0;
	Vec2 v1;
	Vec2 v2;
};

struct FramebufferInfo {
	uint32_t* buf;
	int width;
	int height;
};

void rasterize(const FramebufferInfo& buffer_info, const Triangle& triangle);

#endif