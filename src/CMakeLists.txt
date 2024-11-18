add_library(CS_dependencies STATIC
    "shader_m.cpp"
    "shader_c.cpp"
    "camera3.cpp"
)

set_property(TARGET CS_dependencies PROPERTY CXX_STANDARD 20)
target_link_libraries(CS_dependencies PUBLIC ${SDL2_LIBRARIES} glad glm)
target_include_directories(CS_dependencies PUBLIC ${OPENGL_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} "${CMAKE_SOURCE_DIR}/include")
