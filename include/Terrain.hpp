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
#include <array>
#include <unordered_map>

#include "Exception.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "utils.hpp"
#include "Chunk.hpp"

/*
    Handles the logic for the chunks rendering/creating/deletion
*/

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
    uint64_t operator()(const Key &k) const {
        uint64_t    hash = 0;
        hash |= (int)k.p.x;
        hash |= (int)k.p.y << 8;
        hash |= (int)k.p.z << 16;
        return (hash);
    }
};

class Terrain {

public:
    Terrain( uint8_t renderDistance = 10, uint maxHeight = 256, const glm::ivec3& chunkSize = glm::ivec3(32, 32, 32) );
    ~Terrain( void );

    void                        update( void );
    void                        render( Shader shader, Camera& camera );

    void                        generateChunkTextures( void );
    void                        generateChunkMeshes( void );
    void                        deleteChunk( void );

    /* getters */
    // const std::vector<Chunk*>   getChunks( void ) const { return (chunks); };

private:
    /* TODO: store chunks in an unordered_map with key describing xyz position of chunk
             that way we can check easily if a chunk for a given position is already loaded,
             and we have a O(1), worst O(log n) lookup time
    */
    std::unordered_map<Key, Chunk*, KeyHash>   chunks;
    // std::vector<Chunk*>         chunks;
    glm::ivec3                  chunkSize;
    uint8_t                     renderDistance; /* in chunks */
    uint                        maxHeight;
    Shader*                     chunkGenerationShader;
    tMesh                       chunkGenerationRenderingQuad;
    tRenderBuffer               chunkGenerationFbo;
    GLuint                      noiseSampler;
    uint8_t*                    dataBuffer;

    void                        setupChunkGenerationRenderingQuad( void );
    void                        setupChunkGenerationFbo( void );
    void                        renderChunkGeneration( const glm::vec3& position, uint8_t* data );
};

/*  chunk_volume * number_of_chunks
    32*32*32*12*12*8 / 1000000
    37.74 Mo

    So we have a max number of chunks stored in RAM, let's arbitrarily set that number to 2048 (16*16*8 chunks), that's ~67 Mo of RAM.
    Now, we build all the needed chunks around us in world no further than `renderDistance`.

    We populate the loaded chunks lists until we reach maximum capacity. In that case, we remove the elements that are the most distant to
    the player (we could have a tree as data structure to keep it fast) and insert the new loaded chunk.

    We could also perform a culling check for chunks to generate around player, so that we don't generate them even if they are in
    renderDistance radius if they're occluded.
*/