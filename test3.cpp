#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <glad/glad.h>
#include <cstring> 

// Configuración de la ventana y la textura
const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 600;
const int TEXTURE_WIDTH = 800; // Resolución de la textura
const int TEXTURE_HEIGHT = 600;
const int RANGE = 10;

double deltaTime = 0.0; // time between current frame and last frame
double lastFrame = 0.0;

int main(int argv, char** args) {
    // Inicializar SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Crear ventana y contexto OpenGL
    SDL_Window* window = SDL_CreateWindow("Compute Shader + SDL Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          SCR_WIDTH, SCR_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

    // Crear Compute Shader
    const char* shaderSource = R"(
        #version 430
        layout(local_size_x = 16, local_size_y = 16) in;
        layout(rgba8, binding = 0) uniform image2D outputImage;

        uniform int SCR_WIDTH, SCR_HEIGHT;
        uniform ivec2 lightPos; // Posición de la luz en coordenadas de textura
        uniform float lightIntensity; // Intensidad de la luz
        uniform vec4 baseColor; // Color base de la textura

        void main() {
            ivec2 coords = ivec2(gl_GlobalInvocationID.xy);

            // Calcular la distancia del píxel a la fuente de luz
            float dist = length(vec2(coords - lightPos));

            // Atenuar la luz basada en la distancia
            float attenuation = lightIntensity / (1.0 + dist * dist * 0.01);

            vec4 gradientColor = vec4(float(coords.x) / SCR_WIDTH, float(coords.y) / SCR_HEIGHT, 0.5, 1.0);

            float shadow = smoothstep(0.0, 1.0, attenuation);

            // Mezclar el color base con la iluminación
            vec4 color = baseColor * shadow * gradientColor * attenuation;

            // Guardar el color en la textura
            imageStore(outputImage, coords, color);
        }
    )";

    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(computeShader, 1, &shaderSource, nullptr);
    glCompileShader(computeShader);

    GLint success;
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(computeShader, 512, nullptr, log);
        std::cerr << "Error compiling compute shader: " << log << std::endl;
        return -1;
    }

    GLuint computeProgram = glCreateProgram();
    glAttachShader(computeProgram, computeShader);
    glLinkProgram(computeProgram);
    glDeleteShader(computeShader);

    // Crear textura OpenGL para el Compute Shader
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    // Configurar un PBO para transferencias optimizadas
    GLuint pbo;
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
    glBufferData(GL_PIXEL_PACK_BUFFER, SCR_WIDTH * SCR_HEIGHT * 4 * sizeof(uint8_t), nullptr, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);

    // Configurar textura SDL
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* sdlTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                                                 SCR_WIDTH, SCR_HEIGHT);

    bool running = true;
    SDL_Event event;
    int mouseX = 0, mouseY = 0;

    while (running) {

        static uint64_t frequency = SDL_GetPerformanceFrequency();
		uint64_t currentFrame = SDL_GetPerformanceCounter();
		deltaTime = static_cast<double>(currentFrame - lastFrame) / frequency;
		lastFrame = currentFrame;

        while (SDL_PollEvent(&event)) {
            std::cout << "FPS: " << 1 / deltaTime << std::endl;
            if (event.type == SDL_QUIT) running = false;
        }

        // Obtener posición del mouse
        SDL_GetMouseState(&mouseX, &mouseY);
        int textureMouseX = mouseX;
        int textureMouseY = SCR_HEIGHT - mouseY;

        int lightX = mouseX; // Mueve la luz con el mouse
        int lightY = mouseY;
        float lightIntensity = 5.0f; // Ajusta la intensidad

        // Ejecutar el Compute Shader
        glUseProgram(computeProgram);
        // glUniform2i(glGetUniformLocation(computeProgram, "mousePos"), textureMouseX, textureMouseY);
        // glUniform1i(glGetUniformLocation(computeProgram, "range"), RANGE);
        glUniform1i(glGetUniformLocation(computeProgram, "SCR_WIDTH"), SCR_WIDTH);
        glUniform1i(glGetUniformLocation(computeProgram, "SCR_HEIGHT"), SCR_HEIGHT);
        glUniform2i(glGetUniformLocation(computeProgram, "lightPos"), lightX, lightY);
        glUniform1f(glGetUniformLocation(computeProgram, "lightIntensity"), lightIntensity);
        glUniform4f(glGetUniformLocation(computeProgram, "baseColor"), 0.2f, 0.5f, 0.8f, 1.0f); // Color azul
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
        SDL_RenderCopy(renderer, sdlTexture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    // Limpieza
    glDeleteBuffers(1, &pbo);
    glDeleteTextures(1, &texture);
    glDeleteProgram(computeProgram);
    SDL_DestroyTexture(sdlTexture);
    SDL_DestroyRenderer(renderer);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}