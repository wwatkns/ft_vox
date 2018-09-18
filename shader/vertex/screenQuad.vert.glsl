#version 400 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec3 FragPos;
out vec2 TexCoords;

uniform float near;

void main() {
    mat4 Model = mat4(1.0);
    Model[3].xyz = vec3(0.0, 0.0, -near);
    Model[0][0] = 1.0;
    Model[1][1] = 1.0;
    gl_Position = Model * vec4(aPos, 1.0);

    FragPos = aPos;
    TexCoords = aTexCoords;
}