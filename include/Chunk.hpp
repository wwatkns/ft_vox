#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <array>

#include "Exception.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "utils.hpp"

typedef struct  sPoint {
    glm::vec3   position;
    uint8_t     id;
}               tPoint;

class Chunk {

public:
    // Chunk( std::vector<tPoint> voxels, const glm::vec3& position, const glm::vec3& size, const uint8_t* texture );
    Chunk( const glm::vec3& position, const glm::ivec3& size, const uint8_t* texture );
    ~Chunk( void );

    void                buildMesh( const std::array<const uint8_t*, 6>& adjacentChunks );
    void                render( Shader shader, Camera& camera );
    /* getters */
    const GLuint&       getVao( void ) const { return vao; };
    const GLuint&       getVbo( void ) const { return vbo; };
    const glm::vec3&    getPosition( void ) const { return position; };
    const uint8_t*      getTexture( void ) const { return texture; };

private:
    GLuint              vao;        // Vertex Array Object
    GLuint              vbo;        // Vertex Buffer Object
    std::vector<tPoint> voxels;     // the list of voxels created in Terrain
    glm::mat4           transform;  // the transform of the chunk (its world position)
    glm::vec3           position;
    glm::ivec3          size;
    uint8_t*            texture;    /* the texture outputed by the chunk generation shader */

    void                setup( int mode );
    void                createModelTransform( const glm::vec3& position );
    bool                isVoxelCulled( int x, int y, int z, int i, const std::array<const uint8_t*, 6>& adjacentChunks );

};

/*
    - A chunk is a mesh of the voxels in that volume, so one VBO and one VAO.
    - So for creating or updating a chunk, we must :
        * delete the current Chunk
        * create a new mesh
        * create a new chunk with that mesh
        * bind VBO/VAO...
    - 
*/