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