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
#define GRASS 2/255.
#define STONE 3/255.
#define BEDROCK 4/255.
#define COAL 5/255.
#define IRON 6/255.
#define GOLD 7/255.
#define LAPIS 8/255.
#define REDSTONE 9/255.
#define DIAMOND 10/255.
#define GRAVEL 11/255.
#define SAND 12/255.

float   map(vec3 p) {
    /* TODO : implement biomes with voronoi cells */
    float res;
    /* bedrock level */
    if (p.y == 0 || fbm3d(p, 1.0, 20.0, 2, 1.5, 0.5) > p.y/3.)
        return BEDROCK;
    /* terrain and caves */
    int g0 = int(fbm3d(p, 0.5, 0.01, 6, 1.7, 0.5) > p.y / 255.);            /*  low-frequency landscape */
    int g1 = int(fbm3d(p, 0.5, 0.025, 4, 1.5, 0.5) > 0.45 * (p.y / 128.));  /* high-frequency landscape */
    // int g2 = int(fbm3d(p, 0.35, 0.12, 4, 1.0, 0.35) > 0.1);                 /* cave system (TMP, should implement more complex algorithm) */
    int g2 = int(fbm3d(p, 2.5, 0.15, 3, 1.0, 0.01) > 0.6);                 /* cave system, low-freq */
    int g13 = int(fbm3d(p * vec3(1,1.4,1), 3., 0.08, 5, 5.0, 0.05) > 0.6);                 /* cave system, high-freq */
    /* resource distribution */
    int g3 = int(fbm3d(p+340., 0.29, 0.20, 4, 1.5, 0.37) < 0.1 && p.y < 130);/* coal */
    int g4 = int(fbm3d(p-100., 0.37, 0.30, 4, 1.8, 0.30) < 0.1 && p.y < 64); /* iron */
    int g10= int(fbm3d(p+100., 0.37, 0.35, 4, 1.2, 0.33) < 0.1 && p.y < 32); /* gold */
    int g11= int(fbm3d(p-230., 0.35, 0.30, 4, 1.2, 0.33) < 0.1 && p.y < 32); /* lapis */
    int g12= int(fbm3d(p-160., 0.30, 0.35, 4, 1.2, 0.33) < 0.1 && p.y < 16); /* redstone */
    int g5 = int(fbm3d(p+100., 0.40, 0.20, 4, 1.5, 0.38) < 0.1 && p.y < 16); /* diamond */
    /* stone (we use the same values for fbm as landscape but with a vertical offset) */
    int g7 = int(fbm3d(p+vec3(0,20,0), 0.5, 0.01, 5, 1.7, 0.5) > p.y / 255.);        /*  low-frequency landscape */
    g7 &= int(fbm3d(p+vec3(0,20,0), 0.5, 0.025, 3, 1.5, 0.5) > 0.45 * (p.y / 128.)); /* high-frequency landscape */
    g7 &= int(fbm3d(p+vec3(0,5,0), 0.5, 0.01, 5, 1.7, 0.5) > p.y / 255.);            /*  low-frequency landscape */
    g7 &= int(fbm3d(p+vec3(0,5,0), 0.5, 0.025, 3, 1.5, 0.5) > 0.45 * (p.y / 128.));  /* high-frequency landscape */
    /* pockets of dirt and gravel in undergrounds */
    int g8 = int(fbm3d(p+40, 0.35, 0.18, 4, 1.0, 0.2) < 0.05 + (1.0-p.y/96.)*0.05);
    int g9 = int(fbm3d(p-70, 0.48, 0.14, 4, 1.0, 0.2) < 0.05 + (1.0-p.y/200.)*0.05);

    res = float(g0 & g1 & g2 & g13) * DIRT;

    res = (res == DIRT  && g7 == 1 ? STONE : res );
    res = (res == STONE && g5 == 1 ? DIAMOND : res );
    res = (res == STONE && g12 == 1 ? REDSTONE : res );
    res = (res == STONE && g11 == 1 ? LAPIS : res );
    res = (res == STONE && g10 == 1 ? GOLD : res );
    res = (res == STONE && g4 == 1 ? IRON : res );
    res = (res == STONE && g3 == 1 ? COAL : res );
    res = (res == STONE && g8 == 1 ? DIRT : res );
    res = (res == STONE && g9 == 1 ? GRAVEL : res );

    // res = (g2 & g13) * COAL; // tmp
    // res = g4 * IRON; // tmp
    // res = g5 * DIAMOND; // tmp
    // res = g13 * SAND;
    return res;
}

/* _______________________________________________________
  coordinates:   0                             31       37
  uv:            0                           0.8421     1
                 [-----------------------------|--------]
  corrected uv:  0                             1      1.1875  ->  TexCoords.y * (38/32)
*/

