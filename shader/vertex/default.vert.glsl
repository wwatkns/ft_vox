#version 400 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in int aId;
layout (location = 2) in int aVisibleFaces;
layout (location = 3) in int aAo_xz;
layout (location = 4) in int aAo_y;

out mat4 mvp;
out vec3 gFragPos;
flat out int gId;
flat out int gVisibleFaces;
flat out int gAo_xz;
flat out int gAo_y;
// out vec4 FragPosLightSpace;

uniform mat4 _model;
uniform mat4 _mvp;
// uniform mat4 lightSpaceMat;

void main() {
    // FragPosLightSpace = lightSpaceMat * vec4(FragPos, 1.0);
    gl_Position = _mvp * vec4(aPos, 1.0);
    mvp = _mvp;
    gId = aId;
    gVisibleFaces = aVisibleFaces;
    gAo_xz = aAo_xz;
    gAo_y = aAo_y;
    gFragPos = vec3(_model * vec4(aPos, 1.0));
}