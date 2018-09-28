#include "Chunk.hpp"
#include "glm/ext.hpp"

Chunk::Chunk( const glm::vec3& position, const glm::ivec3& chunkSize, const uint8_t* texture, const uint margin ) : position(position), chunkSize(chunkSize), margin(margin), meshed(false), outOfRange(false) {
    this->createModelTransform(position);
    this->paddedSize = chunkSize + static_cast<int>(margin);
    this->texture = static_cast<uint8_t*>(malloc(sizeof(uint8_t) * paddedSize.x * paddedSize.y * paddedSize.z));
    memcpy(this->texture, texture, paddedSize.x * paddedSize.y * paddedSize.z);
    this->y_step = paddedSize.x * paddedSize.z;
}

Chunk::~Chunk( void ) {
    free(this->texture);
    this->texture = NULL;
    glDeleteBuffers(1, &this->vao);
    glDeleteBuffers(1, &this->vbo);
}

uint8_t Chunk::getVisibleFaces( int i ) {
    uint8_t faces = 0x0;
    faces |=  (this->texture[i + 1                          ] == 0) << 5; // right
    faces |=  (this->texture[i - 1                          ] == 0) << 4; // left
    faces |=  (this->texture[i + paddedSize.x               ] == 0) << 3; // front
    faces |=  (this->texture[i - paddedSize.x               ] == 0) << 2; // back
    faces |=  (this->texture[i + this->y_step] == 0) << 1; // top
    faces |=  (this->texture[i - this->y_step] == 0) << 0; // bottom
    return faces;
}

bool    Chunk::isVoxelCulled( int i ) {
    uint8_t b = 0x1;
    b &= (this->texture[i + 1                          ] != 0); // right
    b &= (this->texture[i - 1                          ] != 0); // left
    b &= (this->texture[i + paddedSize.x               ] != 0); // front
    b &= (this->texture[i - paddedSize.x               ] != 0); // back
    b &= (this->texture[i + this->y_step] != 0); // top
    b &= (this->texture[i - this->y_step] != 0); // bottom
    return b;
}

static int pow2( int x ) {
    return x * x;
}

