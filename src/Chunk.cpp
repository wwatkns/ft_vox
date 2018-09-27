#include "Chunk.hpp"
#include "glm/ext.hpp"

Chunk::Chunk( const glm::vec3& position, const glm::ivec3& chunkSize, const uint8_t* texture, const uint margin ) : position(position), chunkSize(chunkSize), margin(margin), meshed(false), outOfRange(false) {
    this->createModelTransform(position);
    this->paddedSize = chunkSize + static_cast<int>(margin);
    this->texture = static_cast<uint8_t*>(malloc(sizeof(uint8_t) * paddedSize.x * paddedSize.y * paddedSize.z));
    memcpy(this->texture, texture, paddedSize.x * paddedSize.y * paddedSize.z);
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
    faces |=  (this->texture[i + paddedSize.x * paddedSize.z] == 0) << 1; // top
    faces |=  (this->texture[i - paddedSize.x * paddedSize.z] == 0) << 0; // bottom
    return faces;
}

bool    Chunk::isVoxelCulled( int i ) {
    uint8_t b = 0x1;
    b &= (this->texture[i + 1                          ] != 0); // right
    b &= (this->texture[i - 1                          ] != 0); // left
    b &= (this->texture[i + paddedSize.x               ] != 0); // front
    b &= (this->texture[i - paddedSize.x               ] != 0); // back
    b &= (this->texture[i + paddedSize.x * paddedSize.z] != 0); // top
    b &= (this->texture[i - paddedSize.x * paddedSize.z] != 0); // bottom
    return b;
}

static int pow2( int x ) {
    return x * x;
}

