#include "fragment_ops.h"

#include <cstdint>

#include "ptr_utils.h"

constexpr uint32_t FRAGMENT_COORD_SIZE = 2 * sizeof(uint32_t) + sizeof(float);

inline static uint8_t quantizeColor(float color) {
	if (color < 0.0f) {
		return 0;
	}
	if (color > 1.0f) {
		return 255;
	}
	return static_cast<uint8_t>(color * 255.0f);
}

void processFragmentsWithoutDepth(const void* frag_buf, uint32_t used_bytes, const void* output_context) {
	const OutputContext* context = static_cast<const OutputContext*>(output_context);
	void* color_buf = context->color_buffer;
	uint32_t fragment_stride = context->fragment_stride;
	uint32_t width = context->width;
	
	// CHECK: Will overflow if the memory used is over 4GB
	for (uint32_t i = 0; i < used_bytes; i += fragment_stride) {
		const uint32_t* coord_ptr = uint32_ptr(char_ptr(frag_buf) + i);
		uint32_t x = coord_ptr[0];
		uint32_t y = coord_ptr[1];

		const float* color_ptr = float_ptr(char_ptr(frag_buf) + i + FRAGMENT_COORD_SIZE);
		float frag_r = color_ptr[0];
		float frag_g = color_ptr[1];
		float frag_b = color_ptr[2];
		float frag_a = color_ptr[3];

		uint8_t r = quantizeColor(frag_r);
		uint8_t g = quantizeColor(frag_g);
		uint8_t b = quantizeColor(frag_b);
		uint8_t a = quantizeColor(frag_a);

		// The memory layout that is used for the colors is BGRA
		// Since we assume a little-endian machine this means that b must be the least
		// significant byte
		uint32_t* pixel_ptr = uint32_ptr(color_buf) + y * width + x;
		*pixel_ptr = (a << 24) | (r << 16) | (g << 8) | b;
	}
}