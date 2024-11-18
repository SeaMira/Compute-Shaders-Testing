#include <iostream>
#include <SDL3/SDL.h>
#include <glad/glad.h>

// Configuración
const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 600;
const int RANGE = 10;

double deltaTime = 0.0; // time between current frame and last frame
double lastFrame = 0.0;

int main(int argv, char** args) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Error al inicializar SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Compute Shader + SDL3 Renderer",
                                          SCR_WIDTH, SCR_HEIGHT, SDL_WINDOW_OPENGL);
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

    // Crear Compute Shader
    const char* computeShaderSource = R"(
        #version 430
        layout(local_size_x = 16, local_size_y = 16) in;
        layout(rgba8, binding = 0) uniform image2D outputImage;

        uniform ivec2 lightPos;
        uniform int lightRadius;

        void main() {
            ivec2 coords = ivec2(gl_GlobalInvocationID.xy);
            float dist = length(vec2(coords - lightPos));
            float intensity = clamp(1.0 - dist / float(lightRadius), 0.0, 1.0);
            vec4 color = vec4(intensity, intensity * 0.5, intensity * 0.2, 1.0); // Color cálido
            imageStore(outputImage, coords, color);
        }
    )";

    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(computeShader, 1, &computeShaderSource, nullptr);
    glCompileShader(computeShader);
    GLint success;
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(computeShader, 512, nullptr, log);
        std::cerr << "Error compilando el Compute Shader: " << log << std::endl;
        return -1;
    }

    GLuint computeProgram = glCreateProgram();
    glAttachShader(computeProgram, computeShader);
    glLinkProgram(computeProgram);
    glDeleteShader(computeShader);

    // Crear textura para el Compute Shader
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    // Configurar SDL Texture
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    SDL_Texture* sdlTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                                 SDL_TEXTUREACCESS_STREAMING, SCR_WIDTH, SCR_HEIGHT);

    bool running = true;
    SDL_Event event;

    int lightX = SCR_WIDTH / 2, lightY = SCR_HEIGHT / 2;
    int lightRadius = 200;

    while (running) {

        static uint64_t frequency = SDL_GetPerformanceFrequency();
		uint64_t currentFrame = SDL_GetPerformanceCounter();
		deltaTime = static_cast<double>(currentFrame - lastFrame) / frequency;
		lastFrame = currentFrame;

        while (SDL_PollEvent(&event)) {
            std::cout << "FPS: " << 1 / deltaTime << std::endl;
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                lightX = event.motion.x;
                lightY = event.motion.y;

                // Ejecutar Compute Shader
                glUseProgram(computeProgram);
                glUniform2i(glGetUniformLocation(computeProgram, "lightPos"), lightX, lightY);
                glUniform1i(glGetUniformLocation(computeProgram, "lightRadius"), lightRadius);
                glDispatchCompute((SCR_WIDTH + 15) / 16, (SCR_HEIGHT + 15) / 16, 1);
                glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
                // Descargar la textura en SDL
                glBindTexture(GL_TEXTURE_2D, texture);
            }
        }

        void* pixels;
        int pitch;
        SDL_LockTexture(sdlTexture, nullptr, &pixels, &pitch);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        SDL_UnlockTexture(sdlTexture);


        // Renderizar con SDL3
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, sdlTexture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    // Limpieza
    SDL_DestroyTexture(sdlTexture);
    SDL_DestroyRenderer(renderer);
    glDeleteTextures(1, &texture);
    glDeleteProgram(computeProgram);
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
