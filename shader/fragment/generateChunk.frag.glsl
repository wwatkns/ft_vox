#version 400 core
out vec4 FragColor;

in vec3 FragPos;
in vec2 TexCoords;

uniform vec3 chunkPosition;
uniform vec3 chunkSize;
uniform sampler2D noiseSampler;
uniform float uTime;

#define PI 3.14159265359

float   random(vec2 p) {
    return fract(sin(mod(dot(p, vec2(12.9898,78.233)), 3.14))*43758.5453);
}

vec2    random2(vec2 p) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

float   noise( vec3 x ) {
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = f*f*(3.0-2.0*f);
    vec2 uv = (p.xy + vec2(37.0, 17.0) * p.z) + f.xy;
    vec2 rg = texture(noiseSampler, (uv + 0.5) / 256.0, 0.0).rg;
    return mix(rg.y, rg.x, f.z);
}

float   fbm2d(in vec2 st, in float amplitude, in float frequency, in int octaves, in float lacunarity, in float gain) {
    float value = 0.0;
    st *= frequency;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise(vec3(st, 1.0));
        st *= lacunarity;
        amplitude *= gain;
    }
    return value;
}

float   fbm3d(in vec3 st, in float amplitude, in float frequency, in int octaves, in float lacunarity, in float gain) {
    float value = 0.0;
    st *= frequency;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise(st);
        st *= lacunarity;
        amplitude *= gain;
    }
    return value;
}

// float   voronoi2d(vec2 uv, float scale) {
//     uv *= scale;
//     // Tile the space
//     vec2 i_st = floor(uv);
//     vec2 f_st = fract(uv);
//     float m_dist = 1.;
//     for (int y= -1; y <= 1; y++) {
//         for (int x= -1; x <= 1; x++) {
//             vec2 neighbor = vec2(float(x),float(y));     // Neighbor place in the grid
//             vec2 point = random2(i_st + neighbor);       // Random position from current + neighbor place in the grid
//             point = 0.5 + 0.5*sin(uTime + 6.2831*point); // Animate the point
//             vec2 diff = neighbor + point - f_st;         // Vector between the pixel and the point
//             float dist = length(diff);                   // Distance to the point
//             m_dist = min(m_dist, dist);                  // Keep the closer distance
//         }
//     }
//     return m_dist;
// }

#define DIRT 1/255.
#define STONE 3/255.
#define BEDROCK 4/255.
#define COAL 5/255.
#define IRON 6/255.
#define DIAMOND 7/255.

float   map(vec3 p) {
    float res;
    /* bedrock level */
    if (p.y == 0 || fbm3d(p, 1.0, 20.0, 2, 1.5, 0.5) > p.y/3.)
        return BEDROCK;

    /* terrain and caves */
    int g1 = int(fbm3d(p, 0.5, 0.025, 4, 1.5, 0.5) > 0.45 * (p.y / 128.));
    int g2 = int(fbm3d(p, 0.35, 0.12, 4, 1.0, 0.35) > 0.1);
    int g0 = int(fbm3d(p, 0.5, 0.01, 6, 1.7, 0.5) > p.y / 255.);
    /* resource distribution */
    int g3 = int(fbm3d(p+ 10., 0.29, 0.2, 4, 1.5, 0.4) < 0.1 && p.y < 130);/* common distribution : coal    */
    int g4 = int(fbm3d(p-100., 0.40, 0.2, 4, 1.5,0.33) < 0.1 && p.y < 64); /* medium distribution : iron    */
    int g5 = int(fbm3d(p+100., 0.40, 0.2, 4, 1.5,0.38) < 0.1 && p.y < 16); /*   rare distribution : diamond */
    /* stone */
    int g6 = int(fbm3d(p + 6, 0.25, 0.07, 6, 2., 0.2) > p.y / 255.);      // stone repartition

    res = float(g0 & g1 & g2) * DIRT;

    res = (res == DIRT  && g6 == 1 ? STONE : res );
    res = (res == STONE && g4 == 1 ? IRON : res );
    res = (res == STONE && g3 == 1 ? COAL : res );
    res = (res == STONE && g5 == 1 ? DIAMOND : res );

    // res = g3 * COAL; // tmp
    // res = g4 * IRON; // tmp
    // res = g5 * DIAMOND; // tmp
    // res = g0 * DIRT;
    return res;
}

/* 3d volume texture */
void    main() {
    vec2 uv = vec2(TexCoords.x, 1.0 - TexCoords.y);
    vec2 c_uv = floor(uv * chunkSize.xy) / (chunkSize.xy - 1.);
    float z = mod(floor(uv.y * chunkSize.y * chunkSize.z), chunkSize.z) / (chunkSize.z - 1.);
    vec3 pos = vec3(c_uv, z);
    // vec3 chunk = chunkPosition / chunkSize;

    // vec3 worldPos = (chunk - 1.0 + pos) * chunkSize;
    vec3 worldPos = (chunkPosition + pos * chunkSize);
    FragColor.r = sqrt(map(worldPos)); // values from [0..255] (0..1) are in normalized fixed-point representation, a simple sqrt() fixes that.

    // FragColor.r  = (mod(pos.z,  0.125) <= 1/chunkSize.z ? 0.0 : 1.0);
    // FragColor.r *= (mod(pos.x,  0.125) <= 1/chunkSize.x ? 0.0 : 1.0);
    // FragColor.r *= (mod(pos.y, 0.0625) <= 1/chunkSize.y ? 0.0 : 1.0);
}

/* 2d height-map example */
// void    main() {
//     vec2 uv = vec2(TexCoords.x, 1.0 - TexCoords.y);
//     vec2 c_uv = uv - 1. / (chunkSize.xz * 2.0);
//     vec3 chunk = chunkPosition / chunkSize;
//     vec2 worldPos2d = chunk.xz - 1.0 + c_uv.xy;
//     FragColor.r = fbm2d(worldPos2d, 0.5, 0.7, 4, 1.5, 0.5);
// }
