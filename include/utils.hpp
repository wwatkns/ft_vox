#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "Exception.hpp"

void        createCube( std::vector<GLfloat>& vertices, std::vector<unsigned int>& indices );
glm::vec4   hex2vec( int64_t hex );
glm::vec2   mousePosToClipSpace( const glm::dvec2& pos, int winWidth, int winHeight );
GLuint      loadTexture( const char* path, GLuint textureMode = GL_LINEAR );
GLuint      loadTextureSrgb( const char* path, GLuint textureMode = GL_LINEAR );
GLuint      loadTextureMipmapSrgb( const std::vector<std::string>& paths );
GLuint      loadCubemap( const std::vector<std::string>& paths );
GLuint      loadCubemapSrgb( const std::vector<std::string>& paths );