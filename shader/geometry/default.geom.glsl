#version 400 core
layout (points) in;
layout (triangle_strip, max_vertices = 24) out;

in mat4 mvp[];

void    AddQuad(vec4 center, vec4 dy, vec4 dx) {
    // fColor = gColor[0];
    gl_Position = center + ( dx - dy);
    EmitVertex();
    // fColor = gColor[0];
    gl_Position = center + (-dx - dy);
    EmitVertex();
    // fColor = gColor[0];
    gl_Position = center + ( dx + dy);
    EmitVertex();
    // fColor = gColor[0];
    gl_Position = center + (-dx + dy);
    EmitVertex();
    EndPrimitive();
}

void    main() {
    vec4 center = gl_in[0].gl_Position;
    
    vec4 dx = mvp[0][0] / 2.0;
    vec4 dy = mvp[0][1] / 2.0;
    vec4 dz = mvp[0][2] / 2.0;

    AddQuad(center + dx, dy, dz);
    AddQuad(center - dx, dz, dy);
    AddQuad(center + dy, dz, dx);
    AddQuad(center - dy, dx, dz);
    AddQuad(center + dz, dx, dy);
    AddQuad(center - dz, dy, dx);
}


/* for generating faces */

// const vec2 lu = vec2(0.0, 0.5);
// const vec3 dxs[6] = vec3[6](
//     vec3(lu.x, lu.y, lu.x),
//     vec3(lu.x, lu.x, lu.y),
//     vec3(lu.x, lu.x, lu.y),
//     vec3(lu.y, lu.x, lu.x),
//     vec3(lu.y, lu.x, lu.x),
//     vec3(lu.x, lu.y, lu.x)
// );
// const vec3 dys[6] = vec3[6](
//     dxs[1],
//     dxs[0],
//     dxs[3],
//     dxs[2],
//     dxs[0],
//     dxs[3] 
// );
// AddQuad(gl_in[0].gl_Position, mvp * vec4(dxs[gFaceIdx[0]], lu.x), mvp * vec4(dys[gFaceIdx[0]], lu.x));