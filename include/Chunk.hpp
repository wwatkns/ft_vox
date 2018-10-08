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

/* we could optimize that */
typedef struct  point_s {
    glm::vec3   position;
    glm::ivec2  ao;
    uint8_t     id;
    uint8_t     visibleFaces;
    int         light; // for now we have infos about which faces receiving light (4bits of info for 6 faces, 24bits used)
}               point_t;

typedef struct  mesh_s {
    GLuint               vao;
    GLuint               vbo;
    std::vector<point_t> voxels;
}               mesh_t;

class Chunk {

public:
    Chunk( const glm::vec3& position, const glm::ivec3& chunkSize, const uint8_t* texture, const uint margin );
    ~Chunk( void );

    void                buildMesh( void );
    void                rebuildMesh( void );

    void                computeWater( const std::array<Chunk*, 6>& neighbouringChunks );
    void                computeLight( const std::array<Chunk*, 6>& neighbouringChunks, const uint8_t* aboveLightMask );
    void                render( Shader shader, Camera& camera, GLuint textureAtlas, uint renderDistance, int underwater );
    /* getters */
    const glm::vec3&    getPosition( void ) const { return position; };
    const uint8_t*      getTexture( void ) const { return texture; };
    const uint8_t*      getLightMask( void ) const { return lightMask; };
    const uint8_t*      getLightMap( void ) const { return lightMap; };
    const int           getSidesWaterUpdate( void ) const { return sidesWaterUpdate; };
    const int           getSidesLightUpdate( void ) const { return sidesLightUpdate; };
    /* state checks */
    const bool          isMeshed( void ) const { return meshed; };
    const bool          isLighted( void ) const { return lighted; };
    const bool          isUnderground( void ) const { return underground; };
    const bool          isOutOfRange( void ) const { return outOfRange; };
    const bool          isBorder( int i );
    const bool          isMaskZero( const uint8_t* mask );


private:
    /* using heap allocated pointer to type is slightly faster, but messier (~80ms win on 800 chunks, so 0.1ms/chunk) */
    mesh_t              mesh_opaque;
    mesh_t              mesh_transparent;
    glm::mat4           transform;  /* the transform of the chunk (its world position) */
    glm::vec3           position;
    glm::ivec3          chunkSize;  /* the chunk size */
    glm::ivec3          paddedSize; /* the chunk padded size (bigger because we have adjacent bloc informations) */
    uint8_t*            texture;    /* the texture outputed by the chunk generation shader */
    uint8_t*            lightMask;  /* the light mask used for the lighting pass */
    uint8_t*            lightMap;
    uint                margin;     /* the texture margin */
    bool                meshed;
    bool                lighted;
    bool                underground;
    bool                outOfRange;
    bool                firstLightPass;
    int                 y_step;
    int                 sidesWaterUpdate;
    int                 sidesLightUpdate;

    void                setupMesh( mesh_t* mesh, int mode );

    void                createModelTransform( const glm::vec3& position );
    const bool          isVoxelTransparent( int i ) const;
    const bool          isVoxelCulled( int i ) const;
    const bool          isVoxelCulledTransparent( int i ) const;
    const uint8_t       getVisibleFaces( int i ) const;
    const uint8_t       getVisibleFacesTransparent( int i ) const;
    glm::ivec2          getVerticesAoValue( int i, uint8_t visibleFaces ) const;

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
// TODO : implement multi-threading for chunk generation and meshing (we'll see when it becomes a bottleneck)