glm::ivec2  Chunk::getVerticesAoValue( int i, uint8_t visibleFaces ) {
    glm::ivec2  ao = glm::ivec2(0, 0);

    const int x_step = 1;
    const int y_step = paddedSize.x * paddedSize.z;
    const int z_step = paddedSize.x;
    /* we have 20 voxels to check */
    const int p[20] = {
        /* top */
        (texture[i - x_step - z_step + y_step] != 0), //    top:0, 0
        (texture[i -          z_step + y_step] != 0), //    top:1, 1
        (texture[i + x_step - z_step + y_step] != 0), //    top:2, 2
        (texture[i + x_step +          y_step] != 0), //    top:3, 3
        (texture[i + x_step + z_step + y_step] != 0), //    top:4, 4
        (texture[i +          z_step + y_step] != 0), //    top:5, 5
        (texture[i - x_step + z_step + y_step] != 0), //    top:6, 6
        (texture[i - x_step +          y_step] != 0), //    top:7, 7
        /* middle */
        (texture[i - x_step - z_step         ] != 0), // middle:0, 8
        (texture[i + x_step - z_step         ] != 0), // middle:1, 9
        (texture[i + x_step + z_step         ] != 0), // middle:2, 10
        (texture[i - x_step + z_step         ] != 0), // middle:3, 11
        /* bottom */
        (texture[i - x_step - z_step - y_step] != 0), // bottom:0, 12
        (texture[i -          z_step - y_step] != 0), // bottom:1, 13
        (texture[i + x_step - z_step - y_step] != 0), // bottom:2, 14
        (texture[i + x_step -          y_step] != 0), // bottom:3, 15
        (texture[i + x_step + z_step - y_step] != 0), // bottom:4, 16
        (texture[i +          z_step - y_step] != 0), // bottom:5, 17
        (texture[i - x_step + z_step - y_step] != 0), // bottom:6, 18
        (texture[i - x_step -          y_step]  != 0) // bottom:7, 19
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
        facesAo[0] |= static_cast<int>( std::min(p[10]*.75f + p[4] *.5f + p[3] *.75f, 1.5f) * 2) << 2; // right:0
        facesAo[0] |= static_cast<int>( std::min(p[3] *.75f + p[2] *.5f + p[9] *.75f, 1.5f) * 2) << 0; // right:1
        facesAo[0] |= static_cast<int>( std::min(p[9] *.75f + p[14]*.5f + p[15]*.75f, 1.5f) * 2) << 4; // right:2
        facesAo[0] |= static_cast<int>( std::min(p[15]*.75f + p[16]*.5f + p[10]*.75f, 1.5f) * 2) << 6; // right:3
    }
    if (visibleFaces & 0x10) { // [2, 1, 3, 0]
        facesAo[1] |= static_cast<int>( std::min(p[8] *.75f + p[0] *.5f + p[7] *.75f, 1.5f) * 2) << 6; // left:0
        facesAo[1] |= static_cast<int>( std::min(p[7] *.75f + p[6] *.5f + p[11]*.75f, 1.5f) * 2) << 2; // left:1
        facesAo[1] |= static_cast<int>( std::min(p[11]*.75f + p[18]*.5f + p[19]*.75f, 1.5f) * 2) << 0; // left:2
        facesAo[1] |= static_cast<int>( std::min(p[19]*.75f + p[12]*.5f + p[8] *.75f, 1.5f) * 2) << 4; // left:3
    }
    if (visibleFaces & 0x08) { // [2, 1, 3, 0]
        facesAo[2] |= static_cast<int>( std::min(p[11]*.75f + p[6] *.5f + p[5] *.75f, 1.5f) * 2) << 6; // front:0
        facesAo[2] |= static_cast<int>( std::min(p[5] *.75f + p[4] *.5f + p[10]*.75f, 1.5f) * 2) << 2; // front:1
        facesAo[2] |= static_cast<int>( std::min(p[10]*.75f + p[16]*.5f + p[17]*.75f, 1.5f) * 2) << 0; // front:2
        facesAo[2] |= static_cast<int>( std::min(p[17]*.75f + p[18]*.5f + p[11]*.75f, 1.5f) * 2) << 4; // front:3
    }
    if (visibleFaces & 0x04) { // [1, 0, 2, 3]
        facesAo[3] |= static_cast<int>( std::min(p[9] *.75f + p[2] *.5f + p[1] *.75f, 1.5f) * 2) << 2; // back:0
        facesAo[3] |= static_cast<int>( std::min(p[1] *.75f + p[0] *.5f + p[8] *.75f, 1.5f) * 2) << 0; // back:1
        facesAo[3] |= static_cast<int>( std::min(p[8] *.75f + p[12]*.5f + p[13]*.75f, 1.5f) * 2) << 4; // back:2
        facesAo[3] |= static_cast<int>( std::min(p[13]*.75f + p[14]*.5f + p[9] *.75f, 1.5f) * 2) << 6; // back:3
    }
    if (visibleFaces & 0x02) { // [3, 2, 0, 1]
        facesAo[4] |= static_cast<int>( std::min(p[7] *.75f + p[0] *.5f + p[1] *.75f, 1.5f) * 2) << 4; // top:0
        facesAo[4] |= static_cast<int>( std::min(p[1] *.75f + p[2] *.5f + p[3] *.75f, 1.5f) * 2) << 6; // top:1
        facesAo[4] |= static_cast<int>( std::min(p[3] *.75f + p[4] *.5f + p[5] *.75f, 1.5f) * 2) << 2; // top:2
        facesAo[4] |= static_cast<int>( std::min(p[5] *.75f + p[6] *.5f + p[7] *.75f, 1.5f) * 2) << 0; // top:3
    }
    if (visibleFaces & 0x01) { // [1, 2, 0, 3]
        facesAo[5] |= static_cast<int>( std::min(p[19]*.75f + p[12]*.5f + p[13]*.75f, 1.5f) * 2) << 4; // bottom:0
        facesAo[5] |= static_cast<int>( std::min(p[13]*.75f + p[14]*.5f + p[15]*.75f, 1.5f) * 2) << 0; // bottom:1
        facesAo[5] |= static_cast<int>( std::min(p[15]*.75f + p[16]*.5f + p[17]*.75f, 1.5f) * 2) << 2; // bottom:2
        facesAo[5] |= static_cast<int>( std::min(p[17]*.75f + p[18]*.5f + p[19]*.75f, 1.5f) * 2) << 6; // bottom:3
    }
    /* we check the vertices ao values to determine if the quad must be flipped, and we pack it in ao.y at 0x00FF0000 */
    uint8_t flippedQuads = 0x0;
    for (int i = 0; i < 6; i++) {
        flippedQuads |= (int)(pow2((facesAo[i]&0x30)>>4) + pow2((facesAo[i]&0x0C)>>2) > pow2((facesAo[i]&0xC0)>>6) + pow2((facesAo[i]&0x03)>>0)) << (5-i);
    }
    ao.x = (facesAo[0] << 24) | (facesAo[1] << 16) | (facesAo[2] << 8) | (facesAo[3]);
    ao.y = (flippedQuads << 16) | (facesAo[4] <<  8) | (facesAo[5]);
    return ao;
}

    // if (visibleFaces & 0x20) { // [1, 0, 2, 3]
    //     ao.x |= static_cast<int>( std::min(p[10]*.75f + p[4] *.5f + p[3] *.75f, 1.5f) * 2) << 26; // right:0
    //     ao.x |= static_cast<int>( std::min(p[3] *.75f + p[2] *.5f + p[9] *.75f, 1.5f) * 2) << 24; // right:1
    //     ao.x |= static_cast<int>( std::min(p[9] *.75f + p[14]*.5f + p[15]*.75f, 1.5f) * 2) << 28; // right:2
    //     ao.x |= static_cast<int>( std::min(p[15]*.75f + p[16]*.5f + p[10]*.75f, 1.5f) * 2) << 30; // right:3
    // }
    // if (visibleFaces & 0x10) { // [2, 1, 3, 0]
    //     ao.x |= static_cast<int>( std::min(p[8] *.75f + p[0] *.5f + p[7] *.75f, 1.5f) * 2) << 22; // left:0
    //     ao.x |= static_cast<int>( std::min(p[7] *.75f + p[6] *.5f + p[11]*.75f, 1.5f) * 2) << 18; // left:1
    //     ao.x |= static_cast<int>( std::min(p[11]*.75f + p[18]*.5f + p[19]*.75f, 1.5f) * 2) << 16; // left:2
    //     ao.x |= static_cast<int>( std::min(p[19]*.75f + p[12]*.5f + p[8] *.75f, 1.5f) * 2) << 20; // left:3
    // }
    // if (visibleFaces & 0x08) { // [2, 1, 3, 0]
    //     ao.x |= static_cast<int>( std::min(p[11]*.75f + p[6] *.5f + p[5] *.75f, 1.5f) * 2) << 14; // front:0
    //     ao.x |= static_cast<int>( std::min(p[5] *.75f + p[4] *.5f + p[10]*.75f, 1.5f) * 2) << 10; // front:1
    //     ao.x |= static_cast<int>( std::min(p[10]*.75f + p[16]*.5f + p[17]*.75f, 1.5f) * 2) <<  8; // front:2
    //     ao.x |= static_cast<int>( std::min(p[17]*.75f + p[18]*.5f + p[11]*.75f, 1.5f) * 2) << 12; // front:3
    // }
    // if (visibleFaces & 0x04) { // [1, 0, 2, 3]
    //     ao.x |= static_cast<int>( std::min(p[9] *.75f + p[2] *.5f + p[1] *.75f, 1.5f) * 2) <<  2; // back:0
    //     ao.x |= static_cast<int>( std::min(p[1] *.75f + p[0] *.5f + p[8] *.75f, 1.5f) * 2) <<  0; // back:1
    //     ao.x |= static_cast<int>( std::min(p[8] *.75f + p[12]*.5f + p[13]*.75f, 1.5f) * 2) <<  4; // back:2
    //     ao.x |= static_cast<int>( std::min(p[13]*.75f + p[14]*.5f + p[9] *.75f, 1.5f) * 2) <<  6; // back:3
    // }
    // if (visibleFaces & 0x02) { // [3, 2, 0, 1]
    //     ao.y |= static_cast<int>( std::min(p[7] *.75f + p[0] *.5f + p[1] *.75f, 1.5f) * 2) << 12; // top:0
    //     ao.y |= static_cast<int>( std::min(p[1] *.75f + p[2] *.5f + p[3] *.75f, 1.5f) * 2) << 14; // top:1
    //     ao.y |= static_cast<int>( std::min(p[3] *.75f + p[4] *.5f + p[5] *.75f, 1.5f) * 2) << 10; // top:2
    //     ao.y |= static_cast<int>( std::min(p[5] *.75f + p[6] *.5f + p[7] *.75f, 1.5f) * 2) <<  8; // top:3
    // }
    // if (visibleFaces & 0x01) { // [1, 2, 0, 3]
    //     ao.y |= static_cast<int>( std::min(p[19]*.75f + p[12]*.5f + p[13]*.75f, 1.5f) * 2) <<  4; // bottom:0
    //     ao.y |= static_cast<int>( std::min(p[13]*.75f + p[14]*.5f + p[15]*.75f, 1.5f) * 2) <<  0; // bottom:1
    //     ao.y |= static_cast<int>( std::min(p[15]*.75f + p[16]*.5f + p[17]*.75f, 1.5f) * 2) <<  2; // bottom:2
    //     ao.y |= static_cast<int>( std::min(p[17]*.75f + p[18]*.5f + p[19]*.75f, 1.5f) * 2) <<  6; // bottom:3
    // }

void    Chunk::buildMesh( void ) {
    this->meshed = true;
    this->voxels.reserve(chunkSize.x * chunkSize.y * chunkSize.z);
    for (int y = 0; y < chunkSize.y; ++y)
        for (int z = 0; z < chunkSize.z; ++z)
            for (int x = 0; x < chunkSize.x; ++x) {
                int i = (x+margin/2) + (z+margin/2) * paddedSize.x + (y+margin/2) * paddedSize.x * paddedSize.z;
                if (this->texture[i] != 0) { /* if voxel is not air */
                    if (!isVoxelCulled(i)) {
                        uint8_t visibleFaces = getVisibleFaces(i);
                        uint8_t b = static_cast<uint8_t>(this->texture[i] - 1);
                        /* change dirt to grass on top */
                        if (texture[i + paddedSize.x * paddedSize.z] == 0)
                            b = 1;
                        glm::ivec2 ao = getVerticesAoValue(i, visibleFaces);
                        this->voxels.push_back( (tPoint){ glm::vec3(x, y, z), ao, b, visibleFaces } );
                    }
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