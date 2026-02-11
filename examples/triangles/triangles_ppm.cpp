#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <cascade/fragment_ops.h>
#include <cascade/rasterizer.h>

constexpr uint32_t WIDTH = 640;
constexpr uint32_t HEIGHT = 480;

using namespace cascade;

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <output_file>\n";
        return 1;
    }

    const size_t framebuffer_size = WIDTH * HEIGHT * sizeof(uint32_t);
    uint32_t* framebuffer = reinterpret_cast<uint32_t*>(std::malloc(framebuffer_size));

    if (!framebuffer) {
        std::cerr << "framebuffer allocation failed\n";
        abort();
    }
    std::memset(framebuffer, 0, framebuffer_size);

    float vertex_data[] = {50.0f,  200.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // vertex 0 (50, 200, 1, 1) RED
                           100.0f, 200.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // vertex 1 (100, 200, 1, 1) BLUE
                           75.0f,  100.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // vertex 2 (75, 100, 1, 1) GREEN
                           300.0f, 300.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // vertex 3 (300, 300, 1, 1) RED
                           300.0f, 100.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // vertex 4 (300, 100, 1, 1) RED
                           500.0f, 200.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f}; // vertex 5 (500, 200, 1, 1) BLUE

    uint32_t indices[] = {0, 2, 1, 3, 4, 5}; // Make sure the vertices are in clockwise winding order
    uint32_t index_count = 6;
    uint32_t stride_bytes = 8 * sizeof(float);
    ViewportBounds bounds = {{0, 0}, {WIDTH - 1, HEIGHT - 1}};
    RasterizerInput input = {vertex_data, indices, index_count, stride_bytes, bounds};

    constexpr uint32_t FRAG_BUF_SIZE = 1024;
    void* fragment_buffer = std::malloc(FRAG_BUF_SIZE);
    if (!fragment_buffer) {
        std::cerr << "fragment buffer allocation failed\n";
        abort();
    }

    uint32_t fragment_stride_bytes = 2 * sizeof(uint32_t) + 5 * sizeof(float);
    OutputContext context = {framebuffer, fragment_stride_bytes, WIDTH};
    FragmentBufferInfo fbi = {fragment_buffer, FRAG_BUF_SIZE, processFragmentsWithoutDepth, &context};
    rasterize(input, fbi);

    FILE* file = std::fopen(argv[1], "wb");
    std::fprintf(file, "P6\n%u %u\n255\n", WIDTH, HEIGHT);

    for (int i = 0; i < WIDTH * HEIGHT; ++i) {
        uint32_t pixel = framebuffer[i];
        uint8_t r = static_cast<uint8_t>((pixel >> 16) & 0xFF);
        uint8_t g = static_cast<uint8_t>((pixel >> 8) & 0xFF);
        uint8_t b = static_cast<uint8_t>(pixel & 0xFF);
        uint8_t channels[] = {r, g, b};
        std::fwrite(channels, 1, 3, file);
    }

    std::fclose(file);
    std::free(fragment_buffer);
    std::free(framebuffer);
}