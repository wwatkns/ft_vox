#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <algorithm>

#include "Exception.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "utils.hpp"


typedef struct  sPoint {
    glm::vec3   position;
    uint8_t     id;
    uint8_t     visibleFaces;
    glm::ivec2  ao;
}               tPoint;

class Chunk {

public:
    Chunk( const glm::vec3& position, const glm::ivec3& size, const uint8_t* texture, const uint margin );
    ~Chunk( void );

    void                buildMesh( void );
    void                render( Shader shader, Camera& camera, GLuint textureAtlas, uint renderDistance );
    /* getters */
    const GLuint&       getVao( void ) const { return vao; };
    const GLuint&       getVbo( void ) const { return vbo; };
    const glm::vec3&    getPosition( void ) const { return position; };
    const uint8_t*      getTexture( void ) const { return texture; };
    const bool          isMeshed( void ) const { return meshed; };
    const bool          isOutOfRange( void ) const { return outOfRange; };

private:
    GLuint              vao;        // Vertex Array Object
    GLuint              vbo;        // Vertex Buffer Object
    std::vector<tPoint> voxels;     // the list of voxels created in Terrain
    glm::mat4           transform;  // the transform of the chunk (its world position)
    glm::vec3           position;
    glm::ivec3          size;
    uint8_t*            texture;    /* the texture outputed by the chunk generation shader */
    uint8_t*            textureExtras; /* the additional info in the texture outputed by the chunk generation shader */
    uint                margin; /* the texture margin */
    bool                meshed;
    bool                outOfRange;

    void                setup( int mode );
    void                createModelTransform( const glm::vec3& position );
    bool                isVoxelCulled( int i );
    uint8_t             getVisibleFaces( int i );
    glm::ivec2          getVerticesAoValue( int i, uint8_t visibleFaces );

};

/*  Mesh creation optimisations :
    * don't save empty voxels
    * voxel occlusion (don't save/render voxels that are surrounded by solid voxels)
    * back-face occlusion (don't render faces not facing the camera)
    * faces interior occlusion (don't render faces that have an adjacent voxel)
    
    Rendering optimisations :
    * view fustrum chunk occlusion (don't render chunks outside the camera fustrum)
    * don't render empty chunks
*/

// TODO : implement occlusion culling (don't render chunks that are occluded entirely by other chunks)
// TODO : implement front to back rendering (so we keep the chunk in some kind of tree to know which ones are the closest)
// TODO : implement multi-threading for chunk generation and meshing (we'll see when it becomes a bottleneck)

// TODO : chunk management : have a chunksToLoad list, and a maximum number of chunks to generate per frame, that way we can
//        avoid having big framerate hit in some situations

    // if (visibleFaces & 0x20) {
    //     ao_xz |= ((int)((p & 0x0F) != 0) + (int)((p & 0x10) != 0) + (int)((p & 0x09) != 0)) << 30; // right:0 !
    //     ao_xz |= ((int)((p & 0x10) != 0) + (int)((p & 0x11) != 0) + (int)((p & 0x0A) != 0)) << 28; // right:1 !
    //     ao_xz |= ((int)((p & 0x0A) != 0) + (int)((p & 0x05) != 0) + (int)((p & 0x04) != 0)) << 26; // right:2 !
    //     ao_xz |= ((int)((p & 0x09) != 0) + (int)((p & 0x04) != 0) + (int)((p & 0x03) != 0)) << 24; // right:3 !
    // }
    // if (visibleFaces & 0x10) {
    //     ao_xz |= ((int)((p & 0x0B) != 0) + (int)((p & 0x13) != 0) + (int)((p & 0x0C) != 0)) << 22; // left:0 !
    //     ao_xz |= ((int)((p & 0x0C) != 0) + (int)((p & 0x0D) != 0) + (int)((p & 0x08) != 0)) << 20; // left:1 !
    //     ao_xz |= ((int)((p & 0x08) != 0) + (int)((p & 0x01) != 0) + (int)((p & 0x00) != 0)) << 18; // left:2 !
    //     ao_xz |= ((int)((p & 0x00) != 0) + (int)((p & 0x07) != 0) + (int)((p & 0x0B) != 0)) << 16; // left:3 !
    // }
    // if (visibleFaces & 0x08) {
    //     ao_xz |= ((int)((p & 0x08) != 0) + (int)((p & 0x0D) != 0) + (int)((p & 0x0E) != 0)) << 14; // front:0 !
    //     ao_xz |= ((int)((p & 0x0E) != 0) + (int)((p & 0x0F) != 0) + (int)((p & 0x09) != 0)) << 12; // front:1 !
    //     ao_xz |= ((int)((p & 0x09) != 0) + (int)((p & 0x03) != 0) + (int)((p & 0x02) != 0)) << 10; // front:2 !
    //     ao_xz |= ((int)((p & 0x08) != 0) + (int)((p & 0x01) != 0) + (int)((p & 0x02) != 0)) <<  8; // front:3 !
    // }
    // if (visibleFaces & 0x04) {
    //     ao_xz |= ((int)((p & 0x0A) != 0) + (int)((p & 0x11) != 0) + (int)((p & 0x12) != 0)) <<  6; // back:0 !
    //     ao_xz |= ((int)((p & 0x0B) != 0) + (int)((p & 0x13) != 0) + (int)((p & 0x12) != 0)) <<  4; // back:1 !
    //     ao_xz |= ((int)((p & 0x0B) != 0) + (int)((p & 0x07) != 0) + (int)((p & 0x06) != 0)) <<  2; // back:2 !
    //     ao_xz |= ((int)((p & 0x0A) != 0) + (int)((p & 0x05) != 0) + (int)((p & 0x06) != 0));       // back:3 !
    // }
    // if (visibleFaces & 0x02) {
    //     ao_y  |= ((int)((p & 0x0C) != 0) + (int)((p & 0x13) != 0) + (int)((p & 0x12) != 0)) << 14; // top:0
    //     ao_y  |= ((int)((p & 0x12) != 0) + (int)((p & 0x11) != 0) + (int)((p & 0x10) != 0)) << 12; // top:1
    //     ao_y  |= ((int)((p & 0x10) != 0) + (int)((p & 0x0F) != 0) + (int)((p & 0x0E) != 0)) << 10; // top:2
    //     ao_y  |= ((int)((p & 0x0E) != 0) + (int)((p & 0x0D) != 0) + (int)((p & 0x0C) != 0)) <<  8; // top:3
    // }
    // if (visibleFaces & 0x01) {
    //     ao_y  |= ((int)((p & 0x00) != 0) + (int)((p & 0x01) != 0) + (int)((p & 0x02) != 0)) <<  6; // bottom:0
    //     ao_y  |= ((int)((p & 0x02) != 0) + (int)((p & 0x03) != 0) + (int)((p & 0x04) != 0)) <<  4; // bottom:1
    //     ao_y  |= ((int)((p & 0x04) != 0) + (int)((p & 0x05) != 0) + (int)((p & 0x06) != 0)) <<  2; // bottom:2
    //     ao_y  |= ((int)((p & 0x06) != 0) + (int)((p & 0x07) != 0) + (int)((p & 0x00) != 0));       // bottom:3
    // }