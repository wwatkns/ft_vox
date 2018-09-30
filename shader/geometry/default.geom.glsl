#version 400 core
layout (points) in;
layout (triangle_strip, max_vertices = 12) out;

in mat4 mvp[];
in vec3 gFragPos[];
flat in int gId[];
flat in int gVisibleFaces[];
flat in int gLight[];
flat in ivec2 gAo[];

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out float Ao;
out float Light;
flat out int Id;

uniform vec3 viewPos;

const float[4] aoCurve = float[4]( 1.0, 0.55, 0.3, .1 ); // BEST
// const float[4] aoCurve = float[4]( 1.0, 0.7, 0.6, 0.15 ); // best soft
// const float[4] aoCurve = float[4]( 1.0, 0.66, 0.33, 0. ); // linear
// const float[4] aoCurve = float[4]( 1.0, 0.4, 0.2, 0. ); // accentuated debug
/* 3 +---+ 2
     | / |
   1 +---+ 0 */
void    AddQuad(vec4 center, vec4 dy, vec4 dx, int ao, bool flip_uv) {
    Ao = aoCurve[(ao&0xC0)>>6];
    TexCoords = (flip_uv ? vec2(1, 0) : vec2(0, 1) );
    gl_Position = center + ( dx - dy);
    EmitVertex();
    Ao = aoCurve[(ao&0x30)>>4];
    TexCoords = vec2(1, 1);
    gl_Position = center + (-dx - dy);
    EmitVertex();
    Ao = aoCurve[(ao&0x0C)>>2];
    TexCoords = vec2(0, 0);
    gl_Position = center + ( dx + dy);
    EmitVertex();
    Ao = aoCurve[(ao&0x03)>>0];
    TexCoords = (flip_uv ? vec2(0, 1) : vec2(1, 0) );
    gl_Position = center + (-dx + dy);
    EmitVertex();
    EndPrimitive();
}
/* 1 +---+ 3
     | \ |
   0 +---+ 2 */
void    AddQuadFlipped(vec4 center, vec4 dy, vec4 dx, int ao, bool flip_uv) {
    Ao = aoCurve[(ao&0x30)>>4];
    TexCoords = vec2(1, 1);
    gl_Position = center + (-dx - dy);
    EmitVertex();
    Ao = aoCurve[(ao&0x03)>>0];
    TexCoords = (flip_uv ? vec2(0, 1) : vec2(1, 0) );
    gl_Position = center + (-dx + dy);
    EmitVertex();
    Ao = aoCurve[(ao&0xC0)>>6];
    TexCoords = (flip_uv ? vec2(1, 0) : vec2(0, 1) );
    gl_Position = center + ( dx - dy);
    EmitVertex();
    Ao = aoCurve[(ao&0x0C)>>2];
    TexCoords = vec2(0, 0);
    gl_Position = center + ( dx + dy);
    EmitVertex();
    EndPrimitive();
}

