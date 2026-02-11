if (WIN32)
    add_executable(triangles_gdi)
    target_sources(triangles_gdi PRIVATE
        triangles_gdi.cpp
    )
    target_link_libraries(triangles_gdi cascade)
endif()

add_executable(triangles_ppm)
target_sources(triangles_ppm PRIVATE
    triangles_ppm.cpp
)
target_link_libraries(triangles_ppm cascade)
