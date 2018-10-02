#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <array>
#include <forward_list>
#include <unordered_map>
#include <map>
#include <queue>
#include <set>
#include <unordered_set>

#include "Exception.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "utils.hpp"
#include "Chunk.hpp"

typedef struct  sVertex {
    glm::vec3   Position;
    glm::vec2   TexCoords;
}               tVertex;

typedef struct  sMesh {
    GLuint  vao;
    GLuint  vbo;
    GLuint  ebo;
}               tMesh;

typedef struct  sRenderBuffer {
    unsigned int    id;
    unsigned int    fbo;
    size_t          width;
    size_t          height;
}               tRenderBuffer;

struct  Key {
    glm::vec3   p;

    bool operator==(const Key &other) const {
        return (p.x == other.p.x && p.y == other.p.y && p.z == other.p.z);
    }
};

struct KeyHash {
    /* /!\ positions are stored as 16 bits integers, so world position should not go further than 2^15 = 32768 chunks in any direction, or 1,048,576 blocks
    */
    uint64_t operator()(const Key &k) const {
        uint64_t    hash = 0;
        hash |= static_cast<uint64_t>(static_cast<int>(k.p.x) + 0x7FFF);
        hash |= static_cast<uint64_t>(static_cast<int>(k.p.y) + 0x7FFF) << 16;
        hash |= static_cast<uint64_t>(static_cast<int>(k.p.z) + 0x7FFF) << 32;
        return (hash);
    }
};

struct  setChunkRenderCompare {
    bool operator()(const glm::vec4& a, const glm::vec4& b) const {
        return (a.w < b.w);
    }
};
/*
    glm::vec4(position, distance on xz plane)
    return (dist < b.dist || (dist == b.dist && pos.y > b.pos.y) );
*/

class Terrain {

public:
    Terrain( uint renderDistance = 160, uint maxHeight = 256 );
    ~Terrain( void );

    void                        updateChunks( const glm::vec3& cameraPosition );
    void                        renderChunks( Shader shader, Camera& camera );

    void                        addChunksToGenerationList( const glm::vec3& cameraPosition );
    void                        generateChunkTextures( void );
    void                        generateChunkMeshes( void );
    void                        computeChunkLight( void );
    void                        deleteChunk( void );
    glm::vec3                   getChunkPosition( const glm::vec3& position );

private:
    std::unordered_map<Key, Chunk*, KeyHash>    chunks;
    std::set<glm::vec4, setChunkRenderCompare>  chunksToGenerate;
    // std::unordered_set<glm::vec4>               chunksToGenerate; /* we need a map to be able to generate the chunks in order from top to bottom */
    // std::priority_queue<uint64_t>       chunksToGenerate; /* we need a map to be able to generate the chunks in order from top to bottom */
    int                                         maxChunksGeneratedPerFrame;
    glm::ivec3                  chunkSize;
    uint                        renderDistance; /* in blocs */
    uint                        maxHeight;
    Shader*                     chunkGenerationShader;
    tMesh                       chunkGenerationRenderingQuad;
    tRenderBuffer               chunkGenerationFbo;
    GLuint                      noiseSampler;
    GLuint                      textureAtlas;
    uint8_t*                    dataBuffer;
    uint                        dataMargin;

    void                        setupChunkGenerationRenderingQuad( void );
    void                        setupChunkGenerationFbo( void );
    void                        renderChunkGeneration( const glm::vec3& position, uint8_t* data );
};
