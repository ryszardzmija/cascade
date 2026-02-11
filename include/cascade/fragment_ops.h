#ifndef CASCADE_FRAGMENT_OPS_H_
#define CASCADE_FRAGMENT_OPS_H_

#include <cstdint>

namespace cascade {

struct OutputContext {
    void* color_buffer;
    uint32_t fragment_stride;
    uint32_t width;
};

void processFragmentsWithoutDepth(const void* frag_buf, uint32_t used_bytes, const void* output_context);

} // namespace cascade

#endif