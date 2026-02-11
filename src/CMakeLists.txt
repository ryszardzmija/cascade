add_library(cascade STATIC)

target_include_directories(cascade
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_subdirectory(rasterizer)
add_subdirectory(fragment_ops)
