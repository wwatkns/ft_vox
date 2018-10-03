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
    ivec2 top;
    ivec2 bottom;
    ivec2 side;
};

/* input variables */
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
flat in int Id;
in float Ao;
in float Light;

/* uniforms */
uniform sampler2D atlas;
uniform vec3 cameraPos;
uniform sDirectionalLight directionalLight;
uniform int underwater; // NEW

sMaterial material = sMaterial(
    vec3(0.4),
    vec3(1.0),
    vec3(0.0),
    0.0,
    1.0
);

/* the offsets for the bloc textures (top, bottom, side) */
const sOffset offsets[15] = sOffset[](
    sOffset( ivec2(2, 0), ivec2(2, 0), ivec2(2, 0) ), // 0: dirt
    sOffset( ivec2(0, 0), ivec2(2, 0), ivec2(3, 0) ), // 1: grass
    sOffset( ivec2(1, 0), ivec2(1, 0), ivec2(1, 0) ), // 2: stone
    sOffset( ivec2(1, 1), ivec2(1, 1), ivec2(1, 1) ), // 3: bedrock
    sOffset( ivec2(2, 2), ivec2(2, 2), ivec2(2, 2) ), // 4: coal ore
    sOffset( ivec2(1, 2), ivec2(1, 2), ivec2(1, 2) ), // 5: iron ore
    sOffset( ivec2(0, 2), ivec2(0, 2), ivec2(0, 2) ), // 6: gold ore
    sOffset( ivec2(0,10), ivec2(0,10), ivec2(0,10) ), // 7: lapis ore
    sOffset( ivec2(3, 3), ivec2(3, 3), ivec2(3, 3) ), // 8: redstone ore
    sOffset( ivec2(2, 3), ivec2(2, 3), ivec2(2, 3) ), // 9: diamond ore
    sOffset( ivec2(3, 1), ivec2(3, 1), ivec2(3, 1) ), //10: gravel
    sOffset( ivec2(2, 1), ivec2(2, 1), ivec2(2, 1) ), //11: sand
    sOffset( ivec2(5, 1), ivec2(5, 1), ivec2(4, 1) ), //12: oak wood
    sOffset( ivec2(5, 3), ivec2(5, 3), ivec2(5, 3) ), //13: oak leaves
    sOffset( ivec2(4,33), ivec2(4,33), ivec2(4,33) )  //14: water
);
const ivec2 atlasSize = ivec2(24, 42);

/* prototypes */
vec3    computeDirectionalLight( sDirectionalLight light, vec3 normal, vec3 viewDir, vec4 fragPosLightSpace );

vec4    getBlocTextureNoSeams( void ) {
    /* fixes visual seams but introduces visual issue with lod selection (we don't have derivatives) */
    ivec2 offset = (Normal.y == 0 ? offsets[Id].side : (Normal.y == 1 ? offsets[Id].top : offsets[Id].bottom));
    int lod = int(textureQueryLod(atlas, TexCoords/20).x);
    ivec2 p = ivec2(offset*(16>>lod) + ivec2(floor(TexCoords*(16>>lod))));
    return texelFetch(atlas, p, lod);
}

vec4    getBlocTexture( void ) {
    /* exhibits visual seams because of rounding errors */
    vec2 offset = (Normal.y == 0 ? offsets[Id].side : (Normal.y == 1 ? offsets[Id].top : offsets[Id].bottom));
    return texture(atlas, (offset + TexCoords) / atlasSize);
}

void main() {
    vec3 viewDir = normalize(cameraPos - FragPos);
    vec3 result = computeDirectionalLight(directionalLight, Normal, viewDir, vec4(0.0));
    FragColor = vec4(getBlocTexture().xyz * Ao * (Light*0.8+0.2), (Id == 14 ? 0.65 : 1.0) );
    // FragColor = vec4(getBlocTextureNoSeams().xyz * Ao * (Light*0.8+0.2), 1.0);

    if (underwater == 1)
        FragColor = FragColor * vec4(0.75, 0.75, 2., 1) * vec4(vec3(5. / distance(cameraPos, FragPos)), 1);
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
