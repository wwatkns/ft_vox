#version 400 core
layout (points) in;
layout (triangle_strip, max_vertices = 12) out;

in mat4 mvp[];
in vec3 gFragPos[];
flat in int gId[];
flat in int gVisibleFaces[];

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
flat out int Id;

uniform vec3 viewPos;

void    AddQuad(vec4 center, vec4 dy, vec4 dx) {
    TexCoords = vec2(0, 1);
    gl_Position = center + ( dx - dy);
    EmitVertex();
    TexCoords = vec2(1, 1);
    gl_Position = center + (-dx - dy);
    EmitVertex();
    TexCoords = vec2(0, 0);
    gl_Position = center + ( dx + dy);
    EmitVertex();
    TexCoords = vec2(1, 0);
    gl_Position = center + (-dx + dy);
    EmitVertex();
    EndPrimitive();
}

void    main() {
    vec4 center = gl_in[0].gl_Position;
    
    vec4 dx = mvp[0][0] / 2.0;// * 0.75;
    vec4 dy = mvp[0][1] / 2.0;// * 0.75;
    vec4 dz = mvp[0][2] / 2.0;// * 0.75;

    Id = gId[0];
    FragPos = gFragPos[0];

    Normal = vec3( 1.0, 0.0, 0.0);
    if ( (gVisibleFaces[0] & 0x20) != 0 && dot(Normal, (FragPos + dx.xyz) - viewPos) <= -0.5)
        AddQuad(center + dx, dy, dz);
    Normal = vec3(-1.0, 0.0, 0.0);
    if ( (gVisibleFaces[0] & 0x10) != 0 && dot(Normal, (FragPos - dx.xyz) - viewPos) <= -0.5)
        AddQuad(center - dx, dz, dy);
    Normal = vec3( 0.0, 1.0, 0.0);
    if ( (gVisibleFaces[0] & 0x02) != 0 && dot(Normal, (FragPos + dy.xyz) - viewPos) <= 0.0)
        AddQuad(center + dy, dz, dx);
    Normal = vec3( 0.0,-1.0, 0.0);
    if ( (gVisibleFaces[0] & 0x01) != 0 && dot(Normal, (FragPos - dy.xyz) - viewPos) <= 0.0)
        AddQuad(center - dy, dx, dz);
    Normal = vec3( 0.0, 0.0, 1.0);
    if ( (gVisibleFaces[0] & 0x08) != 0 && dot(Normal, (FragPos + dz.xyz) - viewPos) <= 0.0)
        AddQuad(center + dz, dx, dy);
    Normal = vec3( 0.0, 0.0,-1.0);
    if ( (gVisibleFaces[0] & 0x04) != 0 && dot(Normal, (FragPos - dz.xyz) - viewPos) <= 0.0)
        AddQuad(center - dz, dy, dx);

    // Normal = vec3(1.0, 0.0, 0.0);
    // if (dot(Normal, (FragPos + dx.xyz) - viewPos) <= -0.5)
    //     AddQuad(center + dx, dy, dz);
    // else {
    //     Normal = vec3(-1.0, 0.0, 0.0);
    //     AddQuad(center - dx, dz, dy);
    // }
    // Normal = vec3(0.0, 1.0, 0.0);
    // if (dot(Normal, (FragPos + dy.xyz) - viewPos) <= 0.0)
    //     AddQuad(center + dy, dz, dx);
    // else {
    //     Normal = vec3(0.0,-1.0, 0.0);
    //     AddQuad(center - dy, dx, dz);
    // }
    // Normal = vec3(0.0, 0.0, 1.0);
    // if (dot(Normal, (FragPos + dz.xyz) - viewPos) <= 0.0)
    //     AddQuad(center + dz, dx, dy);
    // else {
    //     Normal = vec3(0.0, 0.0,-1.0);
    //     AddQuad(center - dz, dy, dx);
    // }

    // Normal = vec3( 1.0, 0.0, 0.0);
    // AddQuad(center + dx, dy, dz);
    // Normal = vec3(-1.0, 0.0, 0.0);
    // AddQuad(center - dx, dz, dy);
    // Normal = vec3( 0.0, 1.0, 0.0);
    // AddQuad(center + dy, dz, dx);
    // Normal = vec3( 0.0,-1.0, 0.0);
    // AddQuad(center - dy, dx, dz);
    // Normal = vec3( 0.0, 0.0, 1.0);
    // AddQuad(center + dz, dx, dy);
    // Normal = vec3( 0.0, 0.0,-1.0);
    // AddQuad(center - dz, dy, dx);
}