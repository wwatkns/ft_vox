#version 400 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in int aId;
layout (location = 3) in int aFaceId;
layout (location = 4) in ivec2 aAo;

out mat4 mvp;
out vec3 gFragPos;
flat out vec3 gNormal;
flat out int gId;
flat out int gFaceId;
flat out ivec2 gAo;

uniform mat4 _model;
uniform mat4 _mvp;

void main() {
    gl_Position = _mvp * vec4(aPos, 1.0);
    mvp = _mvp;
    gId = aId;
    gFaceId = aFaceId;
    gAo = aAo;
    gNormal = aNormal;
    gFragPos = vec3(_model * vec4(aPos, 1.0));
}