#ifndef CASCADE_PTR_UTILS_H_
#define CASCADE_PTR_UTILS_H_

#include <cstdint>

namespace cascade {

inline char* char_ptr(void* ptr) {
    return reinterpret_cast<char*>(ptr);
}

inline const char* char_ptr(const void* ptr) {
    return reinterpret_cast<const char*>(ptr);
}

inline uint8_t* uint8_ptr(void* ptr) {
    return reinterpret_cast<uint8_t*>(ptr);
}

inline const uint8_t* uint8_ptr(const void* ptr) {
    return reinterpret_cast<const uint8_t*>(ptr);
}

inline uint32_t* uint32_ptr(void* ptr) {
    return reinterpret_cast<uint32_t*>(ptr);
}

inline const uint32_t* uint32_ptr(const void* ptr) {
    return reinterpret_cast<const uint32_t*>(ptr);
}

inline float* float_ptr(void* ptr) {
    return reinterpret_cast<float*>(ptr);
}

inline const float* float_ptr(const void* ptr) {
    return reinterpret_cast<const float*>(ptr);
}

} // namespace cascade

#endif