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
#include <unordered_set>
#include <queue>

#include "Exception.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "utils.hpp"
#include "Chunk.hpp"

typedef struct  vertex_s {
    glm::vec3   Position;
    glm::vec2   TexCoords;
}               vertex_t;

typedef struct  mesh_quad_s {
    GLuint  vao;
    GLuint  vbo;
    GLuint  ebo;
}               mesh_quad_t;

typedef struct  framebuffer_s {
    unsigned int    id;
    unsigned int    fbo;
    size_t          width;
    size_t          height;
}               framebuffer_t;

/* structure and compare function for rendering chunks */
typedef struct  chunkSort_s {
    Chunk*  chunk;
    float   comp;
}               chunkSort_t;

struct {
    bool operator()(chunkSort_t a, chunkSort_t b) const {   
        return a.comp > b.comp;
    }
} chunkRenderingCompareSort;

/* key and keyhash for chunks storing (unordered_map is good for storage of chunks) */
struct ckey_t {
    glm::vec3   p;
    bool operator==(const ckey_t &other) const {
        return (p.x == other.p.x && p.y == other.p.y && p.z == other.p.z);
    }
};

struct KeyHash {
    /* /!\ positions are stored as 16 bits integers, so world position should not go further than 2^15 = 32768 chunks in any direction, or 1,048,576 blocks */
    uint64_t operator()(const ckey_t &k) const {
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

enum class updateType { water, light };

typedef struct  update_s {
    glm::vec3   chunk;
    glm::vec3   from;
    updateType  action;
}               update_t;

class Terrain {

public:
    Terrain( uint renderDistance = 160, uint maxHeight = 256 );
    ~Terrain( void );

    void                        updateChunks( const glm::vec3& cameraPosition );
    void                        renderChunks( Shader shader, Camera& camera );
    void                        deleteOutOfRangeChunks( void );

    void                        addChunksToGenerationList( const glm::vec3& cameraPosition );
    void                        generateChunkTextures( void );
    void                        generateChunkMeshes( void );
    void                        computeChunkLight( void );

    const glm::vec3                   getChunkPosition( const glm::vec3& position ) const;
    const std::array<Chunk*, 6>       getNeighbouringChunks( const glm::vec3& position ) const;

private:
    std::unordered_map<ckey_t, Chunk*, KeyHash> chunks;
    std::unordered_set<ckey_t, KeyHash>         chunksToLoadSet; // need to to easy check if chunk is present in queue
    std::queue<ckey_t>                          chunksToLoadQueue; // queue to have ordered chunk lookup
    std::queue<update_t>                        chunksToUpdateQueue;

    float                       maxAllocatedTimePerFrame;
    glm::ivec3                  chunkSize;
    uint                        renderDistance; /* in blocs */
    uint                        maxHeight;
    Shader*                     chunkGenerationShader;
    mesh_quad_t                 chunkGenerationRenderingQuad;
    framebuffer_t               chunkGenerationFbo;
    GLuint                      noiseSampler;
    GLuint                      textureAtlas;
    uint8_t*                    dataBuffer;
    uint                        dataMargin;

    void                        setupChunkGenerationRenderingQuad( void );
    void                        setupChunkGenerationFbo( void );
    void                        renderChunkGeneration( const glm::vec3& position );
};
