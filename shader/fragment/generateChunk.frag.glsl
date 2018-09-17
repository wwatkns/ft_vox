#version 400 core
out vec4 FragColor;

in vec3 FragPos;
in vec2 TexCoords;

uniform vec3 chunkPosition;
uniform vec3 chunkSize;
uniform sampler2D noiseSampler;

#define PI 3.14159265359

// float   random(vec2 p) {
//     return fract(sin(mod(dot(p, vec2(12.9898,78.233)), 3.14))*43758.5453);
// }

// vec2    random2(vec2 p) {
//     return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
// }

// float   noise( vec3 x ) {
//     vec3 p = floor(x);
//     vec3 f = fract(x);
//     f = f*f*(3.0-2.0*f);
//     vec2 uv = (p.xy + vec2(37.0, 17.0) * p.z) + f.xy;
//     vec2 rg = texture(noiseSampler, (uv + 0.5) / 256.0, 0.0).rg;
//     return mix(rg.y, rg.x, f.z);
// }

// float   fbm2d(in vec2 st, in float amplitude, in float frequency, in int octaves, in float lacunarity, in float gain) {
//     float value = 0.0;
//     st *= frequency;
//     for (int i = 0; i < octaves; i++) {
//         value += amplitude * noise(vec3(st, 1.0));
//         st *= lacunarity;
//         amplitude *= gain;
//     }
//     return value;
// }

// // float   voronoi2d(vec2 uv, float scale) {
// //     uv *= scale;
// //     // Tile the space
// //     vec2 i_st = floor(uv);
// //     vec2 f_st = fract(uv);
// //     float m_dist = 1.;
// //     for (int y= -1; y <= 1; y++) {
// //         for (int x= -1; x <= 1; x++) {
// //             vec2 neighbor = vec2(float(x),float(y));     // Neighbor place in the grid
// //             vec2 point = random2(i_st + neighbor);       // Random position from current + neighbor place in the grid
// //             point = 0.5 + 0.5*sin(uTime + 6.2831*point); // Animate the point
// //             vec2 diff = neighbor + point - f_st;         // Vector between the pixel and the point
// //             float dist = length(diff);                   // Distance to the point
// //             m_dist = min(m_dist, dist);                  // Keep the closer distance
// //         }
// //     }
// //     return m_dist;
// // }

// float   map(vec2 p) {
//     float res = 0.8 - fbm2d(p, 1.0, 1.0, 6, 2.2, 0.5) * 1.3;
//     res = fbm2d(p + res, 0.3, 1.0, 5, 1., 0.5);
//     return res;
// }

void    main() {
    vec2 uv = vec2(TexCoords.x, 1.0 - TexCoords.y);

    // float elevation = sin(uv.x * chunkSize.x * PI) * sin(uv.y * chunkSize.z * PI) * 0.5 + 0.5;
    // float elevation = floor(uv.y * 4.) / 4.;

    FragColor.r = uv.x - 1. / (chunkSize.x * 2.0);
    FragColor.r *= uv.y - 1. / (chunkSize.y * 2.0);

    // FragColor.r = fract(uv.y * 256. * chunkSize.y) / chunkSize.y;
    // FragColor = vec4( mod(chunkPosition.x / 4.0, 2.0) * mod(chunkPosition.z / 4.0, 2.0), 0.0, 0.0, 0.0);
}
