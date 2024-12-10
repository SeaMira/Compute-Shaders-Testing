#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include "camera3.h"
#include <SDL3/SDL.h>

// Configuración
const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 600;

double deltaTime = 0.0; // time between current frame and last frame
double lastFrame = 0.0;

Camera* global_cam;
glm::vec3 ro;
glm::vec3 up;
glm::vec3 right;
glm::vec3 front;
float fov;


struct ProjectionResult {
    float area;
    glm::vec2 center;
    glm::vec2 axisA;
    glm::vec2 axisB;
    float a, b, c, d, e, f;
};

float iSphere(const glm::vec3& ro, const glm::vec3& rd, const glm::vec4& sph) {
    glm::vec3 oc = ro - glm::vec3(sph);
    float b = dot(oc, rd);
    float c = dot(oc, oc) - sph.w * sph.w;
    float h = b * b - c;
    if (h < 0.0f) return -1.0f;
    return -b - sqrt(h);
}

ProjectionResult projectSphere(const glm::vec4& sph, const glm::mat4& cam, float fle) {
    glm::vec3 o = glm::vec3(cam * glm::vec4(sph.x, sph.y, sph.z, 1.0f));
    float r2 = sph.w * sph.w;
    float z2 = o.z * o.z;
    float l2 = dot(o, o);
    float area = -3.141593f * fle * fle * r2 * sqrt(abs((l2 - r2) / (r2 - z2))) / (r2 - z2);

    glm::vec2 axa = glm::vec2(o.x, o.y) * (float)(fle * sqrt(-r2 * (r2 - l2) / ((l2 - z2) * (r2 - z2) * (r2 - z2))));
    glm::vec2 axb = glm::vec2(-o.y, o.x) * (float)(fle * sqrt(-r2 * (r2 - l2) / ((l2 - z2) * (r2 - z2) * (r2 - l2))));
    glm::vec2 cen = glm::vec2(o.x, o.y) / (float)((z2 - r2) * fle * o.z);

    return {area, cen, axa, axb, r2 - o.y * o.y - z2, 2.0f * o.x * o.y, r2 - o.x * o.x - z2,
            -2.0f * o.x * o.z * fle, -2.0f * o.y * o.z * fle, (r2 - l2 + z2) * fle * fle};
}

void renderImage(std::vector<Uint32>& pixels, const glm::vec4* spheres, int sphereCount, const glm::mat4& viewMatrix,
                 const glm::vec2& resolution, bool showGrid, bool showAxis, float iTime);

