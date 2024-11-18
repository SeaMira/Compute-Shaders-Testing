#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <glad/glad.h>

// Configuración de la ventana y la textura
const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 600;
const int TEXTURE_WIDTH = 800; // Resolución de la textura
const int TEXTURE_HEIGHT = 600;
const int RANGE = 10;

int main(int argv, char** args) {
    // Inicializa SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Configura atributos de OpenGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Crea una ventana de SDL
    SDL_Window* window = SDL_CreateWindow("Mouse Pixel Highlighter", 
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                          SCR_WIDTH, SCR_HEIGHT, 
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    // Crea un contexto OpenGL
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Inicializa GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Crea un compute shader
    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    const char* shaderSource = R"(
        #version 430
        layout(local_size_x = 16, local_size_y = 16) in;
        layout(rgba8, binding = 0) uniform image2D outputImage; // Cambiar `rgba32f` a `rgba8`
        uniform ivec2 mousePos;
        uniform int range;

        void main() {
            ivec2 coords = ivec2(gl_GlobalInvocationID.xy);
            vec4 color = vec4(0.0, 0.0, 0.0, 1.0); // Fondo negro

            int dx = abs(coords.x - mousePos.x);
            int dy = abs(coords.y - mousePos.y);
            if (dx <= range && dy <= range) {
                color = vec4(1.0, 1.0, 1.0, 1.0); // Blanco
            }
            imageStore(outputImage, coords, color); // OpenGL convierte automáticamente a [0, 255]
        }
    )";
    glShaderSource(computeShader, 1, &shaderSource, nullptr);
    glCompileShader(computeShader);

    // Verifica errores de compilación
    GLint success;
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(computeShader, 512, nullptr, infoLog);
        std::cerr << "Error compiling compute shader: " << infoLog << std::endl;
        return -1;
    }

    GLuint computeProgram = glCreateProgram();
    glAttachShader(computeProgram, computeShader);
    glLinkProgram(computeProgram);
    glDeleteShader(computeShader);

    // Crea la textura en OpenGL
    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

    // Configura un PBO para transferencias optimizadas
    GLuint pbo;
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
    glBufferData(GL_PIXEL_PACK_BUFFER, TEXTURE_WIDTH * TEXTURE_HEIGHT * 4 * sizeof(uint8_t), nullptr, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);


    // Configura SDL para renderizar
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* sdlTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, 
                                                 SDL_TEXTUREACCESS_STREAMING, 
                                                 TEXTURE_WIDTH, TEXTURE_HEIGHT);

    // Variables para las dimensiones de la ventana y escalado
    int windowWidth, windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight); // Obtén el tamaño de la ventana
    float scaleX = (float)TEXTURE_WIDTH / (float)windowWidth;
    float scaleY = (float)TEXTURE_HEIGHT / (float)windowHeight;

    // Variables del mouse
    int mouseX = 0, mouseY = 0;

    double deltaTime = 0.0f; // time between current frame and last frame
    double lastFrame = 0.0f;

    // Bucle de renderizado
    bool running = true;
    SDL_Event event;
    while (running) {
        static uint64_t frequency = SDL_GetPerformanceFrequency();
		uint64_t currentFrame = SDL_GetPerformanceCounter();
		deltaTime = static_cast<double>(currentFrame - lastFrame) / frequency;
		lastFrame = currentFrame;
        std::cout << "FPS: " << 1 / deltaTime << std::endl;
        // Obtiene la posición del mouse
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
        SDL_GetMouseState(&mouseX, &mouseY);
        int textureMouseX = (int)(mouseX * scaleX);
        int textureMouseY = (int)(mouseY * scaleY);


        // Invierte las coordenadas Y (de SDL a OpenGL)
        // mouseY = TEXTURE_HEIGHT - mouseY;

        // Ejecuta el compute shader
        glUseProgram(computeProgram);
        glUniform2i(glGetUniformLocation(computeProgram, "mousePos"), textureMouseX, textureMouseY);
        glUniform1i(glGetUniformLocation(computeProgram, "range"), RANGE);
        glDispatchCompute(TEXTURE_WIDTH / 16, TEXTURE_HEIGHT / 16, 1);
        glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

        // Inicia la transferencia asincrónica con PBO
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
        glReadPixels(0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        // Mapea los datos del PBO para pasarlos a SDL
        Uint8* pixels = (Uint8*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
        if (pixels) {
            SDL_UpdateTexture(sdlTexture, nullptr, pixels, TEXTURE_WIDTH * 4);
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        // Leer datos de la textura y actualizarlos en SDL
        // glBindTexture(GL_TEXTURE_2D, texture);
        // Uint8* pixels = new Uint8[TEXTURE_WIDTH * TEXTURE_HEIGHT * 4];
        // glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        // SDL_UpdateTexture(sdlTexture, nullptr, pixels, TEXTURE_WIDTH * 4);
        // delete[] pixels;

        // Renderiza la textura en SDL
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, sdlTexture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    // Limpieza
    SDL_DestroyTexture(sdlTexture);
    SDL_DestroyRenderer(renderer);
    glDeleteTextures(1, &texture);
    glDeleteProgram(computeProgram);
    glDeleteBuffers(1, &pbo);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}