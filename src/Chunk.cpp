#include "Chunk.hpp"
#include "glm/ext.hpp"

Chunk::Chunk( const glm::vec3& position, const glm::ivec3& chunkSize, const uint8_t* texture, const uint margin ) : position(position), chunkSize(chunkSize), margin(margin), meshed(false), lighted(false), underground(false), outOfRange(false) {
    this->createModelTransform(position);
    this->paddedSize = chunkSize + static_cast<int>(margin);
    this->y_step = paddedSize.x * paddedSize.z;

    this->texture = static_cast<uint8_t*>(malloc(sizeof(uint8_t) * paddedSize.x * paddedSize.y * paddedSize.z));
    memcpy(this->texture, texture, paddedSize.x * paddedSize.y * paddedSize.z);
    /* the light-mask is only a horizontal slice containing information about wether the sky is seen from this vertical position */
    this->lightMask = static_cast<uint8_t*>(malloc(sizeof(uint8_t) * paddedSize.x * paddedSize.z));
    memset(this->lightMask, 15, paddedSize.x * paddedSize.z);
    /* the light-map is the voxels light values in the chunk */
    this->lightMap = static_cast<uint8_t*>(malloc(sizeof(uint8_t) * paddedSize.x * paddedSize.y * paddedSize.z));
    memset(this->lightMap, 0, paddedSize.x * paddedSize.y * paddedSize.z);
}

Chunk::~Chunk( void ) {
    this->voxels.clear();
    free(this->texture);
    this->texture = nullptr;
    free(this->lightMask);
    this->lightMask = nullptr;
    free(this->lightMap);
    this->lightMap = nullptr;
    glDeleteBuffers(1, &this->vao);
    glDeleteBuffers(1, &this->vbo);
}

const uint8_t Chunk::getVisibleFaces( int i ) const {
    return ((this->texture[i + 1           ] == 0) << 5) | // right
           ((this->texture[i - 1           ] == 0) << 4) | // left
           ((this->texture[i + paddedSize.x] == 0) << 3) | // front
           ((this->texture[i - paddedSize.x] == 0) << 2) | // back
           ((this->texture[i + this->y_step] == 0) << 1) | // top
           ((this->texture[i - this->y_step] == 0) << 0);  // bottom
}

const bool    Chunk::isVoxelCulled( int i ) const {
    return (this->texture[i + 1           ] != 0) & // right
           (this->texture[i - 1           ] != 0) & // left
           (this->texture[i + paddedSize.x] != 0) & // front
           (this->texture[i - paddedSize.x] != 0) & // back
           (this->texture[i + this->y_step] != 0) & // top
           (this->texture[i - this->y_step] != 0);  // bottom
}

