#include "Terrain.hpp"
#include "glm/ext.hpp"

Terrain::Terrain( uint renderDistance, uint maxHeight, const glm::ivec3& chunkSize ) : renderDistance(renderDistance), maxHeight(maxHeight), chunkSize(chunkSize) {
    this->dataMargin = 4; // even though we only need a margin of 2, openGL does not like this number and gl_FragCoord values will be messed up...
    this->maxChunksGeneratedPerFrame = 8;
    this->setupChunkGenerationRenderingQuad();
    this->setupChunkGenerationFbo();
    this->chunkGenerationShader = new Shader("./shader/vertex/screenQuad.vert.glsl", "./shader/fragment/generateChunk.frag.glsl");
    this->noiseSampler = loadTexture("./resource/RGBAnoiseMedium.png");
    this->textureAtlas = loadTextureMipmapSrgb(std::vector<std::string>{{
        "./resource/terrain.png",
        "./resource/terrain-1.png",
        "./resource/terrain-2.png",
        "./resource/terrain-3.png",
        "./resource/terrain-4.png"
    }});
    this->dataBuffer = static_cast<uint8_t*>(malloc(sizeof(uint8_t) * this->chunkGenerationFbo.width * this->chunkGenerationFbo.height));
}

Terrain::~Terrain( void ) {
    /* clean framebuffers */
    glDeleteFramebuffers(1, &this->chunkGenerationFbo.fbo);
    /* clean textures */
    glDeleteTextures(1, &this->noiseSampler);
    glDeleteTextures(1, &this->textureAtlas);
    glDeleteTextures(1, &this->chunkGenerationFbo.id);
    /* clean buffers */
    glDeleteBuffers(1, &this->chunkGenerationRenderingQuad.vao);
    glDeleteBuffers(1, &this->chunkGenerationRenderingQuad.vbo);
    glDeleteBuffers(1, &this->chunkGenerationRenderingQuad.ebo);
    delete this->chunkGenerationShader;
    free(this->dataBuffer);
    this->dataBuffer = NULL;
}

/* return the chunk position (in chunk space, left chunk is {-1, 0, 0} ) */
glm::vec3   Terrain::getChunkPosition( const glm::vec3& position ) {
    glm::vec3 chunkPosition;
    // ceil the y position to max height
    chunkPosition.x = std::floor(position.x / (float)this->chunkSize.x);
    chunkPosition.y = std::max(std::min(std::floor(position.y / (float)this->chunkSize.y), this->maxHeight / (float)this->chunkSize.y - 1.0f), 0.0f);
    chunkPosition.z = std::floor(position.z / (float)this->chunkSize.z);
    return chunkPosition;
}

// void    Terrain::addChunksToGenerationList( const glm::vec3& cameraPosition ) {
//     /*  naive algorithm : 
//         * iterate on all the positions around player in render distance range and create chunk if it does not exist.
//     */
//     int height = this->maxHeight / this->chunkSize.y;
//     glm::ivec3 dist = glm::ivec3(this->renderDistance) / this->chunkSize;
//     for (int y = height; y >= 0; y--) /* we build a column of chunks each time */
//         for (int z = -dist.z; z < dist.z; ++z)
//             for (int x = -dist.x; x < dist.x; ++x) {
//                 /* if chunk was never generated */
//                 Key key = { this->getChunkPosition(cameraPosition) * glm::vec3(1, 0, 1) + glm::vec3(x, y, z) };
//                 if (this->chunks.find(key) == this->chunks.end() && this->chunksToGenerate.find(key) == this->chunksToGenerate.end()) {
//                     this->chunksToGenerate.insert( { key, true } );
//                 }
//             }
// }

/* allows the chunks to be drawn from top to bottom, one column of chunk at a time */
static uint64_t hashKey(const Key& k) {
    uint64_t    hash = 0;
    hash |= static_cast<uint64_t>(static_cast<int>(8-k.p.y) + 0x7FFF);
    hash |= static_cast<uint64_t>(static_cast<int>(  k.p.z) + 0x7FFF) << 16;
    hash |= static_cast<uint64_t>(static_cast<int>(  k.p.x) + 0x7FFF) << 32;
    return (hash);
}

static Key  unhashKey(const uint64_t& hash) {
    Key key;
    key.p.y = 8-(static_cast<float>((hash & 0x00000000FFFF)      ) - 0x7FFF);
    key.p.z =    static_cast<float>((hash & 0x0000FFFF0000) >> 16) - 0x7FFF;
    key.p.x =    static_cast<float>((hash & 0xFFFF00000000) >> 32) - 0x7FFF;
    return (key);
}

