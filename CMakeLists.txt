cmake_minimum_required(VERSION 3.20)

project(compute_shaders_project CXX)
set(CXX_STANDARD 20)

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)

add_subdirectory(src)
add_subdirectory(extern)


add_executable(main "main.cpp")
add_executable(test2 "test2.cpp")
add_executable(test3 "test3.cpp")

target_link_libraries(CS_dependencies PUBLIC  glad glm)

target_link_libraries(main PUBLIC CS_dependencies SDL2 glad)
target_link_libraries(test2 PUBLIC CS_dependencies SDL2main SDL2 glad)
target_link_libraries(test3 PUBLIC CS_dependencies SDL2main SDL2 glad)

target_include_directories(main PUBLIC ${OPENGL_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})
target_include_directories(test2 PUBLIC ${OPENGL_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})
target_include_directories(test3 PUBLIC ${OPENGL_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})