glm::ivec2  Chunk::getVerticesAoValue( int i, uint8_t visibleFaces ) {
    const int x_step = 1;
    const int z_step = paddedSize.x;
    /* we have 20 voxels to check */
    const std::array<int, 20> p = {
        /* top */
        (this->texture[i - x_step - z_step + y_step] != 0), //    top:0, 0
        (this->texture[i -          z_step + y_step] != 0), //    top:1, 1
        (this->texture[i + x_step - z_step + y_step] != 0), //    top:2, 2
        (this->texture[i + x_step +          y_step] != 0), //    top:3, 3
        (this->texture[i + x_step + z_step + y_step] != 0), //    top:4, 4
        (this->texture[i +          z_step + y_step] != 0), //    top:5, 5
        (this->texture[i - x_step + z_step + y_step] != 0), //    top:6, 6
        (this->texture[i - x_step +          y_step] != 0), //    top:7, 7
        /* middle */
        (this->texture[i - x_step - z_step         ] != 0), // middle:0, 8
        (this->texture[i + x_step - z_step         ] != 0), // middle:1, 9
        (this->texture[i + x_step + z_step         ] != 0), // middle:2, 10
        (this->texture[i - x_step + z_step         ] != 0), // middle:3, 11
        /* bottom */
        (this->texture[i - x_step - z_step - y_step] != 0), // bottom:0, 12
        (this->texture[i -          z_step - y_step] != 0), // bottom:1, 13
        (this->texture[i + x_step - z_step - y_step] != 0), // bottom:2, 14
        (this->texture[i + x_step -          y_step] != 0), // bottom:3, 15
        (this->texture[i + x_step + z_step - y_step] != 0), // bottom:4, 16
        (this->texture[i +          z_step - y_step] != 0), // bottom:5, 17
        (this->texture[i - x_step + z_step - y_step] != 0), // bottom:6, 18
        (this->texture[i - x_step -          y_step] != 0)  // bottom:7, 19
    };
    /*      top                  middle                 bottom
    +-----+-----+-----+    +-----+/-/-/+-----+    +-----+-----+-----+
    |  0  |  1  |  2  |    |  0  |/////|  1  |    |  0  |  1  |  2  |
    +-----+/-/-/+-----+    +/-/-/+/-/-/+/-/-/+    +-----+/-/-/+-----+
    |  7  |/////|  3  |    |/////|/////|/////|    |  7  |/////|  3  |
    +-----+/-/-/+-----+    +/-/-/+/-/-/+/-/-/+    +-----+/-/-/+-----+
    |  6  |  5  |  4  |    |  3  |/////|  2  |    |  6  |  5  |  4  |
    +-----+-----+-----+    +-----+/-/-/+-----+    +-----+-----+-----+
    */
    std::array<int, 6> facesAo = { 0, 0, 0, 0, 0, 0 };
    if (visibleFaces & 0x20) { // [1, 0, 2, 3]
        facesAo[0] = (static_cast<int>( std::min(p[10]*.75f + p[4] *.5f + p[3] *.75f, 1.5f) * 2) << 2) | // right:0
                     (static_cast<int>( std::min(p[3] *.75f + p[2] *.5f + p[9] *.75f, 1.5f) * 2) << 0) | // right:1
                     (static_cast<int>( std::min(p[9] *.75f + p[14]*.5f + p[15]*.75f, 1.5f) * 2) << 4) | // right:2
                     (static_cast<int>( std::min(p[15]*.75f + p[16]*.5f + p[10]*.75f, 1.5f) * 2) << 6);  // right:3
    }
    if (visibleFaces & 0x10) { // [2, 1, 3, 0]
        facesAo[1] = (static_cast<int>( std::min(p[8] *.75f + p[0] *.5f + p[7] *.75f, 1.5f) * 2) << 6) | // left:0
                     (static_cast<int>( std::min(p[7] *.75f + p[6] *.5f + p[11]*.75f, 1.5f) * 2) << 2) | // left:1
                     (static_cast<int>( std::min(p[11]*.75f + p[18]*.5f + p[19]*.75f, 1.5f) * 2) << 0) | // left:2
                     (static_cast<int>( std::min(p[19]*.75f + p[12]*.5f + p[8] *.75f, 1.5f) * 2) << 4);  // left:3
    }
    if (visibleFaces & 0x08) { // [2, 1, 3, 0]
        facesAo[2] = (static_cast<int>( std::min(p[11]*.75f + p[6] *.5f + p[5] *.75f, 1.5f) * 2) << 6) | // front:0
                     (static_cast<int>( std::min(p[5] *.75f + p[4] *.5f + p[10]*.75f, 1.5f) * 2) << 2) | // front:1
                     (static_cast<int>( std::min(p[10]*.75f + p[16]*.5f + p[17]*.75f, 1.5f) * 2) << 0) | // front:2
                     (static_cast<int>( std::min(p[17]*.75f + p[18]*.5f + p[11]*.75f, 1.5f) * 2) << 4);  // front:3
    }
    if (visibleFaces & 0x04) { // [1, 0, 2, 3]
        facesAo[3] = (static_cast<int>( std::min(p[9] *.75f + p[2] *.5f + p[1] *.75f, 1.5f) * 2) << 2) | // back:0
                     (static_cast<int>( std::min(p[1] *.75f + p[0] *.5f + p[8] *.75f, 1.5f) * 2) << 0) | // back:1
                     (static_cast<int>( std::min(p[8] *.75f + p[12]*.5f + p[13]*.75f, 1.5f) * 2) << 4) | // back:2
                     (static_cast<int>( std::min(p[13]*.75f + p[14]*.5f + p[9] *.75f, 1.5f) * 2) << 6);  // back:3
    }
    if (visibleFaces & 0x02) { // [3, 2, 0, 1]
        facesAo[4] = (static_cast<int>( std::min(p[7] *.75f + p[0] *.5f + p[1] *.75f, 1.5f) * 2) << 4) | // top:0
                     (static_cast<int>( std::min(p[1] *.75f + p[2] *.5f + p[3] *.75f, 1.5f) * 2) << 6) | // top:1
                     (static_cast<int>( std::min(p[3] *.75f + p[4] *.5f + p[5] *.75f, 1.5f) * 2) << 2) | // top:2
                     (static_cast<int>( std::min(p[5] *.75f + p[6] *.5f + p[7] *.75f, 1.5f) * 2) << 0);  // top:3
    }
    if (visibleFaces & 0x01) { // [1, 2, 0, 3]
        facesAo[5] = (static_cast<int>( std::min(p[19]*.75f + p[12]*.5f + p[13]*.75f, 1.5f) * 2) << 4) | // bottom:0
                     (static_cast<int>( std::min(p[13]*.75f + p[14]*.5f + p[15]*.75f, 1.5f) * 2) << 0) | // bottom:1
                     (static_cast<int>( std::min(p[15]*.75f + p[16]*.5f + p[17]*.75f, 1.5f) * 2) << 2) | // bottom:2
                     (static_cast<int>( std::min(p[17]*.75f + p[18]*.5f + p[19]*.75f, 1.5f) * 2) << 6);  // bottom:3
    }
    /* we check the vertices ao values to determine if the quad must be flipped, and we pack it in ao.y at 0x00FF0000 */
    uint8_t flippedQuads = 0x0;
    for (int i = 0; i < 6; i++) {
        if (visibleFaces & (0x20 >> i))
            flippedQuads |= (int)(pow2((facesAo[i]&0x30)>>4) + pow2((facesAo[i]&0x0C)>>2) > pow2((facesAo[i]&0xC0)>>6) + pow2((facesAo[i]&0x03)>>0)) << (5-i);
    }
    glm::ivec2  ao = glm::ivec2(0, 0);
    ao.x = (facesAo[0] << 24) | (facesAo[1] << 16) | (facesAo[2] << 8) | (facesAo[3]);
    ao.y = (flippedQuads << 16) | (facesAo[4] <<  8) | (facesAo[5]);
    return ao;
}

