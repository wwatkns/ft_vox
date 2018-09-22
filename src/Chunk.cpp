#include "Chunk.hpp"
#include "glm/ext.hpp"

Chunk::Chunk( const glm::vec3& position, const glm::ivec3& size, const uint8_t* texture ) : position(position), size(size), meshed(false), outOfRange(false) {
    this->createModelTransform(position);
    this->texture = static_cast<uint8_t*>(malloc(sizeof(uint8_t) * size.x * size.y * size.z));
    memcpy(this->texture, texture, size.x * size.y * size.z);
}

Chunk::~Chunk( void ) {
    free(this->texture);
    this->texture = NULL;
    glDeleteBuffers(1, &this->vao);
    glDeleteBuffers(1, &this->vbo);
}

uint8_t Chunk::getVisibleFaces( int x, int y, int z, int i, const std::array<const uint8_t*, 6>& adjacentChunks ) {
    uint8_t faces = 0x0;
    faces |=  (x + 1 < size.x && this->texture[i + 1              ] == 0) << 5; // right
    faces |=  (x - 1 >= 0     && this->texture[i - 1              ] == 0) << 4; // left
    faces |=  (z + 1 < size.z && this->texture[i + size.x         ] == 0) << 3; // front
    faces |=  (z - 1 >= 0     && this->texture[i - size.x         ] == 0) << 2; // back
    faces |= ((y + 1 < size.y && this->texture[i + size.x * size.z] == 0) || (y == size.y - 1 && adjacentChunks[5] == nullptr) ) << 1; // up
    faces |= ((y - 1 >= 0     && this->texture[i - size.x * size.z] == 0) || (y == 0          && adjacentChunks[4] == nullptr) );      // down
    return faces;
}

bool    Chunk::isVoxelCulled( int x, int y, int z, int i, const std::array<const uint8_t*, 6>& adjacentChunks ) {
    uint8_t b = 0x1;
    /* empty with borders on world extremities */
    b &= (adjacentChunks[0] != nullptr && x == 0          ? (adjacentChunks[0][i + (size.x - 1)                  ] != 0) : (adjacentChunks[0] == nullptr && x == 0          ? 0 : (texture[i - 1] != 0)));
    b &= (adjacentChunks[1] != nullptr && x == size.x - 1 ? (adjacentChunks[1][i - (size.x - 1)                  ] != 0) : (adjacentChunks[1] == nullptr && x == size.x - 1 ? 0 : (texture[i + 1] != 0)));
    b &= (adjacentChunks[2] != nullptr && z == 0          ? (adjacentChunks[2][i + (size.z - 1) * size.x         ] != 0) : (adjacentChunks[2] == nullptr && z == 0          ? 0 : (texture[i - size.x] != 0)));
    b &= (adjacentChunks[3] != nullptr && z == size.z - 1 ? (adjacentChunks[3][i - (size.z - 1) * size.x         ] != 0) : (adjacentChunks[3] == nullptr && z == size.z - 1 ? 0 : (texture[i + size.x] != 0)));
    b &= (adjacentChunks[4] != nullptr && y == 0          ? (adjacentChunks[4][i + (size.y - 1) * size.x * size.z] != 0) : (adjacentChunks[4] == nullptr && y == 0          ? 0 : (texture[i - size.x * size.z] != 0)));
    b &= (adjacentChunks[5] != nullptr && y == size.y - 1 ? (adjacentChunks[5][i - (size.y - 1) * size.x * size.z] != 0) : (adjacentChunks[5] == nullptr && y == size.y - 1 ? 0 : (texture[i + size.x * size.z] != 0)));
    return b;
}

void    Chunk::buildMesh( const std::array<const uint8_t*, 6>& adjacentChunks ) {
    this->meshed = true;
    this->voxels.reserve(this->size.x * this->size.y * this->size.z);
    for (int y = 0; y < this->size.y; ++y)
        for (int z = 0; z < this->size.z; ++z)
            for (int x = 0; x < this->size.x; ++x) {
                int i = x + z * this->size.x + y * this->size.x * this->size.z;
                if (this->texture[i] != 0) { /* if voxel is not air */
                    if (!isVoxelCulled(x, y, z, i, adjacentChunks)) {
                        uint8_t visibleFaces = getVisibleFaces(x, y, z, i, adjacentChunks);
                        this->voxels.push_back( { glm::vec3(x, y, z), this->texture[i], visibleFaces } );
                    }
                }
            }
    this->setup(GL_STATIC_DRAW);
}

void    Chunk::render( Shader shader, Camera& camera, uint renderDistance ) {
    if (glm::distance(this->position * glm::vec3(1,0,1), camera.getPosition() * glm::vec3(1,0,1)) > renderDistance * 1.5) {
        outOfRange = true;
        return;
    }
    glm::vec3 size = this->size;
    if (camera.aabInFustrum(-(this->position + size / 2), size) && this->voxels.size() > 0 && this->texture) {
        /* set transform matrix */
        shader.setMat4UniformValue("_mvp", camera.getViewProjectionMatrix() * this->transform);
        shader.setMat4UniformValue("_model", this->transform);
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

    /* position attribute */
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(tPoint), static_cast<GLvoid*>(0));
    /* id attribute */
    glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(tPoint), reinterpret_cast<GLvoid*>(offsetof(tPoint, id)));
    /* occluded faces attribute */
    glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(tPoint), reinterpret_cast<GLvoid*>(offsetof(tPoint, visibleFaces)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void    Chunk::createModelTransform( const glm::vec3& position ) {
    this->transform = glm::mat4();
    this->transform = glm::translate(this->transform, position);
}