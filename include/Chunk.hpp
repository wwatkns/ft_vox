#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <queue>
#include <algorithm>

#include "Exception.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "utils.hpp"


typedef struct  sPoint {
    glm::vec3   position;
    glm::ivec2  ao;
    uint8_t     id;
    uint8_t     visibleFaces;
    int         light; // for now we have infos about which faces receiving light (4bits of info for 6 faces, 24bits used)
}               tPoint;

typedef struct  lightNode_s {
    int t_index;
    int l_index;
}               lightNode_t;

class Chunk {

public:
    Chunk( const glm::vec3& position, const glm::ivec3& chunkSize, const uint8_t* texture, const uint margin );
    ~Chunk( void );

    void                buildMesh( void );
    void                computeLight( const uint8_t* aboveLightMask );
    void                render( Shader shader, Camera& camera, GLuint textureAtlas, uint renderDistance );
    /* getters */
    const GLuint&       getVao( void ) const { return vao; };
    const GLuint&       getVbo( void ) const { return vbo; };
    const glm::vec3&    getPosition( void ) const { return position; };
    const uint8_t*      getTexture( void ) const { return texture; };
    const uint8_t*      getLightMask( void ) const { return lightMask; };
    /* state checks */
    const bool          isMeshed( void ) const { return meshed; };
    const bool          isLighted( void ) const { return lighted; };
    const bool          isOutOfRange( void ) const { return outOfRange; };

private:
    GLuint              vao;        // Vertex Array Object
    GLuint              vbo;        // Vertex Buffer Object
    std::vector<tPoint> voxels;     // the list of voxels created in Terrain
    glm::mat4           transform;  // the transform of the chunk (its world position)
    glm::vec3           position;
    glm::ivec3          chunkSize;  /* the chunk size */
    glm::ivec3          paddedSize; /* the chunk padded size (bigger because we have adjacent bloc informations) */
    uint8_t*            texture;    /* the texture outputed by the chunk generation shader */
    uint8_t*            lightMask;  /* the light mask used for the lighting pass */ // TMP?
    uint8_t*            lightMap;
    uint                margin;     /* the texture margin */
    bool                meshed;
    bool                lighted;
    bool                outOfRange;
    int                 y_step;

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
