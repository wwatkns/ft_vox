#include "Chunk.hpp"
#include "glm/ext.hpp"

Chunk::Chunk( const glm::vec3& position, const glm::ivec3& chunkSize, const uint8_t* texture, const uint margin ) : position(position), chunkSize(chunkSize), margin(margin), meshed(false), lighted(false), underground(false), outOfRange(false) {
    this->createModelTransform(position);
    this->paddedSize = chunkSize + static_cast<int>(margin);
    this->y_step = paddedSize.x * paddedSize.z;
    this->sidesWaterUpdate = 0;
    this->sidesLightUpdate = 0;
    this->firstLightPass = true;

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
    this->mesh_opaque.voxels.clear();
    this->mesh_transparent.voxels.clear();
    free(this->texture);
    this->texture = nullptr;
    free(this->lightMask);
    this->lightMask = nullptr;
    free(this->lightMap);
    this->lightMap = nullptr;
    glDeleteBuffers(1, &this->mesh_opaque.vao);
    glDeleteBuffers(1, &this->mesh_opaque.vbo);
    glDeleteBuffers(1, &this->mesh_transparent.vao);
    glDeleteBuffers(1, &this->mesh_transparent.vbo);
}

const bool  Chunk::isVoxelTransparent( int i ) const {
    return (this->texture[i] == 0 || this->texture[i] == 15);
}

const uint8_t Chunk::getVisibleFaces( int i ) const {
    return (isVoxelTransparent(i + 1           ) << 5) | // right
           (isVoxelTransparent(i - 1           ) << 4) | // left
           (isVoxelTransparent(i + paddedSize.x) << 3) | // front
           (isVoxelTransparent(i - paddedSize.x) << 2) | // back
           (isVoxelTransparent(i + this->y_step) << 1) | // top
           (isVoxelTransparent(i - this->y_step) << 0);  // bottom
}

const bool    Chunk::isVoxelCulled( int i ) const {
    return (isVoxelTransparent(i + 1           ) == false) & // right
           (isVoxelTransparent(i - 1           ) == false) & // left
           (isVoxelTransparent(i + paddedSize.x) == false) & // front
           (isVoxelTransparent(i - paddedSize.x) == false) & // back
           (isVoxelTransparent(i + this->y_step) == false) & // top
           (isVoxelTransparent(i - this->y_step) == false);  // bottom
}

const uint8_t Chunk::getVisibleFacesTransparent( int i ) const {
    return ((this->texture[i + 1           ] == 0) << 5) | // right
           ((this->texture[i - 1           ] == 0) << 4) | // left
           ((this->texture[i + paddedSize.x] == 0) << 3) | // front
           ((this->texture[i - paddedSize.x] == 0) << 2) | // back
           ((this->texture[i + this->y_step] == 0) << 1) | // top
           ((this->texture[i - this->y_step] == 0) << 0);  // bottom
}