/* 3d volume texture */
void    main() {
    // int extraHeight = 6; //! pass as uniform
    // float correction = (chunkSize.z + extraHeight) / chunkSize.z;
    vec2 uv = vec2(TexCoords.x, (1.0 - TexCoords.y));// * correction);
    vec3 worldPos;
    // if (uv.y < 1.0) { /* normal chunk generation */
        vec2 c_uv = floor(uv * chunkSize.xy) / (chunkSize.xy - 1.);
        float z = mod(floor(uv.y * chunkSize.y * chunkSize.z), chunkSize.z) / (chunkSize.z - 1.);
        vec3 pos = vec3(c_uv, z);
        worldPos = (chunkPosition + pos * chunkSize);

    // }
    // else { /* generate extra informations on faces around chunks */
    //     vec3 pos;
    //     /* split the uv.y upper part (above from 1 to 1.1875) in 6 parts */
    //     float faceUvMax = 1.0 + floor((uv.y - 1.) * chunkSize.x + 1.) / chunkSize.x;
    //     float uvy = 1.0 - (faceUvMax - uv.y) * chunkSize.y;
    //     vec2 c_uv = floor(vec2(uv.x, uvy) * chunkSize.xy) / (chunkSize.xy - 1.);

    //     int face = int((faceUvMax-1.0) * chunkSize.x) - 1;
    //     // switch (face) {
    //     //     case 0: pos = vec3(c_uv.x, 1.0, c_uv.y); break;
    //     //     case 1: pos = vec3(c_uv.x, 0.0, c_uv.y); break;
    //     //     case 2: pos = vec3(1.0, c_uv.yx); break;
    //     //     case 3: pos = vec3(0.0, c_uv.yx); break;
    //     //     case 4: pos = vec3(c_uv.xy, 1.0); break;
    //     //     case 5: pos = vec3(c_uv.xy, 0.0); break;
    //     // }
    //     // if (face == 0) { /* top */
    //     //     pos = vec3(c_uv.x, 1.0, c_uv.y);
    //     // }
    //     // else if (face == 1) { /* bottom */
    //     //     pos = vec3(c_uv.x, 0.0, c_uv.y);
    //     // }
    //     // else if (face == 2) { /* right */
    //     //     pos = vec3(1.0, c_uv.yx);
    //     // }
    //     // else if (face == 3) { /* left */
    //     //     pos = vec3(0.0, c_uv.yx);
    //     // }
    //     // else if (face == 4) { /* front */
    //     //     pos = vec3(c_uv.xy, 1.0);
    //     // }
    //     // else { /* back */
    //     //     pos = vec3(c_uv.xy, 0.0);
    //     // }
    //     // worldPos = (chunkPosition + pos * chunkSize);

    //     switch (face) {
    //         case 0: worldPos = (chunkPosition + vec3(c_uv.x, 1.0, c_uv.y) * chunkSize) + vec3(0, 1, 0); break; // top
    //         case 1: worldPos = (chunkPosition + vec3(c_uv.x, 0.0, c_uv.y) * chunkSize) - vec3(0, 1, 0); break; // bottom
    //         case 2: worldPos = (chunkPosition + vec3(1.0, c_uv.yx) * chunkSize) + vec3(1, 0, 0); break; // right
    //         case 3: worldPos = (chunkPosition + vec3(0.0, c_uv.yx) * chunkSize) - vec3(1, 0, 0); break; // left
    //         case 4: worldPos = (chunkPosition + vec3(c_uv.xy, 1.0) * chunkSize) + vec3(0, 0, 1); break; // front
    //         case 5: worldPos = (chunkPosition + vec3(c_uv.xy, 0.0) * chunkSize) - vec3(0, 0, 1); break; // back
    //     }
    // }
    FragColor.r = sqrt(map(worldPos)); // values from [0..255] (0..1) are in normalized fixed-point representation, a simple sqrt() fixes that.

    // FragColor.r = sqrt(int(sin(worldPos.y/16.+0.5*PI)*sin(worldPos.x/16.+0.5*PI)*0.5+0.5 < worldPos.z/32.) * STONE); // Z
    // FragColor.r = sqrt(int(sin(worldPos.y/16.+0.5*PI)*sin(worldPos.z/16.+0.5*PI)*0.5+0.5 < worldPos.x/32.) * STONE); // X
    // FragColor.r = sqrt(int(sin(worldPos.x/16.+0.5*PI)*sin(worldPos.z/16.+0.5*PI)*0.5+0.5 > worldPos.y/32.) * STONE); // Y
}

/* 2d height-map example */
// void    main() {
//     vec2 uv = vec2(TexCoords.x, 1.0 - TexCoords.y);
//     vec2 c_uv = uv - 1. / (chunkSize.xz * 2.0);
//     vec3 chunk = chunkPosition / chunkSize;
//     vec2 worldPos2d = chunk.xz - 1.0 + c_uv.xy;
//     FragColor.r = fbm2d(worldPos2d, 0.5, 0.7, 4, 1.5, 0.5);
// }