glm::ivec2  Chunk::getVerticesAoValue( int i, uint8_t visibleFaces ) const {
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
        facesAo[0] = (static_cast<int>( std::min(p[10]*1.5f + p[4]  + p[3] *1.5f, 3.0f)) << 2) | // right:0
                     (static_cast<int>( std::min(p[3] *1.5f + p[2]  + p[9] *1.5f, 3.0f)) << 0) | // right:1
                     (static_cast<int>( std::min(p[9] *1.5f + p[14] + p[15]*1.5f, 3.0f)) << 4) | // right:2
                     (static_cast<int>( std::min(p[15]*1.5f + p[16] + p[10]*1.5f, 3.0f)) << 6);  // right:3
    }
    if (visibleFaces & 0x10) { // [2, 1, 3, 0]
        facesAo[1] = (static_cast<int>( std::min(p[8] *1.5f + p[0]  + p[7] *1.5f, 3.0f)) << 6) | // left:0
                     (static_cast<int>( std::min(p[7] *1.5f + p[6]  + p[11]*1.5f, 3.0f)) << 2) | // left:1
                     (static_cast<int>( std::min(p[11]*1.5f + p[18] + p[19]*1.5f, 3.0f)) << 0) | // left:2
                     (static_cast<int>( std::min(p[19]*1.5f + p[12] + p[8] *1.5f, 3.0f)) << 4);  // left:3
    }
    if (visibleFaces & 0x08) { // [2, 1, 3, 0]
        facesAo[2] = (static_cast<int>( std::min(p[11]*1.5f + p[6]  + p[5] *1.5f, 3.0f)) << 6) | // front:0
                     (static_cast<int>( std::min(p[5] *1.5f + p[4]  + p[10]*1.5f, 3.0f)) << 2) | // front:1
                     (static_cast<int>( std::min(p[10]*1.5f + p[16] + p[17]*1.5f, 3.0f)) << 0) | // front:2
                     (static_cast<int>( std::min(p[17]*1.5f + p[18] + p[11]*1.5f, 3.0f)) << 4);  // front:3
    }
    if (visibleFaces & 0x04) { // [1, 0, 2, 3]
        facesAo[3] = (static_cast<int>( std::min(p[9] *1.5f + p[2]  + p[1] *1.5f, 3.0f)) << 2) | // back:0
                     (static_cast<int>( std::min(p[1] *1.5f + p[0]  + p[8] *1.5f, 3.0f)) << 0) | // back:1
                     (static_cast<int>( std::min(p[8] *1.5f + p[12] + p[13]*1.5f, 3.0f)) << 4) | // back:2
                     (static_cast<int>( std::min(p[13]*1.5f + p[14] + p[9] *1.5f, 3.0f)) << 6);  // back:3
    }
    if (visibleFaces & 0x02) { // [3, 2, 0, 1]
        facesAo[4] = (static_cast<int>( std::min(p[7] *1.5f + p[0]  + p[1] *1.5f, 3.0f)) << 4) | // top:0
                     (static_cast<int>( std::min(p[1] *1.5f + p[2]  + p[3] *1.5f, 3.0f)) << 6) | // top:1
                     (static_cast<int>( std::min(p[3] *1.5f + p[4]  + p[5] *1.5f, 3.0f)) << 2) | // top:2
                     (static_cast<int>( std::min(p[5] *1.5f + p[6]  + p[7] *1.5f, 3.0f)) << 0);  // top:3
    }
    if (visibleFaces & 0x01) { // [1, 2, 0, 3]
        facesAo[5] = (static_cast<int>( std::min(p[19]*1.5f + p[12] + p[13]*1.5f, 3.0f)) << 4) | // bottom:0
                     (static_cast<int>( std::min(p[13]*1.5f + p[14] + p[15]*1.5f, 3.0f)) << 0) | // bottom:1
                     (static_cast<int>( std::min(p[15]*1.5f + p[16] + p[17]*1.5f, 3.0f)) << 2) | // bottom:2
                     (static_cast<int>( std::min(p[17]*1.5f + p[18] + p[19]*1.5f, 3.0f)) << 6);  // bottom:3
    }
    auto pow2 = [](int x) { return x * x; };
    /* we check the vertices ao values to determine if the quad must be flipped, and we pack it in ao.y at 0x00FF0000 */
    uint8_t flippedQuads = 0x0;
    for (int i = 0; i < 6; i++) {
        if (visibleFaces & (0x20 >> i))
            flippedQuads |= (int)(pow2((facesAo[i]&0x30)>>4) + pow2((facesAo[i]&0x0C)>>2) > pow2((facesAo[i]&0xC0)>>6) + pow2((facesAo[i]&0x03)>>0)) << (5-i);
    }
    return glm::ivec2(
        (facesAo[0] << 24) | (facesAo[1] << 16) | (facesAo[2] << 8) | (facesAo[3]),
        (flippedQuads << 16) | (facesAo[4] <<  8) | (facesAo[5])
    );
}