const bool    Chunk::isVoxelCulledTransparent( int i ) const {
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
        !isVoxelTransparent(i - x_step - z_step + y_step), //    top:0, 0
        !isVoxelTransparent(i -          z_step + y_step), //    top:1, 1
        !isVoxelTransparent(i + x_step - z_step + y_step), //    top:2, 2
        !isVoxelTransparent(i + x_step +          y_step), //    top:3, 3
        !isVoxelTransparent(i + x_step + z_step + y_step), //    top:4, 4
        !isVoxelTransparent(i +          z_step + y_step), //    top:5, 5
        !isVoxelTransparent(i - x_step + z_step + y_step), //    top:6, 6
        !isVoxelTransparent(i - x_step +          y_step), //    top:7, 7
        /* middle */
        !isVoxelTransparent(i - x_step - z_step         ), // middle:0, 8
        !isVoxelTransparent(i + x_step - z_step         ), // middle:1, 9
        !isVoxelTransparent(i + x_step + z_step         ), // middle:2, 10
        !isVoxelTransparent(i - x_step + z_step         ), // middle:3, 11
        /* bottom */
        !isVoxelTransparent(i - x_step - z_step - y_step), // bottom:0, 12
        !isVoxelTransparent(i -          z_step - y_step), // bottom:1, 13
        !isVoxelTransparent(i + x_step - z_step - y_step), // bottom:2, 14
        !isVoxelTransparent(i + x_step -          y_step), // bottom:3, 15
        !isVoxelTransparent(i + x_step + z_step - y_step), // bottom:4, 16
        !isVoxelTransparent(i +          z_step - y_step), // bottom:5, 17
        !isVoxelTransparent(i - x_step + z_step - y_step), // bottom:6, 18
        !isVoxelTransparent(i - x_step -          y_step)  // bottom:7, 19
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

void    Chunk::rebuildMesh( void ) {
    if (this->meshed == true) {
        this->mesh_opaque.voxels.clear();
        this->mesh_transparent.voxels.clear();
        glDeleteBuffers(1, &this->mesh_opaque.vao);
        glDeleteBuffers(1, &this->mesh_opaque.vbo);
        glDeleteBuffers(1, &this->mesh_transparent.vao);
        glDeleteBuffers(1, &this->mesh_transparent.vbo);
    }
    this->buildMesh();
}

void    Chunk::buildMesh( void ) {
    const int m = this->margin / 2;
    this->mesh_opaque.voxels.reserve(chunkSize.x * chunkSize.y * chunkSize.z);
    this->mesh_transparent.voxels.reserve(chunkSize.x * chunkSize.y * chunkSize.z);

    for (int y = chunkSize.y-1; y >= 0; --y)
        for (int z = 0; z < chunkSize.z; ++z)
            for (int x = 0; x < chunkSize.x; ++x) {
                int i = (x+m) + (z+m) * paddedSize.x + (y+m) * this->y_step;
                if (!isVoxelTransparent(i) && !isVoxelCulled(i)) { /* if voxel is not transparent and not culled */
                    uint8_t visibleFaces = getVisibleFaces(i);
                    uint8_t b = static_cast<uint8_t>(this->texture[i] - 1);
                    /* change dirt to grass on top */
                    if (texture[i] == 1 && texture[i + this->y_step] == 0 && lightMap[i + this->y_step] > 1)
                        b = 1;
                    glm::ivec2 ao = getVerticesAoValue(i, visibleFaces);
                    int light = ((int)lightMap[i + 1           ] << 20) | ((int)lightMap[i - 1           ] << 16) |
                                ((int)lightMap[i + paddedSize.x] << 12) | ((int)lightMap[i - paddedSize.x] <<  8) |
                                ((int)lightMap[i + this->y_step] <<  4) | ((int)lightMap[i - this->y_step]);
                    // extra bit debug (faces underwater, 6bits)
                    if (texture[i - this->y_step] == 15) light |= (0x1 << 24); // bottom
                    if (texture[i + this->y_step] == 15) light |= (0x1 << 25); // top
                    if (texture[i - paddedSize.x] == 15) light |= (0x1 << 26); // back
                    if (texture[i + paddedSize.x] == 15) light |= (0x1 << 27); // front
                    if (texture[i - 1           ] == 15) light |= (0x1 << 28); // left
                    if (texture[i + 1           ] == 15) light |= (0x1 << 29); // right
                    this->mesh_opaque.voxels.push_back( (point_t){ glm::vec3(x, y, z), ao, b, visibleFaces, light } );
                    // add voxel bit mod, for modifiers like underwater (to paint voxel in blue)
                }
                else if (this->texture[i] == 15 && !isVoxelCulledTransparent(i)) { /* if voxel is water */
                    uint8_t visibleFaces = 0x03; // we won't see the surface from under
                    uint8_t b = static_cast<uint8_t>(this->texture[i] - 1);
                    glm::ivec2 ao = getVerticesAoValue(i, visibleFaces);
                    int light = ((int)lightMap[i + 1           ] << 20) | ((int)lightMap[i - 1           ] << 16) |
                                ((int)lightMap[i + paddedSize.x] << 12) | ((int)lightMap[i - paddedSize.x] <<  8) |
                                ((int)lightMap[i + this->y_step] <<  4) | ((int)lightMap[i - this->y_step]);
                    this->mesh_transparent.voxels.push_back( (point_t){ glm::vec3(x, y, z), ao, b, visibleFaces, light } );
                }
            }
    this->setupMesh(&this->mesh_opaque, GL_STATIC_DRAW);
    this->setupMesh(&this->mesh_transparent, GL_STATIC_DRAW);
    this->meshed = true;
}

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

void    Chunk::computeLight( const std::array<Chunk*, 6>& neighbouringChunks, const uint8_t* aboveLightMask ) {
    const int m = this->margin / 2;
    std::queue<int>   lightNodes;

    if (this->firstLightPass == true) { /* only do on first pass */
        if (aboveLightMask != nullptr) {
            memcpy(lightMask, aboveLightMask, this->y_step);
            if (isMaskZero(aboveLightMask)) { // if no light is present, skip
                this->underground = true;
                this->lighted = true;
                this->firstLightPass = false;
                return ;
            }
        }
        /* first pass */
        for (int y = chunkSize.y; y >= 0; --y)
            for (int z = -1; z < chunkSize.z+1; ++z)
                for (int x = -1; x < chunkSize.x+1; ++x) {
                    int j = (x+m) + (z+m) * paddedSize.x;
                    int i = j + (y+m) * this->y_step;
                    /* if voxel is transparent, has full light and has a transparent neighbour with no light */
                    if (texture[i             ] == 0 && (this->lightMask[j] == 15) && (
                       (texture[i+1           ] == 0 && (this->lightMask[j+1           ] == 0)) ||
                       (texture[i-1           ] == 0 && (this->lightMask[j-1           ] == 0)) ||
                       (texture[i+paddedSize.x] == 0 && (this->lightMask[j+paddedSize.x] == 0)) ||
                       (texture[i-paddedSize.x] == 0 && (this->lightMask[j-paddedSize.x] == 0))))
                    {
                        lightMask[j] = 15;
                        lightNodes.push(i);
                    }
                    else if (texture[i] == 0 && (this->lightMask[j] == 15))
                        lightMask[j] = 15;
                    else if (texture[i] == 15 && (this->lightMask[j] == 15)) { /* create nodes on water surface */
                        lightMask[j] = lightMask[j]-1;
                        lightNodes.push(i);
                    }
                    else /* if voxel is opaque */
                        lightMask[j] = 0;
                    lightMap[i] = lightMask[j];
                }
    }
    const std::array<int, 6> offset = { 1, -1, this->y_step, -this->y_step, paddedSize.x, -paddedSize.x };
    const std::array<int, 6> offsetInv = { -chunkSize.x, chunkSize.x, -this->y_step * chunkSize.y, this->y_step * chunkSize.y, -paddedSize.x * chunkSize.z, paddedSize.x * chunkSize.z };
    /* create nodes from neighbouring chunks */
    for (int y = chunkSize.y; y >= -1; --y)
        for (int z = -1; z < chunkSize.z+1; ++z)
            for (int x = -1; x < chunkSize.x+1; ++x) {
                int i = (x+m) + (z+m) * paddedSize.x + (y+m) * this->y_step;
                if (x == -1 || y == -1 || z == -1 || x == chunkSize.x || y == chunkSize.y || z == chunkSize.z) {
                    int side = 6;
                    if (x == chunkSize.x) side = 0; else if (x == -1) side = 1;
                    if (y == chunkSize.y) side = 2;
                    if (z == chunkSize.z) side = 4; else if (z == -1) side = 5;

                    if (neighbouringChunks[side] != nullptr && side != 6) {
                        int currentLight = (int)neighbouringChunks[side]->getLightMap()[i + offsetInv[side] + offset[side]];
                        if (isVoxelTransparent(i) && this->lightMap[i] + 2 <= currentLight) {
                            this->lightMap[i] = currentLight - 1;
                            lightNodes.push(i);
                        }
                    }
                }
            }
    this->sidesLightUpdate = 0;
    /* propagation pass */
    while (lightNodes.empty() == false) {
        int index = lightNodes.front();
        lightNodes.pop();
        int currentLight = this->lightMap[index];
        for (int side = 0; side < 6; side++) {
            /* if block is transparent and light value is at least 2 under current light */
            if (isVoxelTransparent(index + offset[side]) && this->lightMap[index + offset[side]] + 2 <= currentLight) {
                this->lightMap[index + offset[side]] = currentLight - 1;
                lightNodes.push(index + offset[side]);
                /* set bits for sides that are updated */
                if (isBorder(index + offset[side]))
                    this->sidesLightUpdate |= (0x1 << side);
            }
        }
    }
    this->lighted = true;
    this->firstLightPass = false;
}

void    Chunk::computeWater( const std::array<Chunk*, 6>& neighbouringChunks ) {
    const int m = this->margin / 2;
    std::queue<int>   waterNodes;

    const std::array<int, 6> offset = { 1, -1, this->y_step, -this->y_step, paddedSize.x, -paddedSize.x };
    const std::array<int, 6> offsetInv = { -chunkSize.x, chunkSize.x, -this->y_step * chunkSize.y, this->y_step * chunkSize.y, -paddedSize.x * chunkSize.z, paddedSize.x * chunkSize.z };

    /* initial pass to add nodes generated in texture */
    for (int y = chunkSize.y; y >= 0; --y)
        for (int z = -1; z < chunkSize.z+1; ++z)
            for (int x = -1; x < chunkSize.x+1; ++x) {
                int i = (x+m) + (z+m) * paddedSize.x + (y+m) * this->y_step;
                /* handle neighbouring chunks */
                if (x == -1 || y == -1 || z == -1 || x == chunkSize.x || y == chunkSize.y || z == chunkSize.z) {
                    int side = 6;
                    if (x == chunkSize.x) side = 0; else if (x == -1) side = 1;
                    if (y == chunkSize.y) side = 2; else if (y == -1) side = 3;
                    if (z == chunkSize.z) side = 4; else if (z == -1) side = 5;

                    if (neighbouringChunks[side] != nullptr && side < 6) {
                        if ((int)neighbouringChunks[side]->getTexture()[i + offsetInv[side]] == 15) {
                            this->texture[i] = 15;
                            waterNodes.push(i);
                        }
                    }
                }
                /* if voxel is water and at least one neighbouring voxel is air */
                else if ((this->texture[i             ] == 15) && (
                          this->texture[i+1           ] == 0 ||
                          this->texture[i-1           ] == 0 ||
                          this->texture[i+paddedSize.x] == 0 ||
                          this->texture[i-paddedSize.x] == 0 ||
                          this->texture[i-this->y_step] == 0 ))
                    waterNodes.push(i);
            }
    this->sidesWaterUpdate = 0;
    /* propagation pass */
    while (waterNodes.empty() == false) {
        int index = waterNodes.front();
        waterNodes.pop();
        for (int side = 0; side < 6; side++) {
            if (side != 2 && this->texture[index + offset[side]] == 0) { /* propagate water on air blocks */
                this->texture[index + offset[side]] = 15;
                waterNodes.push(index + offset[side]);
                /* set sides that were updated (to propagate to neighbours) */
                if (isBorder(index + offset[side]))
                    this->sidesWaterUpdate |= (0x1 << side);
            }
        }
    }
}

void    Chunk::render( Shader shader, Camera& camera, GLuint textureAtlas, uint renderDistance, int underwater ) {
    float distHorizontal = glm::distance(this->position * glm::vec3(1,0,1), camera.getPosition() * glm::vec3(1,0,1));
    if (distHorizontal > renderDistance * 3.0f) {
        outOfRange = true;
        return;
    }
    glm::vec3 size = this->chunkSize;
    if (camera.aabInFustrum(-(this->position + size / 2), size) && this->texture && distHorizontal - 16 <= renderDistance) {
        /* set transform matrix */
        shader.setMat4UniformValue("_mvp", camera.getViewProjectionMatrix() * this->transform);
        shader.setMat4UniformValue("_model", this->transform);
        shader.setIntUniformValue("underwater", underwater);
        /* texture atlas */
        glActiveTexture(GL_TEXTURE0);
        shader.setIntUniformValue("atlas", 0);
        glBindTexture(GL_TEXTURE_2D, textureAtlas);

        /* render opaque */
        if (this->mesh_opaque.voxels.size() > 0) {
            glBindVertexArray(this->mesh_opaque.vao);
            glDrawArrays(GL_POINTS, 0, this->mesh_opaque.voxels.size());
            glBindVertexArray(0);
        }
        /* render transparent */
        if (this->mesh_transparent.voxels.size() > 0) {
            /* perform small offset of mesh to have waterline a bit lower */
            glm::mat4 newTransform = this->transform;
            // newTransform = glm::translate(newTransform, glm::vec3(0, -0.25, 0)); /* ISSUE: with face culling if we offset down or up */
            newTransform = glm::translate(newTransform, glm::vec3(0, underwater, 0)); /* HACK: back faces are off by 1 unit down, so if we're underwater, we raise water voxels by one so that water line is at "correct" height */
            shader.setMat4UniformValue("_mvp", camera.getViewProjectionMatrix() * newTransform);
            shader.setMat4UniformValue("_model", newTransform);

            glBindVertexArray(this->mesh_transparent.vao);
            glDrawArrays(GL_POINTS, 0, this->mesh_transparent.voxels.size());
            glBindVertexArray(0);
        }
    }
}

void    Chunk::setupMesh( mesh_t* mesh, int mode ) {
    /* new for transparent */
    glGenVertexArrays(1, &mesh->vao);
    glGenBuffers(1, &mesh->vbo);
	glBindVertexArray(mesh->vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh->voxels.size() * sizeof(point_t), mesh->voxels.data(), mode);
    /* position attribute */
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(point_t), static_cast<GLvoid*>(0));
    /* ao attribute */
    glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 2, GL_INT, sizeof(point_t), reinterpret_cast<GLvoid*>(offsetof(point_t, ao)));
    /* id attribute */
    glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(point_t), reinterpret_cast<GLvoid*>(offsetof(point_t, id)));
    /* occluded faces attribute */
    glEnableVertexAttribArray(3);
	glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE, sizeof(point_t), reinterpret_cast<GLvoid*>(offsetof(point_t, visibleFaces)));
    /* light attribute */
    glEnableVertexAttribArray(4);
	glVertexAttribIPointer(4, 1, GL_INT, sizeof(point_t), reinterpret_cast<GLvoid*>(offsetof(point_t, light)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void    Chunk::createModelTransform( const glm::vec3& position ) {
    this->transform = glm::mat4();
    this->transform = glm::translate(this->transform, position);
}