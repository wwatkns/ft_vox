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

typedef struct  sQuadVertex {
    glm::vec3   Position;
    glm::vec2   TexCoords;
}               tQuadVertex;

class PostProcess {

public:
    PostProcess( void );
    ~PostProcess( void );

    void                        render( Shader shader, GLuint tex );

private:
    std::vector<unsigned int>   indices;
    std::vector<tQuadVertex>    vertices;
    GLuint                      vao;
    GLuint                      vbo;
    GLuint                      ebo;

    void                        setup( int mode );
    void                        createRenderQuad( void );

};