void    Terrain::addChunksToGenerationList( const glm::vec3& cameraPosition ) {
    /*  naive algorithm : 
        * iterate on all the positions around player in render distance range and create chunk if it does not exist.
    */
    int height = this->maxHeight / this->chunkSize.y;
    glm::ivec3 dist = glm::ivec3(this->renderDistance) / this->chunkSize;
    for (int y = height; y >= 0; y--) /* we build a column of chunks each time */
        for (int z = -dist.z; z < dist.z; ++z)
            for (int x = -dist.x; x < dist.x; ++x) {
                /* if chunk was never generated */
                Key key = { this->getChunkPosition(cameraPosition) * glm::vec3(1, 0, 1) + glm::vec3(x, y, z) };
                if (this->chunks.find(key) == this->chunks.end() && this->chunksToGenerate.find(key.p) == this->chunksToGenerate.end())
                    this->chunksToGenerate.insert( { key.p, true } );
            }
}
    
// void    Terrain::generateChunkTextures( void ) {
//     static tTimePoint last = std::chrono::steady_clock::now();
//     bool work = false;

//     std::forward_list<Key> toDelete;
//     int chunksGenerated = 0;
//     for (std::pair<Key, bool> chunk : this->chunksToGenerate) {
//         if (chunksGenerated < this->maxChunksGeneratedPerFrame) {
//             glm::vec3 position = chunk.first.p * (glm::vec3)this->chunkSize;
//             this->renderChunkGeneration(position, this->dataBuffer);
//             this->chunks.insert( { chunk.first, new Chunk(position, this->chunkSize, this->dataBuffer, this->dataMargin) } );
//             toDelete.push_front(chunk.first);
//             chunksGenerated++;
//             work = true;
//         }
//     }
//     for ( auto it = toDelete.begin(); it != toDelete.end(); ++it )
//             this->chunksToGenerate.erase( *it );
    
//     tTimePoint current = std::chrono::steady_clock::now();
//     if (work)
//         std::cout << "> generation: " << (static_cast<tMilliseconds>(current - last)).count() << " ms" << std::endl;
//     last = current;
// }

void    Terrain::generateChunkTextures( void ) {
    std::forward_list<glm::vec3> toDelete;

    int chunksGenerated = 0;
    for (auto it = this->chunksToGenerate.begin(); it != this->chunksToGenerate.end(); ++it) {
        if (chunksGenerated < this->maxChunksGeneratedPerFrame) {
            /* generate texture */
            Key key = { it->first };
            glm::vec3 position = key.p * (glm::vec3)this->chunkSize;
            this->renderChunkGeneration(position, this->dataBuffer);
            this->chunks.insert( { key, new Chunk(position, this->chunkSize, this->dataBuffer, this->dataMargin) } );
            toDelete.push_front(it->first);
            chunksGenerated++;
        }
    }
    for (auto it = toDelete.begin(); it != toDelete.end(); ++it) {
        this->chunksToGenerate.erase( *it );
    }
}

void    Terrain::generateChunkMeshes( void ) {
    static tTimePoint last = std::chrono::steady_clock::now();
    bool work = false;

    for (auto it = this->chunks.begin(); it != this->chunks.end(); ++it) {
        if (!it->second->isMeshed()) {
            it->second->buildMesh();
            work = true;
        }
    }
    /* display time */
    tTimePoint current = std::chrono::steady_clock::now();
    if (work) std::cout << "> meshing: " << (static_cast<tMilliseconds>(current - last)).count() << " ms" << std::endl;
    last = current;
}

void    Terrain::computeChunkLight( void ) {
    for (auto it = this->chunks.begin(); it != this->chunks.end(); ++it) {
        if (!it->second->isLighted()) {
            /* the lightMask is the last vertical slice of the LightMap of previous chunk (must compute it) */
            std::array<const uint8_t*, 6> neighbouringChunks = {
                (chunks.find({it->first.p + glm::vec3(1,0,0)}) != chunks.end() ? chunks[{it->first.p + glm::vec3(1,0,0)}]->getLightMap() : nullptr),
                (chunks.find({it->first.p - glm::vec3(1,0,0)}) != chunks.end() ? chunks[{it->first.p - glm::vec3(1,0,0)}]->getLightMap() : nullptr),
                (chunks.find({it->first.p + glm::vec3(0,1,0)}) != chunks.end() ? chunks[{it->first.p + glm::vec3(0,1,0)}]->getLightMap() : nullptr),
                (chunks.find({it->first.p - glm::vec3(0,1,0)}) != chunks.end() ? chunks[{it->first.p - glm::vec3(0,1,0)}]->getLightMap() : nullptr),
                (chunks.find({it->first.p + glm::vec3(0,0,1)}) != chunks.end() ? chunks[{it->first.p + glm::vec3(0,0,1)}]->getLightMap() : nullptr),
                (chunks.find({it->first.p - glm::vec3(0,0,1)}) != chunks.end() ? chunks[{it->first.p - glm::vec3(0,0,1)}]->getLightMap() : nullptr)
            };
            const uint8_t* aboveLightMask = (chunks.find({it->first.p + glm::vec3(0,1,0)}) != chunks.end() ? chunks[{it->first.p + glm::vec3(0,1,0)}]->getLightMask() : nullptr);
            it->second->computeLight(neighbouringChunks, aboveLightMask);
        }
    }
}

