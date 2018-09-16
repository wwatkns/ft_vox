#include "Terrain.hpp"
#include "glm/ext.hpp"

Terrain::Terrain( uint8_t renderDistance, const glm::vec3& chunkSize ) : renderDistance(renderDistance), chunkSize(chunkSize) {
    this->update();

    /* Generate all the chunks around player */
    for (uint8_t z = 0; z < renderDistance; ++z)
        for (uint8_t x = 0; x < renderDistance; ++x) {
            this->generateChunk(glm::vec3(x * this->chunkSize.x, 0, z * this->chunkSize.z)); // TODO: handle y axis also
            // std::cout << "generating chunk : [" << static_cast<int>(x) << ", " << static_cast<int>(z) << "]\n";
        }
}

Terrain::~Terrain( void ) {
}

/* Chunks will be created relative to their world position */
void    Terrain::generateChunk( const glm::vec3& position ) {
    std::vector<tPoint> voxels;
    // generate voxels
    // voxels.push_back( { glm::vec3(0, 0, 0) } );
    for (int z = 0; z < this->chunkSize.z; ++z)
        for (int y = 0; y < this->chunkSize.y; ++y)
            for (int x = 0; x < this->chunkSize.x; ++x)
                voxels.push_back( { glm::vec3(x, y, z) } );

    this->chunks.push_back( new Chunk(voxels, position) );
}

void    Terrain::render( Shader shader ) {
    this->update();
    for (unsigned int i = 0; i < this->chunks.size(); ++i)
        this->chunks[i]->render(shader);
}

void    Terrain::update( void ) {

}
