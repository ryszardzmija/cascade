#ifndef FRAGMENT_OPS_H_
#define FRAGMENT_OPS_H_

#include <cstdint>

struct OutputContext {
    void* color_buffer;
    uint32_t fragment_stride;
    uint32_t width;
};

void processFragmentsWithoutDepth(const void* frag_buf, uint32_t used_bytes, const void* output_context);

#endif