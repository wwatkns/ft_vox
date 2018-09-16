#version 400 core
layout (location = 0) in vec3 aPos;
// layout (location = 1) in uint aId;
// layout (location = 1) in vec3 aNormal;
// layout (location = 2) in vec2 aTexCoords;

// out vec3 FragPos;
out mat4 mvp; // do something else
// out uint Id;
// out vec3 Normal;
// out vec2 TexCoords;
// out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
// pass viewProjection matrix instead
// uniform mat4 lightSpaceMat;

void main() {
    // FragPos = vec3(model * vec4(aPos, 1.0));
    mvp = projection * view * model;
    // Id = aId;
    // Normal = mat3(transpose(inverse(model))) * aNormal;
    // TexCoords = aTexCoords;
    // FragPosLightSpace = lightSpaceMat * vec4(FragPos, 1.0);
    gl_Position = mvp * vec4(aPos, 1.0);
}