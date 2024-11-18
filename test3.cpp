#include <iostream>
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstring> 
#include "shader_c.h"

// Configuración de la ventana y la textura
const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 600;
const int TEXTURE_WIDTH = 800; // Resolución de la textura
const int TEXTURE_HEIGHT = 600;
const int RANGE = 10;

double deltaTime = 0.0; // time between current frame and last frame
double lastFrame = 0.0;

void setTexture(GLuint& texId);

int main(int argv, char** args) {
    // Inicializar SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Crear ventana y contexto OpenGL
    SDL_Window* window = SDL_CreateWindow("Compute Shader + SDL Renderer",
                                          SCR_WIDTH, SCR_HEIGHT, SDL_WINDOW_OPENGL);
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);


    ComputeShader computeShader("computeShader2.cs");
    computeShader.use();

    // Crear textura OpenGL para el Compute Shader
    GLuint texture;
    setTexture(texture);

    // Configurar un PBO para transferencias optimizadas
    GLuint pbo;
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
    glBufferData(GL_PIXEL_PACK_BUFFER, SCR_WIDTH * SCR_HEIGHT * 4 * sizeof(uint8_t), nullptr, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);

    // Configurar textura SDL
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    SDL_Texture* sdlTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                                                 SCR_WIDTH, SCR_HEIGHT);

    bool running = true;
    SDL_Event event;
    float mouseX = 0.0f, mouseY = 0.0f;

    while (running) {

        static uint64_t frequency = SDL_GetPerformanceFrequency();
		uint64_t currentFrame = SDL_GetPerformanceCounter();
		deltaTime = static_cast<double>(currentFrame - lastFrame) / frequency;
		lastFrame = currentFrame;

        while (SDL_PollEvent(&event)) {
            std::cout << "FPS: " << 1 / deltaTime << std::endl;
            if (event.type == SDL_EVENT_QUIT) running = false;
        }

        // Obtener posición del mouse
        SDL_GetMouseState(&mouseX, &mouseY);
        int textureMouseX = (int)mouseX;
        int textureMouseY = SCR_HEIGHT - (int)mouseY;

        int lightX = mouseX; // Mueve la luz con el mouse
        int lightY = mouseY;
        float lightIntensity = 5.0f; // Ajusta la intensidad

        // Ejecutar el Compute Shader
        computeShader.setInt("SCR_WIDTH", SCR_WIDTH);
        computeShader.setInt("SCR_HEIGHT", SCR_HEIGHT);
        computeShader.setVec2I("lightPos", glm::vec2(lightX, lightY));
        computeShader.setFloat("lightIntensity", lightIntensity);
        computeShader.setVec4("baseColor", glm::vec4(0.2f, 0.5f, 0.8f, 1.0f));
    
        glDispatchCompute((SCR_WIDTH + 15) / 16, (SCR_HEIGHT + 15) / 16, 1);
        glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

        // Transferir datos con PBO
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
        glReadPixels(0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        // Mapear el PBO para pasarlo a SDL
        Uint8* pixels = (Uint8*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
        if (pixels) {
            SDL_UpdateTexture(sdlTexture, nullptr, pixels, SCR_WIDTH * 4);
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        // Renderizar la textura con SDL
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, sdlTexture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    // Limpieza
    glDeleteBuffers(1, &pbo);
    glDeleteTextures(1, &texture);
    // glDeleteProgram(computeProgram);
    SDL_DestroyTexture(sdlTexture);
    SDL_DestroyRenderer(renderer);
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void setTexture(GLuint& texId) {
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindImageTexture(0, texId, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
}