void    Chunk::buildMesh( void ) {
    const int m = this->margin / 2;
    this->voxels.reserve(chunkSize.x * chunkSize.y * chunkSize.z);

    for (int y = chunkSize.y-1; y >= 0; --y)
        for (int z = 0; z < chunkSize.z; ++z)
            for (int x = 0; x < chunkSize.x; ++x) {
                int i = (x+m) + (z+m) * paddedSize.x + (y+m) * this->y_step;
                int j = x + z * chunkSize.x + y * chunkSize.x * chunkSize.z;
                if (this->texture[i] != 0 && !isVoxelCulled(i)) { /* if voxel is not air and not culled */
                    uint8_t visibleFaces = getVisibleFaces(i);
                    uint8_t b = static_cast<uint8_t>(this->texture[i] - 1);
                    /* change dirt to grass on top */
                    if (texture[i] == 1 && texture[i + this->y_step] == 0 && !this->underground) // && lightMap[i + this->y_step] > 2)
                        b = 1;
                    glm::ivec2 ao = getVerticesAoValue(i, visibleFaces);
                    int light = ((int)lightMap[i + 1           ] << 20) | ((int)lightMap[i - 1           ] << 16) |
                                ((int)lightMap[i + paddedSize.x] << 12) | ((int)lightMap[i - paddedSize.x] <<  8) |
                                ((int)lightMap[i + this->y_step] <<  4) | ((int)lightMap[i - this->y_step]);
                    this->voxels.push_back( (tPoint){ glm::vec3(x, y, z), ao, b, visibleFaces, light } );
                }
            }
    this->setup(GL_STATIC_DRAW);
    this->meshed = true;
}
/*
    +---+---+---+---++---+---+---+---+
    | 15| 15|###|###|| 15|###|###|###|
    +---+---+---+ - ++---+---+---+---+
    | 15| 15|###| 12|| 15| 14| 13| 12|
    +---+---+---+ - ++---+---+---+---+
    | 15| 15| 14| 13||###|###| 12| 11|
    +---+---+---+---++ - +---+---+---+
    |###| 15| 14| 13|| 9 | 10| 11| 10|
    +---+---+---+---++ - +---+---+---+
    * The issue is that we must also update the chunk on the left when values of the right chunk have been
      computed. It results in hard cut of shadows on one side, and smooth transition on the other...
    * One way to solve that is to update them, though they are meshed, so we have to remesh them...
    * Another solution could be to have a pass for all texture generation in chunksToLoad queue,
      when that is done, we compute the light, and then we mesh all those chunks. We would still have to
      remesh the chunks at borders of new batches, but the complexity is reduced a bit. (still, it's far from optimal)

    +---+---+---+---++---+---+---+---+
    | 15| 15|###|###|| 15|###|###|###|
    +---+---+---+---++---+---+---+---+
    | 15| 15|###| 14|| 15| 14| 13| 12|
    +---+---+---+---++---+---+---+---+
    | 15| 15| 14| 13||###|###| 12| 11|
    +---+---+---+---++---+---+---+---+
    |###| 15| 14| 13|| 12| 11| 11| 10|
    +---+---+---+---++---+---+---+---+

    +---+---+---+---++---+---+---+---+
    | 15| 15|###|###|| 5 |###|###| 10|
    +---+---+---+---++---+---+---+---+
    | 15| 15|###| 12|| 6 | 7 | 8 | 9 |
    +---+---+---+---++---+---+---+---+
    | 15| 15| 14| 13||###|###| 7 | 8 |
    +---+---+---+---++---+---+---+---+
    |###| 15| 14| 13|| 4 | 5 | 6 | 7 |
    +---+---+---+---++---+---+---+---+
*/

const bool  Chunk::isBorder( int i ) {
    const int m = this->margin / 2;
    return (i % paddedSize.x < m || /* left border */
            i % paddedSize.x >= chunkSize.x + m || /* right border */
            i % this->y_step < paddedSize.x * m || /* back border */
            i % this->y_step >= this->y_step - paddedSize.x * m || /* front border */
            i % (this->y_step * paddedSize.y) < this->y_step * m || /* top border */
            i % (this->y_step * paddedSize.y) >= this->y_step * paddedSize.y - this->y_step * m); /* bottom border */
}

const bool  Chunk::isMaskZero( const uint8_t* mask ) {
    const int m = this->margin / 2;
    bool b = false;
    for (int z = -1; z < chunkSize.z+1 && !b; ++z)
        for (int x = -1; x < chunkSize.x+1 && !b; ++x) {
            int j = (x+m) + (z+m) * paddedSize.x;
            b = (mask[j] != 0);
        }
    return !b;
}

