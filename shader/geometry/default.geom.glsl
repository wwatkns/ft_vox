#version 400 core
layout (points) in;
layout (triangle_strip, max_vertices = 12) out;

in mat4 mvp[];
in vec3 gFragPos[];
flat in int gId[];
flat in int gVisibleFaces[];
flat in int gAo_xz[];
flat in int gAo_y[];

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out float Ao;
flat out int Id;

uniform vec3 viewPos;

/* 2 3
   0 1 */
void    AddQuad(vec4 center, vec4 dy, vec4 dx, int ao) {
    Ao = 1.0-((ao & 0xC0) >> 6)/3.; // 0
    TexCoords = vec2(0, 1);
    gl_Position = center + ( dx - dy);
    EmitVertex();
    Ao = 1.0-((ao & 0x30) >> 4)/3.; // 1
    TexCoords = vec2(1, 1);
    gl_Position = center + (-dx - dy);
    EmitVertex();
    Ao = 1.0-((ao & 0x0C) >> 2)/3.; // 2
    TexCoords = vec2(0, 0);
    gl_Position = center + ( dx + dy);
    EmitVertex();
    Ao = 1.0-((ao & 0x03) >> 0)/3.; // 3
    TexCoords = vec2(1, 0);
    gl_Position = center + (-dx + dy);
    EmitVertex();
    EndPrimitive();
}

/* 2 0
   3 1 */
void    AddQuad2(vec4 center, vec4 dy, vec4 dx, int ao) {
    Ao = 1.0-((ao & 0x03) >> 0)/3.; // 3
    TexCoords = vec2(1, 0);
    gl_Position = center + ( dx - dy);
    EmitVertex();
    Ao = 1.0-((ao & 0x30) >> 4)/3.; // 1
    TexCoords = vec2(1, 1);
    gl_Position = center + (-dx - dy);
    EmitVertex();
    Ao = 1.0-((ao & 0x0C) >> 2)/3.; // 2
    TexCoords = vec2(0, 0);
    gl_Position = center + ( dx + dy);
    EmitVertex();
    Ao = 1.0-((ao & 0xC0) >> 6)/3.; // 0
    TexCoords = vec2(0, 1);
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


    // Normal = vec3( 1.0, 0.0, 0.0);
    // if ( (gVisibleFaces[0] & 0x20) != 0 && dot(Normal, (FragPos + dx.xyz) - viewPos) < 0) /* right */
    //     AddQuad(center + dx, dy, dz);
    // Normal = vec3(-1.0, 0.0, 0.0);
    // if ( (gVisibleFaces[0] & 0x10) != 0 && dot(Normal, (FragPos - dx.xyz) - viewPos) < 0) /* left */
    //     AddQuad2(center - dx, dz, dy);
    // Normal = vec3( 0.0, 1.0, 0.0);
    // if ( (gVisibleFaces[0] & 0x02) != 0 && dot(Normal, (FragPos + dy.xyz) - viewPos) < 0) /* top */
    //     AddQuad(center + dy, dz, dx);
    // Normal = vec3( 0.0,-1.0, 0.0);
    // if ( (gVisibleFaces[0] & 0x01) != 0 && dot(Normal, (FragPos - dy.xyz) - viewPos) < 0) /* bottom */
    //     AddQuad(center - dy, dx, dz);
    // Normal = vec3( 0.0, 0.0, 1.0);
    // if ( (gVisibleFaces[0] & 0x08) != 0 && dot(Normal, (FragPos + dz.xyz) - viewPos) < 0) /* front */
    //     AddQuad2(center + dz, dx, dy);
    // Normal = vec3( 0.0, 0.0,-1.0);
    // if ( (gVisibleFaces[0] & 0x04) != 0 && dot(Normal, (FragPos - dz.xyz) - viewPos) < 0) /* back */
    //     AddQuad(center - dz, dy, dx);

    /* fixes visual issue */
    Normal = vec3( 1.0, 0.0, 0.0);
    if (dot(Normal, (FragPos + dx.xyz) - viewPos) < 0) {
        if ( (gVisibleFaces[0] & 0x20) != 0) /* right */
            AddQuad(center + dx, dy, dz, (gAo_xz[0] & 0xFF000000) >> 24);
    }
    else {
        Normal = vec3(-1.0, 0.0, 0.0);
        if ( (gVisibleFaces[0] & 0x10) != 0) /* left */
            AddQuad2(center - dx, dz, dy, (gAo_xz[0] & 0x00FF0000) >> 16);
    }
    Normal = vec3( 0.0, 1.0, 0.0);
    if (dot(Normal, (FragPos + dy.xyz) - viewPos) < 0) {
        if ( (gVisibleFaces[0] & 0x02) != 0) /* top */
            AddQuad(center + dy, dz, dx, (gAo_y[0] & 0x0000FF00) >> 8);
    }
    else {
        Normal = vec3( 0.0,-1.0, 0.0);
        if ( (gVisibleFaces[0] & 0x01) != 0) /* bottom */
            AddQuad(center - dy, dx, dz, (gAo_y[0] & 0x000000FF));
    }
    Normal = vec3( 0.0, 0.0, 1.0);
    if (dot(Normal, (FragPos + dz.xyz) - viewPos) < 0) {
        if ( (gVisibleFaces[0] & 0x08) != 0) /* front */
            AddQuad2(center + dz, dx, dy, (gAo_xz[0] & 0x0000FF00) >> 8);
    }
    else {
        Normal = vec3( 0.0, 0.0,-1.0);
        if ( (gVisibleFaces[0] & 0x04) != 0) /* back */
            AddQuad(center - dz, dy, dx, (gAo_xz[0] & 0x000000FF));
    }

}