void    main() {
    vec4 center = gl_in[0].gl_Position;
    
    vec4 dx = mvp[0][0] / 2.0;
    vec4 dy = mvp[0][1] / 2.0;
    vec4 dz = mvp[0][2] / 2.0;

    Id = gId[0];
    FragPos = gFragPos[0];
    // Light = (float((gAo[0][1] & 0xFF000000) >> 24)/15)*0.75+0.25;

    // Normal = vec3( 1.0, 0.0, 0.0);
    // if ( (gVisibleFaces[0] & 0x20) != 0 && dot(Normal, (FragPos + dx.xyz) - viewPos) < 0) /* right */
    //     AddQuad(center + dx, dy, dz, (gAo[0][0] & 0xFF000000) >> 24, false);
    // Normal = vec3(-1.0, 0.0, 0.0);
    // if ( (gVisibleFaces[0] & 0x10) != 0 && dot(Normal, (FragPos - dx.xyz) - viewPos) < 0) /* left */
    //     AddQuad(center - dx, dz, dy, (gAo[0][0] & 0x00FF0000) >> 16, true);
    // Normal = vec3( 0.0, 1.0, 0.0);
    // if ( (gVisibleFaces[0] & 0x02) != 0 && dot(Normal, (FragPos + dy.xyz) - viewPos) < 0) /* top */
    //     AddQuad(center + dy, dz, dx, (gAo[0][1] & 0x0000FF00) >> 8, false);
    // Normal = vec3( 0.0,-1.0, 0.0);
    // if ( (gVisibleFaces[0] & 0x01) != 0 && dot(Normal, (FragPos - dy.xyz) - viewPos) < 0) /* bottom */
    //     AddQuad(center - dy, dx, dz, (gAo[0][1] & 0x000000FF), false);
    // Normal = vec3( 0.0, 0.0, 1.0);
    // if ( (gVisibleFaces[0] & 0x08) != 0 && dot(Normal, (FragPos + dz.xyz) - viewPos) < 0) /* front */
    //     AddQuad(center + dz, dx, dy, (gAo[0][0] & 0x0000FF00) >> 8, true);
    // Normal = vec3( 0.0, 0.0,-1.0);
    // if ( (gVisibleFaces[0] & 0x04) != 0 && dot(Normal, (FragPos - dz.xyz) - viewPos) < 0) /* back */
    //     AddQuad(center - dz, dy, dx, (gAo[0][0] & 0x000000FF), false);

    int flippedQuads = (gAo[0][1] & 0x00FF0000) >> 16;
    /* fixes visual issue (partially, best is to pass faces to geometry shader and compute them on CPU, instead of point) */
    Normal = vec3( 1.0, 0.0, 0.0);
    if (dot(Normal, (FragPos + dx.xyz) - viewPos) < 0) {
        if ( (gVisibleFaces[0] & 0x20) != 0) { /* right */
            Light = (float((gLight[0] & 0xF00000) >> 20)/15);
            int ao = (gAo[0][0] & 0xFF000000) >> 24;
            if ( (flippedQuads & 0x20) != 0 )
                AddQuadFlipped(center + dx, dy, dz, ao, false);
            else
                AddQuad(center + dx, dy, dz, ao, false);
        }
    }
    else {
        Normal = vec3(-1.0, 0.0, 0.0);
        if ( (gVisibleFaces[0] & 0x10) != 0) { /* left */
            Light = (float((gLight[0] & 0x0F0000) >> 16)/15);
            int ao = (gAo[0][0] & 0x00FF0000) >> 16;
            if ( (flippedQuads & 0x10) != 0 )
                AddQuadFlipped(center - dx, dz, dy, ao, true);
            else
                AddQuad(center - dx, dz, dy, ao, true);
        }
    }
    Normal = vec3( 0.0, 1.0, 0.0);
    if (dot(Normal, (FragPos + dy.xyz) - viewPos) < 0) {
        if ( (gVisibleFaces[0] & 0x02) != 0) { /* top */
            Light = (float((gLight[0] & 0x0000F0) >> 4)/15);
            int ao = (gAo[0][1] & 0x0000FF00) >> 8;
            if ( (flippedQuads & 0x02) != 0 )
                AddQuadFlipped(center + dy, dz, dx, ao, false);
            else
                AddQuad(center + dy, dz, dx, ao, false);
        }
    }
    else {
        Normal = vec3( 0.0,-1.0, 0.0);
        if ( (gVisibleFaces[0] & 0x01) != 0) { /* bottom */
            Light = (float(gLight[0] & 0x00000F)/15);
            int ao = (gAo[0][1] & 0x000000FF);
            if ( (flippedQuads & 0x01) != 0 )
                AddQuadFlipped(center - dy, dx, dz, ao, false);
            else
                AddQuad(center - dy, dx, dz, ao, false);
        }
    }
    Normal = vec3( 0.0, 0.0, 1.0);
    if (dot(Normal, (FragPos + dz.xyz) - viewPos) < 0) {
        if ( (gVisibleFaces[0] & 0x08) != 0) { /* front */
            Light = (float((gLight[0] & 0x00F000) >> 12)/15);
            int ao = (gAo[0][0] & 0x0000FF00) >> 8;
            if ( (flippedQuads & 0x08) != 0 )
                AddQuadFlipped(center + dz, dx, dy, ao, true);
            else
                AddQuad(center + dz, dx, dy, ao, true);
        }
    }
    else {
        Normal = vec3( 0.0, 0.0,-1.0);
        if ( (gVisibleFaces[0] & 0x04) != 0) { /* back */
            Light = (float((gLight[0] & 0x000F00) >> 8)/15);
            int ao = (gAo[0][0] & 0x000000FF);
            if ( (flippedQuads & 0x04) != 0 )
                AddQuadFlipped(center - dz, dy, dx, ao, false);
            else
                AddQuad(center - dz, dy, dx, ao, false);
        }
    }
}