void    Terrain::renderChunkGeneration( const glm::vec3& position, uint8_t* data ) {
    GLint m_viewport[4];
    glGetIntegerv(GL_VIEWPORT, m_viewport);
    /* configure the framebuffer */
    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, this->chunkGenerationFbo.fbo);
    glViewport(0, 0, this->chunkGenerationFbo.width, this->chunkGenerationFbo.height);
    glClear(GL_COLOR_BUFFER_BIT);

    /* use and set uniform for chunk generation shader */
    this->chunkGenerationShader->use();
    this->chunkGenerationShader->setFloatUniformValue("near", 0.1f); // get near from camera
    this->chunkGenerationShader->setVec3UniformValue("chunkPosition", position);
    this->chunkGenerationShader->setVec3UniformValue("chunkSize", glm::vec3(this->chunkSize + (int)this->dataMargin) );
    this->chunkGenerationShader->setFloatUniformValue("margin", (float)this->dataMargin );
    glActiveTexture(GL_TEXTURE0);
    this->chunkGenerationShader->setIntUniformValue("noiseSampler", 0);
    glBindTexture(GL_TEXTURE_2D, this->noiseSampler);

    /* render quad */
    glBindVertexArray(this->chunkGenerationRenderingQuad.vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);

    /* read the pixels from the framebuffer */
    glReadPixels(0, 0, this->chunkGenerationFbo.width, this->chunkGenerationFbo.height, GL_RED, GL_UNSIGNED_BYTE, data);
    /* reset the framebuffer target and size */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
    glEnable(GL_DEPTH_TEST);
}

void    Terrain::renderChunks( Shader shader, Camera& camera ) {
    for (std::pair<Key, Chunk*> chunk : this->chunks) {
        chunk.second->render(shader, camera, this->textureAtlas, renderDistance);
    }
}

void    Terrain::updateChunks( const glm::vec3& cameraPosition ) {
    this->addChunksToGenerationList(cameraPosition);
    this->generateChunkTextures();
    this->computeChunkLight();
    this->generateChunkMeshes();
}

void    Terrain::setupChunkGenerationRenderingQuad( void ) {
    /* create quad */
    std::vector<tVertex>    vertices;
    std::vector<float>      quad = {{
        -1.0,-1.0, 0.0,  0.0, 1.0, // top-left
         1.0,-1.0, 0.0,  1.0, 1.0, // top-right
         1.0, 1.0, 0.0,  1.0, 0.0, // bottom-right
        -1.0, 1.0, 0.0,  0.0, 0.0  // bottom-left
    }};
    std::vector<unsigned int>   indices = {{
        0, 1, 2,  2, 3, 0
    }};
    for (size_t i = 5; i < quad.size()+1; i += 5) {
        tVertex vertex;
        vertex.Position = glm::vec3(quad[i-5], quad[i-4], quad[i-3]);
        vertex.TexCoords = glm::vec2(quad[i-2], quad[i-1]);
        vertices.push_back(vertex);
    }
    /* create openGL bindings */
	glGenVertexArrays(1, &this->chunkGenerationRenderingQuad.vao);
    glGenBuffers(1, &this->chunkGenerationRenderingQuad.vbo);
	glGenBuffers(1, &this->chunkGenerationRenderingQuad.ebo);
	glBindVertexArray(this->chunkGenerationRenderingQuad.vao);
	glBindBuffer(GL_ARRAY_BUFFER, this->chunkGenerationRenderingQuad.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(tVertex), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->chunkGenerationRenderingQuad.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    // position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(tVertex), static_cast<GLvoid*>(0));
    // texture coord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(tVertex), reinterpret_cast<GLvoid*>(offsetof(tVertex, TexCoords)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void    Terrain::setupChunkGenerationFbo( void ) {
    this->chunkGenerationFbo.width = (this->chunkSize.x + this->dataMargin);
    this->chunkGenerationFbo.height = (this->chunkSize.y + this->dataMargin) * (this->chunkSize.z + this->dataMargin);

    glGenFramebuffers(1, &this->chunkGenerationFbo.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, this->chunkGenerationFbo.fbo);
    /* create color texture */
    glGenTextures(1, &this->chunkGenerationFbo.id);
    glBindTexture(GL_TEXTURE_2D, this->chunkGenerationFbo.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, this->chunkGenerationFbo.width, this->chunkGenerationFbo.height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->chunkGenerationFbo.id, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
