#include "Chunk.hpp"
#include "glm/ext.hpp"

Chunk::Chunk( const glm::vec3& position, const glm::ivec3& size, const uint8_t* texture ) : position(position), size(size) {
    this->createModelTransform(position);
    this->texture = static_cast<uint8_t*>(malloc(sizeof(uint8_t) * size.x * size.y * size.z));
    memcpy(this->texture, texture, size.x * size.y * size.z);
}

Chunk::~Chunk( void ) {
    glDeleteBuffers(1, &this->vao);
    glDeleteBuffers(1, &this->vbo);
}

// bool    Chunk::isVoxelCulled( int x, int y, int z, int i ) {
//     return !((x + 1 < size.x && this->texture[i + 1              ] != 0) && /* right */
//              (x - 1 >= 0     && this->texture[i - 1              ] != 0) && /* left */
//              (z + 1 < size.z && this->texture[i + size.x         ] != 0) && /* front */
//              (z - 1 >= 0     && this->texture[i - size.x         ] != 0) && /* back */
//              (y + 1 < size.y && this->texture[i + size.x * size.z] != 0) && /* up */
//              (y - 1 >= 0     && this->texture[i - size.x * size.z] != 0));  /* down */
// }

uint8_t Chunk::getVisibleFaces( int x, int y, int z, int i ) {
    uint8_t faces = 0x0;
    if (x + 1 < size.x && this->texture[i + 1              ] == 0) faces |= (1 << 5); // right
    if (x - 1 >= 0     && this->texture[i - 1              ] == 0) faces |= (1 << 4); // left
    if (z + 1 < size.z && this->texture[i + size.x         ] == 0) faces |= (1 << 3); // front
    if (z - 1 >= 0     && this->texture[i - size.x         ] == 0) faces |= (1 << 2); // back
    if (y + 1 < size.y && this->texture[i + size.x * size.z] == 0) faces |= (1 << 1); // up
    if (y - 1 >= 0     && this->texture[i - size.x * size.z] == 0) faces |= (1 << 0); // down
    return faces;
}

bool    Chunk::isVoxelCulled( int x, int y, int z, int i, const std::array<const uint8_t*, 6>& adjacentChunks ) {
    bool left, right, front, back, top, down;
    /* empty */
    // left  = (adjacentChunks[0] != nullptr && x == 0          ? (adjacentChunks[0][i + (size.x - 1)                  ] != 0) : (texture[i - 1] != 0));
    // right = (adjacentChunks[1] != nullptr && x == size.x - 1 ? (adjacentChunks[1][i - (size.x - 1)                  ] != 0) : (texture[i + 1] != 0));
    // back  = (adjacentChunks[2] != nullptr && z == 0          ? (adjacentChunks[2][i + (size.z - 1) * size.x         ] != 0) : (texture[i - size.x] != 0));
    // front = (adjacentChunks[3] != nullptr && z == size.z - 1 ? (adjacentChunks[3][i - (size.z - 1) * size.x         ] != 0) : (texture[i + size.x] != 0));
    // down  = (adjacentChunks[4] != nullptr && y == 0          ? (adjacentChunks[4][i + (size.y - 1) * size.x * size.z] != 0) : (texture[i - size.x * size.z] != 0));
    // top   = (adjacentChunks[5] != nullptr && y == size.y - 1 ? (adjacentChunks[5][i - (size.y - 1) * size.x * size.z] != 0) : (texture[i + size.x * size.z] != 0));
    /* empty with borders on world extremities */
    left  = (adjacentChunks[0] != nullptr && x == 0          ? (adjacentChunks[0][i + (size.x - 1)                  ] != 0) : (adjacentChunks[0] == nullptr && x == 0          ? 0 : (texture[i - 1] != 0)));
    right = (adjacentChunks[1] != nullptr && x == size.x - 1 ? (adjacentChunks[1][i - (size.x - 1)                  ] != 0) : (adjacentChunks[1] == nullptr && x == size.x - 1 ? 0 : (texture[i + 1] != 0)));
    back  = (adjacentChunks[2] != nullptr && z == 0          ? (adjacentChunks[2][i + (size.z - 1) * size.x         ] != 0) : (adjacentChunks[2] == nullptr && z == 0          ? 0 : (texture[i - size.x] != 0)));
    front = (adjacentChunks[3] != nullptr && z == size.z - 1 ? (adjacentChunks[3][i - (size.z - 1) * size.x         ] != 0) : (adjacentChunks[3] == nullptr && z == size.z - 1 ? 0 : (texture[i + size.x] != 0)));
    down  = (adjacentChunks[4] != nullptr && y == 0          ? (adjacentChunks[4][i + (size.y - 1) * size.x * size.z] != 0) : (adjacentChunks[4] == nullptr && y == 0          ? 0 : (texture[i - size.x * size.z] != 0)));
    top   = (adjacentChunks[5] != nullptr && y == size.y - 1 ? (adjacentChunks[5][i - (size.y - 1) * size.x * size.z] != 0) : (adjacentChunks[5] == nullptr && y == size.y - 1 ? 0 : (texture[i + size.x * size.z] != 0)));
    return (left && right && front && back && top && down);
}

void    Chunk::buildMesh( const std::array<const uint8_t*, 6>& adjacentChunks ) {
    this->voxels.reserve(this->size.x * this->size.y * this->size.z);
    for (int y = 0; y < this->size.y; ++y)
        for (int z = 0; z < this->size.z; ++z)
            for (int x = 0; x < this->size.x; ++x) {
                int i = x + z * this->size.x + y * this->size.x * this->size.z;
                if (this->texture[i] != 0) { /* if voxel is not air */
                    if (!isVoxelCulled(x, y, z, i, adjacentChunks)) {
                        uint8_t visibleFaces = getVisibleFaces(x, y, z, i);
                        this->voxels.push_back( { glm::vec3(x, y, z), this->texture[i], visibleFaces } );
                    }
                }
            }
    this->setup(GL_STATIC_DRAW);
}

void    Chunk::render( Shader shader, Camera& camera ) {
    glm::vec3 size = this->size;
    /* optimisation: view fustrum occlusion */
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