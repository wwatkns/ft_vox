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

/*
       ___________ __________ ___________ 
      / '        / '        / '        / |
     /  '       /  '       /  '       /  |
    /__________/__________/__________/   |
    |   '_ _ _ |_ _'_ _ _ |_ _'_ _ _ |_ _|
    |  /'[0]   |  /'[1]   |  /'[2]   |  /|
    | / '      | / '      | / '      | / |
    |/_________|/_________|/_________|/  |
    |   '_ _ _ |_ _'_ _ _ |_ _'_ _ _ |_ _|
    |  /'[7]   |  /'      |  /'[3]   |  /|
    | / '      | / '      | / '      | / |
    |/_________|/_________|/_________|/  |
    |   '_ _ _ |_ _'_ _ _ |_ _'_ _ _ |_ _|
    |  / [6]   |  / [5]   |  / [4]   |  /
    | /        | /        | /        | /
    |/_________|/_________|/_________|/

            top                  middle                 bottom
    +-----+-----+-----+    +-----+/-/-/+-----+    +-----+-----+-----+
    |  0  |  1  |  2  |    |  0  |/////|  1  |    |  0  |  1  |  2  |
    +-----+/-/-/+-----+    +/-/-/+/-/-/+/-/-/+    +-----+/-/-/+-----+
    |  7  |/////|  3  |    |/////|/////|/////|    |  7  |/////|  3  |  3 2
    +-----+/-/-/+-----+    +/-/-/+/-/-/+/-/-/+    +-----+/-/-/+-----+  0 1
    |  6  |  5  |  4  |    |  3  |/////|  2  |    |  6  |  5  |  4  |
    +-----+-----+-----+    +-----+/-/-/+-----+    +-----+-----+-----+

      0 __________ 1
      / |        / |
     /  |       /  |
  3 /__________/ 2 |
    |   |_ _ _ |_ _|
    |  /4      |  / 5
    | /        | /
    |/_________|/
  7              6 

    we have 6 faces, each fce have 4 vertices, each vertex have 4 different ao levels (3 levels + 1 clear)
    so we have 16 configurations for ao per face (16 = 2^4 = 4bits), so we can use an int using 6 * 4bits
*/

