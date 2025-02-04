cmake_minimum_required(VERSION 3.20)

project(compute_shaders_project CXX)
set(CXX_STANDARD 20)

find_package(OpenGL REQUIRED)
# find_package(SDL2 REQUIRED)

add_subdirectory(src)
add_subdirectory(extern)

# Definir SDL3 como una biblioteca est�tica
set(SDL_SHARED OFF CACHE BOOL "" FORCE)   # Desactiva la versi�n din�mica
set(SDL_STATIC ON CACHE BOOL "" FORCE)    # Activa la versi�n est�tica
add_subdirectory(extern/SDL)


add_executable(main "main.cpp")
add_executable(test2 "test2.cpp")
add_executable(test3 "test3.cpp")
add_executable(test4 "test4.cpp")
add_executable(test5 "test5.cpp")
add_executable(test6 "test6_camera.cpp")
add_executable(test7 "test7_secuencial.cpp")

target_link_libraries(CS_dependencies PUBLIC  glad glm)

target_link_libraries(main PUBLIC CS_dependencies SDL3-static glad)
target_link_libraries(test2 PUBLIC CS_dependencies SDL3-static glad)
target_link_libraries(test3 PUBLIC CS_dependencies SDL3-static glad)
target_link_libraries(test4 PUBLIC CS_dependencies SDL3-static glad)
target_link_libraries(test5 PUBLIC CS_dependencies SDL3-static glad)
target_link_libraries(test6 PUBLIC CS_dependencies SDL3-static glad)
target_link_libraries(test7 PUBLIC CS_dependencies SDL3-static glad)

target_include_directories(main PUBLIC ${OPENGL_INCLUDE_DIRS} extern/SDL/include)
target_include_directories(test2 PUBLIC ${OPENGL_INCLUDE_DIRS} extern/SDL/include)
target_include_directories(test3 PUBLIC ${OPENGL_INCLUDE_DIRS} extern/SDL/include)
target_include_directories(test4 PUBLIC ${OPENGL_INCLUDE_DIRS} extern/SDL/include)
target_include_directories(test5 PUBLIC ${OPENGL_INCLUDE_DIRS} extern/SDL/include)
target_include_directories(test6 PUBLIC ${OPENGL_INCLUDE_DIRS} extern/SDL/include)
target_include_directories(test7 PUBLIC ${OPENGL_INCLUDE_DIRS} extern/SDL/include)

set(SHADER_FILES
    ${CMAKE_SOURCE_DIR}/computeSh_test6.cs
    ${CMAKE_SOURCE_DIR}/computeShader.cs
    ${CMAKE_SOURCE_DIR}/computeShader2.cs
    ${CMAKE_SOURCE_DIR}/screenQuad.fs
    ${CMAKE_SOURCE_DIR}/screenQuad.vs
    
)

# Copia los archivos .cs al directorio de salida del ejecutable
add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${SHADER_FILES}
    $<TARGET_FILE_DIR:main>
)