void    Chunk::computeLight( std::array<const uint8_t*, 6> neighbouringChunks, const uint8_t* aboveLightMask ) {
    const int m = this->margin / 2;
    // std::queue<int>   lightNodes;

    if (aboveLightMask != nullptr) { // the lightMask of the chunk above
        memcpy(lightMask, aboveLightMask, this->y_step); // copies the mask as current lightMask
        if (isMaskZero(aboveLightMask)) { // if no light is present, skip
            this->underground = true;
            this->lighted = true;
            return ;
        }
    }
    /* first pass (/!\ DON'T TOUCH, IT'S PERFECT) */
    for (int y = chunkSize.y; y >= 0; --y)
        for (int z = -1; z < chunkSize.z+1; ++z)
            for (int x = -1; x < chunkSize.x+1; ++x) {
                int j = (x+m) + (z+m) * paddedSize.x;
                int i = j + (y+m) * this->y_step;
                if (this->texture[i] == 0 && (this->lightMask[j] == 15) ) { /* if voxel is transparent, and voxel above also */
                    lightMask[j] = 15;
                    // lightNodes.push(i); /* optimization idea: add node only if 1 or more neighbouring voxel light is 0 (but transparent) */
                }
                else if (this->texture[i] != 0) /* if voxel is opaque */
                    lightMask[j] = 0;
                lightMap[i] = lightMask[j];
            }
    /* propagation pass */
    // while (lightNodes.empty() == false) {
    //     int index = lightNodes.front();
    //     lightNodes.pop();

    //     const std::array<int, 6> offset = { 1, -1, this->y_step, -this->y_step, paddedSize.x, -paddedSize.x };
    //     const std::array<int, 6> offsetInv = { -chunkSize.x, chunkSize.x, -this->y_step * chunkSize.y, this->y_step * chunkSize.y, -paddedSize.x * chunkSize.z, paddedSize.x * chunkSize.z };

    //     int currentLight = this->lightMap[index];

    //     for (int side = 0; side < 6; side++) {

    //         if (!isBorder(index + offset[side])) {
    //             /* if block is opaque and light value is at least 2 under current light */
    //             if (this->texture[index + offset[side]] == 0 && this->lightMap[index + offset[side]] + 2 <= currentLight) {
    //                 this->lightMap[index + offset[side]] = currentLight - 1;
    //                 lightNodes.push(index + offset[side]);
    //             }
    //         }
    //         else { /* we're on a chunk border */
    //             // std::cout << currentLight << std::endl;
    //             // neighbouringChunks[side][ (index + (paddedSize.x - x * 2)) - offset[side] ]
    //             int value = this->lightMap[index + offset[side]];
                
    //             if (neighbouringChunks[side] != nullptr && side != 2 && side != 3)
    //                 value = std::max(value, (int)neighbouringChunks[side][ (index + offsetInv[side]) - offset[side] ]);

    //             if (this->texture[index + offset[side]] == 0 && value + 2 <= currentLight) {
    //                 this->lightMap[index + offset[side]] = currentLight - 1;
    //                 lightNodes.push(index + offset[side]);
    //             }
    //         }
    //     }
    // }
    this->lighted = true;
}

void    Chunk::render( Shader shader, Camera& camera, GLuint textureAtlas, uint renderDistance ) {
    if (glm::distance(this->position * glm::vec3(1,0,1), camera.getPosition() * glm::vec3(1,0,1)) > renderDistance * 3.0f) {
        outOfRange = true;
        return;
    } else {
        outOfRange = false;
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
    /* light attribute */
    glEnableVertexAttribArray(4);
	glVertexAttribIPointer(4, 1, GL_INT, sizeof(tPoint), reinterpret_cast<GLvoid*>(offsetof(tPoint, light)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void    Chunk::createModelTransform( const glm::vec3& position ) {
    this->transform = glm::mat4();
    this->transform = glm::translate(this->transform, position);
}