void    Chunk::buildMesh( void ) {
    const int m = this->margin / 2;
    this->meshed = true;
    this->voxels.reserve(chunkSize.x * chunkSize.y * chunkSize.z);
    for (int y = 0; y < chunkSize.y; ++y)
        for (int z = 0; z < chunkSize.z; ++z)
            for (int x = 0; x < chunkSize.x; ++x) {
                int i = (x+m) + (z+m) * paddedSize.x + (y+m) * this->y_step;
                if (this->texture[i] != 0 && !isVoxelCulled(i)) { /* if voxel is not air and not culled */
                    uint8_t visibleFaces = getVisibleFaces(i);
                    uint8_t b = static_cast<uint8_t>(this->texture[i] - 1);
                    /* change dirt to grass on top */
                    if (texture[i + this->y_step] == 0)
                        b = 1;
                    glm::ivec2 ao = getVerticesAoValue(i, visibleFaces);
                    this->voxels.push_back( (tPoint){ glm::vec3(x, y, z), ao, b, visibleFaces } );
                }
            }
    this->setup(GL_STATIC_DRAW);
}

void    Chunk::render( Shader shader, Camera& camera, GLuint textureAtlas, uint renderDistance ) {
    if (glm::distance(this->position * glm::vec3(1,0,1), camera.getPosition() * glm::vec3(1,0,1)) > renderDistance * 1.5) {
        outOfRange = true;
        return;
    }
    glm::vec3 size = this->chunkSize;
    if (camera.aabInFustrum(-(this->position + size / 2), size) && this->voxels.size() > 0 && this->texture) {
        /* set transform matrix */
        shader.setMat4UniformValue("_mvp", camera.getViewProjectionMatrix() * this->transform);
        shader.setMat4UniformValue("_model", this->transform);
        /* texture atlas */
        glActiveTexture(GL_TEXTURE0);
        shader.setIntUniformValue("atlas", 0);
        glBindTexture(GL_TEXTURE_2D, textureAtlas);
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
    /* ao attribute */
    glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 2, GL_INT, sizeof(tPoint), reinterpret_cast<GLvoid*>(offsetof(tPoint, ao)));
    /* id attribute */
    glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(tPoint), reinterpret_cast<GLvoid*>(offsetof(tPoint, id)));
    /* occluded faces attribute */
    glEnableVertexAttribArray(3);
	glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE, sizeof(tPoint), reinterpret_cast<GLvoid*>(offsetof(tPoint, visibleFaces)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void    Chunk::createModelTransform( const glm::vec3& position ) {
    this->transform = glm::mat4();
    this->transform = glm::translate(this->transform, position);
}