tAo  Chunk::getVerticesAoValue( int x, int y, int z, int i, const std::array<const uint8_t*, 6>& adjacentChunks, uint8_t visibleFaces ) {
    /* compute vertices ao value (we have 8 vertices, so 32/8 == 4 bits info per vertex) */
    // glm::ivec2 ao = glm::ivec2(0, 0);
    tAo ao = { 0, 0 };

    const int x_step = 1;
    const int y_step = size.x * size.z;
    const int z_step = size.x;

    const int tnw = -x_step +  z_step + y_step;
    const int tn  =            z_step + y_step;
    const int tne =  x_step +  z_step + y_step;
    const int te  =  x_step +           y_step;
    const int tse =  x_step + -z_step + y_step;
    const int ts  =           -z_step + y_step;
    const int tsw = -x_step + -z_step + y_step;
    const int tw  = -x_step +           y_step;

    const int mnw = -x_step +  z_step;
    const int mne =  x_step +  z_step;
    const int mse =  x_step + -z_step;
    const int msw = -x_step + -z_step;

    const int bnw = -x_step +  z_step + -y_step;
    const int bn  =            z_step + -y_step;
    const int bne =  x_step +  z_step + -y_step;
    const int be  =  x_step +           -y_step;
    const int bse =  x_step + -z_step + -y_step;
    const int bs  =           -z_step + -y_step;
    const int bsw = -x_step + -z_step + -y_step;
    const int bw  = -x_step +           -y_step;
    
    // we need to check 20 voxels
    int max = size.x * size.y * size.z;
    int p[20];
    /* 8 top */
    p[0]  = (int)(i + tnw >= 0 && i + tnw < max && texture[i + tnw] != 0); // top:0
    p[1]  = (int)(i + tn  >= 0 && i + tn  < max && texture[i + tn ] != 0); // top:1
    p[2]  = (int)(i + tne >= 0 && i + tne < max && texture[i + tne] != 0); // top:2
    p[3]  = (int)(i + te  >= 0 && i + te  < max && texture[i + te ] != 0); // top:3
    p[4]  = (int)(i + tse >= 0 && i + tse < max && texture[i + tse] != 0); // top:4
    p[5]  = (int)(i + ts  >= 0 && i + ts  < max && texture[i + ts ] != 0); // top:5
    p[6]  = (int)(i + tsw >= 0 && i + tsw < max && texture[i + tsw] != 0); // top:6
    p[7]  = (int)(i + tw  >= 0 && i + tw  < max && texture[i + tw ] != 0); // top:7
    /* 4 middle */
    p[8]  = (int)(i + mnw >= 0 && i + mnw < max && texture[i + mnw] != 0); // middle:0
    p[9]  = (int)(i + mne >= 0 && i + mne < max && texture[i + mne] != 0); // middle:1
    p[10] = (int)(i + mse >= 0 && i + mse < max && texture[i + mse] != 0); // middle:2
    p[11] = (int)(i + msw >= 0 && i + msw < max && texture[i + msw] != 0); // middle:3
    /* 8 bottom */
    p[12] = (int)(i + bnw >= 0 && i + bnw < max && texture[i + bnw] != 0); // bottom:0
    p[13] = (int)(i + bn  >= 0 && i + bn  < max && texture[i + bn ] != 0); // bottom:1
    p[14] = (int)(i + bne >= 0 && i + bne < max && texture[i + bne] != 0); // bottom:2
    p[15] = (int)(i + be  >= 0 && i + be  < max && texture[i + be ] != 0); // bottom:3
    p[16] = (int)(i + bse >= 0 && i + bse < max && texture[i + bse] != 0); // bottom:4
    p[17] = (int)(i + bs  >= 0 && i + bs  < max && texture[i + bs ] != 0); // bottom:5
    p[18] = (int)(i + bsw >= 0 && i + bsw < max && texture[i + bsw] != 0); // bottom:6
    p[19] = (int)(i + bw  >= 0 && i + bw  < max && texture[i + bw]  != 0); // bottom:7
    // we have 24 vertices to check, 6 faces
    // right, left, front, back, up, down
    /*      top                  middle                 bottom
    +-----+-----+-----+    +-----+/-/-/+-----+    +-----+-----+-----+
    |  0  |  1  |  2  |    |  0  |/////|  1  |    |  0  |  1  |  2  |
    +-----+/-/-/+-----+    +/-/-/+/-/-/+/-/-/+    +-----+/-/-/+-----+
    |  7  |/////|  3  |    |/////|/////|/////|    |  7  |/////|  3  |
    +-----+/-/-/+-----+    +/-/-/+/-/-/+/-/-/+    +-----+/-/-/+-----+
    |  6  |  5  |  4  |    |  3  |/////|  2  |    |  6  |  5  |  4  |
    +-----+-----+-----+    +-----+/-/-/+-----+    +-----+-----+-----+
    */
    if (visibleFaces & 0x20) {
        ao.xz |= (p[10] + p[4]  + p[3] ) << 30; // right:0
        ao.xz |= (p[3]  + p[2]  + p[9] ) << 28; // right:1
        ao.xz |= (p[9]  + p[14] + p[15]) << 26; // right:2
        ao.xz |= (p[15] + p[16] + p[10]) << 24; // right:3
    }
    if (visibleFaces & 0x10) {
        ao.xz |= (p[8]  + p[0]  + p[7] ) << 22; // left:0
        ao.xz |= (p[7]  + p[6]  + p[11]) << 20; // left:1
        ao.xz |= (p[11] + p[18] + p[19]) << 18; // left:2
        ao.xz |= (p[19] + p[12] + p[8] ) << 16; // left:3
    }
    if (visibleFaces & 0x08) {
        ao.xz |= (p[11] + p[6]  + p[5] ) << 14; // front:0
        ao.xz |= (p[5]  + p[4]  + p[10]) << 12; // front:1
        ao.xz |= (p[10] + p[16] + p[17]) << 10; // front:2
        ao.xz |= (p[17] + p[18] + p[11]) <<  8; // front:3
    }
    if (visibleFaces & 0x04) {
        ao.xz |= (p[9]  + p[2]  + p[1] ) <<  6; // back:0
        ao.xz |= (p[1]  + p[0]  + p[8] ) <<  4; // back:1
        ao.xz |= (p[8]  + p[12] + p[13]) <<  3; // back:2
        ao.xz |= (p[13] + p[14] + p[9] ) <<  0; // back:3
    }
    if (visibleFaces & 0x02) {
        ao.y  |= (p[7]  + p[0]  + p[1] ) << 14; // top:0
        ao.y  |= (p[1]  + p[2]  + p[3] ) << 12; // top:1
        ao.y  |= (p[3]  + p[4]  + p[5] ) << 10; // top:2
        ao.y  |= (p[5]  + p[6]  + p[7] ) <<  8; // top:3
    }
    if (visibleFaces & 0x01) {
        ao.y  |= (p[19] + p[18] + p[17]) <<  6; // bottom:0
        ao.y  |= (p[17] + p[16] + p[15]) <<  4; // bottom:1
        ao.y  |= (p[15] + p[14] + p[13]) <<  2; // bottom:2
        ao.y  |= (p[13] + p[12] + p[19]) <<  0; // bottom:3
    }
    return ao;
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
                        uint8_t visibleFaces = 255;//getVisibleFaces(x, y, z, i, adjacentChunks);
                        uint8_t b = (uint8_t)(this->texture[i] - 1);
                        /* change dirt to grass on top */
                        if (b == 0 && !(adjacentChunks[5] != nullptr && y == size.y - 1 ? (adjacentChunks[5][i - (size.y - 1) * size.x * size.z] != 0) : (adjacentChunks[5] == nullptr && y == size.y - 1 ? 0 : (texture[i + size.x * size.z] != 0))))
                            b = 1;
                        tAo ao = getVerticesAoValue(x, y, z, i, adjacentChunks, visibleFaces);
                        // this->voxels.push_back( (tPoint){ glm::vec3(x, y, z), b, visibleFaces, ao } );
                        this->voxels.push_back( { glm::vec3(x, y, z), b, visibleFaces, ao.xz, ao.y } );
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
    glm::vec3 size = this->size;
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
    /* id attribute */
    glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(tPoint), reinterpret_cast<GLvoid*>(offsetof(tPoint, id)));
    /* occluded faces attribute */
    glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(tPoint), reinterpret_cast<GLvoid*>(offsetof(tPoint, visibleFaces)));
    /* ao attribute */
    glEnableVertexAttribArray(3);
	glVertexAttribIPointer(3, 1, GL_INT, sizeof(tPoint), reinterpret_cast<GLvoid*>(offsetof(tPoint, ao_xz)));

    glEnableVertexAttribArray(4);
	glVertexAttribIPointer(4, 1, GL_INT, sizeof(tPoint), reinterpret_cast<GLvoid*>(offsetof(tPoint, ao_y)));


    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void    Chunk::createModelTransform( const glm::vec3& position ) {
    this->transform = glm::mat4();
    this->transform = glm::translate(this->transform, position);
}