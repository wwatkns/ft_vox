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

class Cubemap {

public:
    Cubemap( const std::vector<std::string>& paths );
    ~Cubemap( void );

    void            update( void );
    void            render( Shader shader );

    const std::vector<GLuint> getTextures( void ) const { return (textures); };

private:
    GLuint                      vao;
    GLuint                      vbo;
    GLuint                      ebo;
    std::vector<glm::vec3>      vertices;
    std::vector<unsigned int>   indices;
    std::vector<GLuint>         textures;

    void            setup( int mode );
};
