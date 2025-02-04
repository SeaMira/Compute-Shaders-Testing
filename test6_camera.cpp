#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "camera3.h"
#include "shader_c.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>

// Configuración
const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 600;

double deltaTime = 0.0; // time between current frame and last frame
double lastFrame = 0.0;

Camera* global_cam;

// Datos de la esfera
struct Sphere {
    glm::vec3 position;
    float radius;
};

Sphere sphere = {glm::vec3(0.0f, 5.0f, 0.0f), 2.0f};

int main(int argv, char** args) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Error al inicializar SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Compute Shader + SDL3 Renderer",
                                          SCR_WIDTH, SCR_HEIGHT, SDL_WINDOW_OPENGL);
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);


    // Obtener el tiempo inicial
    uint64_t startTime = SDL_GetPerformanceCounter();
    uint64_t frequency = SDL_GetPerformanceFrequency();

    Camera camera(SCR_WIDTH, SCR_HEIGHT);

    global_cam = &camera;

    camera.SetPosition(3.0f, 0.0f, 0.0f);

    ComputeShader computeShader("computeSh_test6.cs");
    computeShader.use();

    // Crear textura para el Compute Shader
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    // Crear un framebuffer para renderizar
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    bool running = true;
    SDL_Event event;

    
    float lastX = SCR_WIDTH/2.0f;
    float lastY = SCR_HEIGHT/2.0f;

    bool show_grid = true;
    bool show_axes = true;
    while (running) {

        // static uint64_t frequency = SDL_GetPerformanceFrequency();
		uint64_t currentFrame = SDL_GetPerformanceCounter();
		deltaTime = static_cast<double>(currentFrame - lastFrame) / frequency;
		lastFrame = currentFrame;

        // std::cout << "FPS: " << 1 / deltaTime << std::endl;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_G) show_grid = !show_grid;
                if (event.key.key == SDLK_H) show_axes = !show_axes;
            }
            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                camera.OnMouse((float) event.motion.x, (float) event.motion.y);

                lastX = (float) event.motion.x;
                lastY = (float) event.motion.y;

            }
            if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                camera.OnScroll(event.wheel.y);
            }
        }
        //std::cout << camera.getYaw() << ", " << camera.getPitch() << std::endl; 
        
         // Obtener el estado de todas las teclas (mover la cámara si las teclas están presionadas)
        const bool* state = SDL_GetKeyboardState(NULL);
        
        // Usa las teclas para mover la cámara si están presionadas
        if (state[SDL_SCANCODE_W]) {
            camera.OnKeyboard('w', deltaTime);  // Movimiento hacia adelante
        }
        if (state[SDL_SCANCODE_S]) {
            camera.OnKeyboard('s', deltaTime);  // Movimiento hacia atrás
        }
        if (state[SDL_SCANCODE_A]) {
            camera.OnKeyboard('a', deltaTime);  // Movimiento hacia la izquierda
        }
        if (state[SDL_SCANCODE_D]) {
            camera.OnKeyboard('d', deltaTime);  // Movimiento hacia la derecha
        }
        if (state[SDL_SCANCODE_E]) {
            camera.OnKeyboard('e', deltaTime);  // Movimiento hacia la derecha
        }
        if (state[SDL_SCANCODE_Q]) {
            camera.OnKeyboard('q', deltaTime);  // Movimiento hacia la derecha
        }
        if (state[SDL_SCANCODE_SPACE]) {
            camera.OnKeyboard(' ', deltaTime);  // Movimiento hacia la derecha
        }
        if (state[SDL_SCANCODE_LSHIFT]) {
            camera.OnKeyboard('l', deltaTime);  // Movimiento hacia la derecha
        }

        camera.OnRender(deltaTime);
        uint64_t elapsedTime = (SDL_GetPerformanceCounter() - startTime)/ 100000.0f;
        // Ejecutar Compute Shader
        computeShader.setVec4("sphere", sphere.position.x, sphere.position.y, sphere.position.z, sphere.radius);
        computeShader.setMat4("viewMatrix", camera.getView());

        computeShader.setVec3("front", camera.getFront());
        computeShader.setVec3("up", camera.getUp());
        computeShader.setVec3("right", camera.getRight());
        computeShader.setVec3("cameraPos", camera.getPosition());

        computeShader.setVec2("screenResolution", SCR_WIDTH, SCR_HEIGHT);
        computeShader.setFloat("iTime", elapsedTime);
        computeShader.setFloat("FOV", camera.getFov());
        computeShader.setFloat("show_grid", show_grid);
        computeShader.setFloat("show_axis", show_axes);
        
        glDispatchCompute((SCR_WIDTH + 15) / 16, (SCR_HEIGHT + 15) / 16, 1);
        glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
        
        // Blit del framebuffer al default framebuffer (pantalla)
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // Actualizar pantalla
        SDL_GL_SwapWindow(window);

    }

    // Limpieza
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}


