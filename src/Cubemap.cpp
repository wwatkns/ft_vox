#include "Cubemap.hpp"
#include "glm/ext.hpp"

Cubemap::Cubemap( const std::vector<std::string>& paths ) {
    /* load textures */
    textures.push_back(loadCubemapSrgb(paths));
    /* create cube mesh */
    std::vector<float>  v;
    createCube(v, this->indices);
    for (size_t i = 5; i < v.size()+1; i += 5)
        this->vertices.push_back( glm::vec3(v[i-5], v[i-4], v[i-3]) );

    this->setup(GL_STATIC_DRAW);
}

Cubemap::~Cubemap( void ) {
    glDeleteTextures(1, &this->textures[0]);
    glDeleteVertexArrays(1, &this->vao);
    glDeleteBuffers(1, &this->vbo);
    glDeleteBuffers(1, &this->ebo);
}

void    Cubemap::render( Shader shader ) {
    /* activate texture */
    glActiveTexture(GL_TEXTURE0);
    shader.setIntUniformValue("skybox", 0);
    glBindTexture(GL_TEXTURE_2D, this->textures[0]);

    glBindVertexArray(this->vao);
    glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void    Cubemap::update( void ) {
}

void    Cubemap::setup( int mode ) {
	glGenVertexArrays(1, &this->vao);
    glGenBuffers(1, &this->vbo);
	glGenBuffers(1, &this->ebo);
	glBindVertexArray(this->vao);
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(glm::vec3), this->vertices.data(), mode);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(unsigned int), this->indices.data(), mode);
    /* position attribute */
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), static_cast<GLvoid*>(0));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
