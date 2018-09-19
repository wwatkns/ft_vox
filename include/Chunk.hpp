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
    Chunk( std::vector<tPoint> voxels, const glm::vec3& position, const glm::vec3& size );
    ~Chunk( void );

    void                render( Shader shader, Camera& camera );
    /* getters */
    const GLuint&       getVao( void ) const { return (vao); };
    const GLuint&       getVbo( void ) const { return (vbo); };

private:
    GLuint              vao;        // Vertex Array Object
    GLuint              vbo;        // Vertex Buffer Object
    std::vector<tPoint> voxels;     // the list of voxels created in Terrain
    glm::mat4           transform;  // the transform of the chunk (its world position)
    glm::vec3           position;
    glm::vec3           size;

    void                setup( int mode );
    void                createModelTransform( const glm::vec3& position );

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