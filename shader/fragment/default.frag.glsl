#version 400 core
out vec4 FragColor;

// the direction is always from the position to the center of the scene
struct sDirectionalLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct sMaterial {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    float opacity;
};

struct sOffset {
    vec2 top;
    vec2 bottom;
    vec2 side;
};

/*  32bits, 5bits per offset component
    we can have top x/y, then side x/y, and bottom x/y
    [______top______|_____side______|____bottom_____]
    [   x   |   y   |   x   |   y   |   x   |   y   ]..]
    top_x    = (offset & 0xF8000000) >> 27;
    top_y    = (offset & 0x7C00000) >> 22;
    side_x   = (offset & 0x3E0000) >> 17;
    side_y   = (offset & 0x1F000) >> 12;
    bottom_x = (offset & 0xF80) >> 7;
    bottom_y = (offset & 0x7C) >> 2;
*/


/* input variables */
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
flat in int Id;
// in vec4 FragPosLightSpace;

// #define MAX_POINT_LIGHTS 8

/* uniforms */
uniform sampler2D atlas;
uniform vec3 cameraPos;
uniform sDirectionalLight directionalLight;
// uniform sPointLight pointLights[MAX_POINT_LIGHTS];
// uniform int nPointLights;

sMaterial material = sMaterial(
    vec3(0.4),
    vec3(1.0),
    vec3(0.0),
    0.0,
    1.0
);

/* the offsets for the bloc textures (top, bottom, side) */
const sOffset offsets[12] = sOffset[](
    sOffset( vec2(2, 0), vec2(2, 0), vec2(2, 0) ), // 0: dirt
    sOffset( vec2(0, 0), vec2(2, 0), vec2(3, 0) ), // 1: grass
    sOffset( vec2(1, 0), vec2(1, 0), vec2(1, 0) ), // 2: stone
    sOffset( vec2(1, 1), vec2(1, 1), vec2(1, 1) ), // 3: bedrock
    sOffset( vec2(2, 2), vec2(2, 2), vec2(2, 2) ), // 4: coal ore
    sOffset( vec2(1, 2), vec2(1, 2), vec2(1, 2) ), // 5: iron ore
    sOffset( vec2(0, 2), vec2(0, 2), vec2(0, 2) ), // 6: gold ore
    sOffset( vec2(0,10), vec2(0,10), vec2(0,10) ), // 7: lapis ore
    sOffset( vec2(3, 3), vec2(3, 3), vec2(3, 3) ), // 8: redstone ore
    sOffset( vec2(2, 3), vec2(2, 3), vec2(2, 3) ), // 9: diamond ore
    sOffset( vec2(3, 1), vec2(3, 1), vec2(3, 1) ), //10: gravel
    sOffset( vec2(2, 1), vec2(2, 1), vec2(2, 1) )  //11: sand
);
const vec2 atlasSize = vec2(24, 42);

/* prototypes */
vec3    computeDirectionalLight( sDirectionalLight light, vec3 normal, vec3 viewDir, vec4 fragPosLightSpace );

vec4    getBlocTexture( void ) {
    vec2 offset = (Normal.y == 0 ? offsets[Id].side : (Normal.y == 1 ? offsets[Id].top : offsets[Id].bottom));
    return texture(atlas, (offset + TexCoords) / atlasSize);
}

void main() {
    vec3 viewDir = normalize(cameraPos - FragPos);

    vec3 result = computeDirectionalLight(directionalLight, Normal, viewDir, vec4(0.0));

    // FragColor = vec4(result, material.opacity);
    // if (mod(FragPos.x, 32.0)*mod(FragPos.y, 32.0)*mod(FragPos.z, 32.0) <= 0.5) {
    //     FragColor = vec4(vec3(0.0, 1.0, 0.2), 1.);
    //     return;
    // }
    // FragColor = vec4((Normal * 0.5 + 0.4) * vec3(1., 0.5, 0.5), 1.0);
    FragColor = vec4(getBlocTexture().xyz * result, 1.0);
}

vec3 computeDirectionalLight( sDirectionalLight light, vec3 normal, vec3 viewDir, vec4 fragPosLightSpace ) {
    vec3 lightDir = normalize(light.position);
    /* diffuse */
    float diff = max(dot(normal, lightDir), 0.0);
    /* specular */
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    /* compute terms */
    vec3 ambient  = light.ambient  * material.diffuse;
    vec3 diffuse  = light.diffuse  * diff * material.diffuse;
    vec3 specular = light.specular * spec * material.specular;
    return (material.ambient + ambient + (diffuse + specular));
}

// vec3 computePointLight( sPointLight light, vec3 normal, vec3 fragPos, vec3 viewDir ) {
//     vec3 lightDir = normalize(light.position - fragPos);
//     /* diffuse */
//     float diff = max(dot(normal, lightDir), 0.0);
//     /* specular */
//     vec3 reflectDir = reflect(-lightDir, normal);
//     float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
//     /* attenuation */
//     float dist = length(light.position - fragPos);
//     float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * (dist * dist));
//     /* compute terms */
//     vec3 ambient  = light.ambient  * gDiffuse * attenuation;
//     vec3 diffuse  = light.diffuse  * diff * gDiffuse * attenuation;
//     vec3 specular = light.specular * spec * gSpecular * attenuation;

//     return (gEmissive + ambient + diffuse + specular);
// }

// float computeShadows( vec4 fragPosLightSpace ) {
//     vec3 projCoords = (fragPosLightSpace.xyz / fragPosLightSpace.w) * 0.5 + 0.5;
//     float currentDepth = projCoords.z;
//     float bias = 0.0025;
//     /* PCF */
//     float shadow = 0.0;
//     vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
//     for (int x = -1; x <= 1; ++x) {
//         for (int y = -1; y <= 1; ++y) {
//             float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
//             shadow += (currentDepth - bias > pcfDepth ? 1.0 : 0.0);
//         }
//     }
//     return (projCoords.z > 1.0 ? 0.0 : shadow / 9.0);
// }
