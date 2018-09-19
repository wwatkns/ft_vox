#include "Chunk.hpp"
#include "glm/ext.hpp"

Chunk::Chunk( std::vector<tPoint> voxels, const glm::vec3& position, const glm::vec3& size ) : voxels(voxels), position(position), size(size) {
    this->createModelTransform(position);
    this->setup(GL_STATIC_DRAW);
}

Chunk::~Chunk( void ) {
    glDeleteBuffers(1, &this->vao);
    glDeleteBuffers(1, &this->vbo);
}

void    Chunk::render( Shader shader, Camera& camera ) {
    /* view fustrum optimisation (don't render chunk not in view fustrum) */
    // if (camera.aabInFustrum(-(this->position + this->size / 2), this->size)) {
    if (camera.sphereInFustrum(-(this->position + this->size / 2), this->size.x * 0.7071067812)) {
        /* set transform matrix */
        shader.setMat4UniformValue("model", this->transform);
        /* render */
        glBindVertexArray(this->vao);
        glDrawArrays(GL_POINTS, 0, this->voxels.size());
        glBindVertexArray(0);
    }
}

void    Chunk::setup( int mode ) {
	glGenVertexArrays(1, &this->vao);
    glGenBuffers(1, &this->vbo);
	glBindVertexArray(this->vao);
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glBufferData(GL_ARRAY_BUFFER, this->voxels.size() * sizeof(tPoint), this->voxels.data(), mode);

    // position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(tPoint), static_cast<GLvoid*>(0));
    // id attribute
    glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(tPoint), reinterpret_cast<GLvoid*>(offsetof(tPoint, id)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void    Chunk::createModelTransform( const glm::vec3& position ) {
    this->transform = glm::mat4();
    this->transform = glm::translate(this->transform, position);
}