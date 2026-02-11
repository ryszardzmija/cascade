# Cascade

A 3D graphics software renderer implementing the graphics pipeline from first principles.

## About

Cascade is a CPU-based rasterization renderer built from the ground up without dependencies on existing rendering libraries or GPU APIs. The goal is a clean, understandable implementation of the complete graphics pipeline.

Graphics APIs (Vulkan, Direct3D, OpenGL) abstract away implementation details to support diverse hardware, enable device-specific optimizations, and maintain stable interfaces. While essential for GPU-accelerated rendering, this means the actual algorithms - rasterization, interpolation, clipping - remain hidden in hardware and proprietary drivers. Cascade makes these algorithms explicit by implementing them in software. It provides clear, readable implementations of the classic (but still relevant!) algorithms used in modern rendering, from triangle rasterization to perspective-correct interpolation.

Understanding how these work in a CPU implementation - where each step can be examined and debugged - provides a strong foundation for understanding how GPUs execute the same operations in parallel across thousands of cores simultaneously.

## Current Status

**Implemented:**

- Triangle rasterization (Pineda's algorithm)
- Perspective-correct attribute interpolation
- Fragment processing with color output

**Planned**:

- Depth buffering
- Vertex transformation pipeline
- View frustum clipping
- Texturing

## Building

Requires CMake 3.20+ and a C++20 compiler.

```sh
cmake -B build
cmake --build build
```

## Running the example

```sh
./build/examples/triangles/triangles_ppm output.ppm
```

Writes the rendered triangles into a .ppm file viewable in any image editor.

## Examples

### Triangle Rasterization

![TriangleDemo](https://github.com/user-attachments/assets/6b5de05c-d36b-4606-b252-11462de97d28)

*Two triangles with perspective-correct color interpolation*
