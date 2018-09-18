#version 400 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in int aId;

out mat4 mvp;
out vec3 gFragPos;
flat out int gId;
// out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
// uniform mat4 lightSpaceMat;

void main() {
    // FragPosLightSpace = lightSpaceMat * vec4(FragPos, 1.0);
    mvp = projection * view * model;
    gl_Position = mvp * vec4(aPos, 1.0);
    
    gId = aId;
    gFragPos = vec3(model * vec4(aPos, 1.0));
}