Uint32 vec4ToUint32(const glm::vec4 color) {
    Uint8 r = static_cast<Uint8>(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f);
    Uint8 g = static_cast<Uint8>(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f);
    Uint8 b = static_cast<Uint8>(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f);
    Uint8 a = static_cast<Uint8>(glm::clamp(color.a, 0.0f, 1.0f) * 255.0f);
    return (a << 24) | (b << 16) | (g << 8) | r;
}

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Error al inicializar SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Raytracing Secuencial", SCR_WIDTH, SCR_HEIGHT, SDL_WINDOW_OPENGL);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCR_WIDTH, SCR_HEIGHT);

    // Sphere sphere = {glm::vec3(0.0f, 0.0f, -5.0f), 2.0f};
    glm::vec4 spheres[3] = {glm::vec4(-2.0f, 1.0f, 0.0f, 1.1f), 
                            glm::vec4(3.0f, 1.5f, 1.0f, 1.2f), 
                            glm::vec4(1.0f, -1.0f, 1.0f, 1.3f)};
    Camera camera(SCR_WIDTH, SCR_HEIGHT);

    global_cam = &camera;

    camera.SetPosition(3.0f, 0.0f, 0.0f);

    bool running = true;
    SDL_Event event;
    std::vector<Uint32> pixels(SCR_WIDTH * SCR_HEIGHT);
    glm::vec2 resolution(SCR_WIDTH, SCR_HEIGHT);

    float lastX = SCR_WIDTH/2.0f;
    float lastY = SCR_HEIGHT/2.0f;

    bool show_grid = false;
    bool show_axis = false;

    uint64_t startTime = SDL_GetPerformanceCounter();
    uint64_t frequency = SDL_GetPerformanceFrequency();

    while (running) {

        uint64_t currentFrame = SDL_GetPerformanceCounter();
		deltaTime = static_cast<double>(currentFrame - lastFrame) / frequency;
		lastFrame = currentFrame;
        std::cout << "FPS: " << 1 / deltaTime << std::endl;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                camera.OnMouse((float) event.motion.x, (float) event.motion.y);

                lastX = (float) event.motion.x;
                lastY = (float) event.motion.y;

            }
            if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                camera.OnScroll(event.wheel.y);
            }
        }
        // std::cout << camera.getYaw() << ", " << camera.getPitch() << std::endl;

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
            camera.OnKeyboard('e', deltaTime);  // acelerar
        }
        if (state[SDL_SCANCODE_Q]) {
            camera.OnKeyboard('q', deltaTime);  // desacelerar
        }
        if (state[SDL_SCANCODE_SPACE]) {
            camera.OnKeyboard(' ', deltaTime);  // subir
        }
        if (state[SDL_SCANCODE_LSHIFT]) {
            camera.OnKeyboard('l', deltaTime);  // bajar
        }

        uint64_t elapsedTime = (SDL_GetPerformanceCounter() - startTime)/ 100000.0f;

        ro = camera.getPosition();
        up = camera.getUp();
        front = camera.getFront();
        right = camera.getRight();
        fov = camera.getFov()/90.0f;

        // Renderizar cada píxel secuencialmente
        // for (int y = 0; y < SCR_HEIGHT; ++y) {
        //     for (int x = 0; x < SCR_WIDTH; ++x) {
        //         pixels[y * SCR_WIDTH + x] = calculatePixel(x, y, sphere);
        //     }
        // }
        renderImage(pixels, spheres, 3, camera.getView(), resolution, show_grid, show_axis, elapsedTime);

        camera.OnRender(deltaTime);

        // Actualizar textura y dibujar en pantalla
        // std::vector<Uint32> pixelBuffer(SCR_WIDTH * SCR_HEIGHT);
        // for (int i = 0; i < pixels.size(); ++i) {
        //     pixelBuffer[i] = vec4ToUint32(pixels[i]);
        // }
        SDL_UpdateTexture(texture, nullptr, pixels.data(), SCR_WIDTH * sizeof(Uint32));
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}


void renderImage(std::vector<Uint32>& pixels, const glm::vec4* spheres, int sphereCount, const glm::mat4& viewMatrix,
                const glm::vec2& resolution, bool showGrid, bool showAxis, float iTime) {
                float aspect = resolution.x / resolution.y;
    for (int y = 0; y < resolution.y; ++y) {
        for (int x = 0; x < resolution.x; ++x) {
            glm::vec2 uv = glm::vec2(x, resolution.y - y) / resolution * 2.0f - 1.0f;
            uv.x *= aspect;

            glm::vec3 rd = glm::normalize(uv.x * right + uv.y * up + fov * front);

            float tmin = 10000.0f;
            glm::vec3 color = glm::vec3(0.0f);

            for (int i = 0; i < sphereCount; ++i) {
                float t = iSphere(ro, rd, spheres[i]);
                if (t > 0.0f && t < tmin) {
                    tmin = t;
                    glm::vec3 pos = ro + t * rd;
                    glm::vec3 nor = glm::normalize(pos - glm::vec3(spheres[i]));
                    glm::vec3 lightDir = glm::normalize(glm::vec3(2.0f, 1.4f, -1.0f));
                    float ndl = glm::dot(nor, lightDir);
                    color = glm::mix(color, glm::vec3(1.0f, 0.9f, 0.8f) * std::max(ndl, 0.0f), 0.5f);
                }
            }

            pixels[y * (int)resolution.x + x] = vec4ToUint32(glm::vec4(color, 1.0f));
        }
    }
}