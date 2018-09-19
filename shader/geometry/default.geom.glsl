#version 400 core
layout (points) in;
layout (triangle_strip, max_vertices = 24) out;

in mat4 mvp[];
in vec3 gFragPos[];
flat in int gId[];

out vec3 FragPos;
out vec3 Normal;
flat out int Id;

uniform vec3 cameraPos2;

void    AddQuad(vec4 center, vec4 dy, vec4 dx) {
    gl_Position = center + ( dx - dy);
    EmitVertex();
    gl_Position = center + (-dx - dy);
    EmitVertex();
    gl_Position = center + ( dx + dy);
    EmitVertex();
    gl_Position = center + (-dx + dy);
    EmitVertex();
    EndPrimitive();
}

// vec4[4] computeVertices(vec4 center, vec4 dy, vec4 dx) {
//     return vec3[4](
//         center + ( dx - dy),
//         center + (-dx - dy),
//         center + ( dx + dy),
//         center + (-dx + dy)
//     );
// }

void    main() {
    vec4 center = gl_in[0].gl_Position;
    
    vec4 dx = mvp[0][0] / 2.0 * 0.75;
    vec4 dy = mvp[0][1] / 2.0 * 0.75;
    vec4 dz = mvp[0][2] / 2.0 * 0.75;

    Id = gId[0];
    FragPos = gFragPos[0];

    vec3[4] vert;
    vec3 norm;

    // if (dx.z < 0) {    
    //     Normal = vec3( 1.0, 0.0, 0.0);
    //     AddQuad(center + dx, dy, dz);
    // }
    // if (-dx.z < 0) {    
    //     Normal = vec3(-1.0, 0.0, 0.0);
    //     AddQuad(center - dx, dz, dy);
    // }
    // if (dy.z < 0) {    
    //     Normal = vec3( 0.0, 1.0, 0.0);
    //     AddQuad(center + dy, dz, dx);
    // }
    // if (-dy.z < 0) {    
    //     Normal = vec3( 0.0,-1.0, 0.0);
    //     AddQuad(center - dy, dx, dz);
    // }
    // if (dz.z < 0) {    
    //     Normal = vec3( 0.0, 0.0, 1.0);
    //     AddQuad(center + dz, dx, dy);
    // }
    // if (-dz.z < 0) {    
    //     Normal = vec3( 0.0, 0.0,-1.0);
    //     AddQuad(center - dz, dy, dx);
    // }

    Normal = vec3( 1.0, 0.0, 0.0);
    AddQuad(center + dx, dy, dz);
    Normal = vec3(-1.0, 0.0, 0.0);
    AddQuad(center - dx, dz, dy);
    Normal = vec3( 0.0, 1.0, 0.0);
    AddQuad(center + dy, dz, dx);
    Normal = vec3( 0.0,-1.0, 0.0);
    AddQuad(center - dy, dx, dz);
    Normal = vec3( 0.0, 0.0, 1.0);
    AddQuad(center + dz, dx, dy);
    Normal = vec3( 0.0, 0.0,-1.0);
    AddQuad(center - dz, dy, dx);
    /*  BIG optimization to display only the 3 faces visibles out of the 6 :
        Relative to the camera position and the cube position we can know
        which faces are visible, and don't render those that are not.
     */
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