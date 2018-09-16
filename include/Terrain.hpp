#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "Exception.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "utils.hpp"
#include "Chunk.hpp"

/*
    Handles the logic for the chunks rendering/creating/deletion
*/

class Terrain {

public:
    Terrain( uint8_t renderDistance = 10, const glm::vec3& chunkSize = glm::vec3(16, 128, 16) );
    ~Terrain( void );

    void            update( void );
    void            render( Shader shader );
    void            generateChunk( const glm::vec3& position );
    void            deleteChunk( void );

    /* getters */
    const std::vector<Chunk*>   getChunks( void ) const { return (chunks); };

private:
    std::vector<Chunk*>         chunks;
    glm::vec3                   chunkSize;
    uint8_t                     renderDistance; /* in chunks */

};
