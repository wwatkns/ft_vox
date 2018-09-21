#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <array>

#include "Exception.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "utils.hpp"

typedef struct  sPoint {
    glm::vec3   position;
    uint8_t     id;
    uint8_t     visibleFaces;
}               tPoint;

class Chunk {

public:
    Chunk( const glm::vec3& position, const glm::ivec3& size, const uint8_t* texture );
    ~Chunk( void );

    void                buildMesh( const std::array<const uint8_t*, 6>& adjacentChunks );
    void                render( Shader shader, Camera& camera );
    /* getters */
    const GLuint&       getVao( void ) const { return vao; };
    const GLuint&       getVbo( void ) const { return vbo; };
    const glm::vec3&    getPosition( void ) const { return position; };
    const uint8_t*      getTexture( void ) const { return texture; };
    const bool          isMeshed( void ) const { return !voxels.empty(); };

private:
    GLuint              vao;        // Vertex Array Object
    GLuint              vbo;        // Vertex Buffer Object
    std::vector<tPoint> voxels;     // the list of voxels created in Terrain
    glm::mat4           transform;  // the transform of the chunk (its world position)
    glm::vec3           position;
    glm::ivec3          size;
    uint8_t*            texture;    /* the texture outputed by the chunk generation shader */

    void                setup( int mode );
    void                createModelTransform( const glm::vec3& position );
    bool                isVoxelCulled( int x, int y, int z, int i, const std::array<const uint8_t*, 6>& adjacentChunks );
    uint8_t             getVisibleFaces( int x, int y, int z, int i, const std::array<const uint8_t*, 6>& adjacentChunks );

};

/*
    Mesh creation optimisations :
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