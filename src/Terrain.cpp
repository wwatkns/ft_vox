#include "Terrain.hpp"
#include "glm/ext.hpp"

Terrain::Terrain( uint renderDistance, uint maxHeight ) : renderDistance(renderDistance), maxHeight(maxHeight) {
    this->chunkSize = glm::ivec3(32);
    this->dataMargin = 4; // even though we only need a margin of 2, openGL does not like this number and gl_FragCoord values will be messed up...
    this->maxAllocatedTimePerFrame = 24.0;//ms
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
    this->dataBuffer = nullptr;
}

const std::array<glm::vec3, 6> neighboursOffsets = {
    glm::vec3( 1, 0, 0), glm::vec3(-1, 0, 0), glm::vec3( 0, 1, 0), glm::vec3( 0,-1, 0), glm::vec3( 0, 0, 1), glm::vec3( 0, 0,-1)
};

/* return the chunk position (in chunk space, left chunk is {-1, 0, 0} ) */
const glm::vec3   Terrain::getChunkPosition( const glm::vec3& position ) const {
    glm::vec3 chunkPosition;
    // ceil the y position to max height
    chunkPosition.x = std::floor(position.x / (float)this->chunkSize.x);
    chunkPosition.y = std::max(std::min(std::floor(position.y / (float)this->chunkSize.y), this->maxHeight / (float)this->chunkSize.y - 1.0f), 0.0f);
    chunkPosition.z = std::floor(position.z / (float)this->chunkSize.z);
    return chunkPosition;
}

/* return the neighbouring chunks given a chunk position */
const std::array<Chunk*, 6>   Terrain::getNeighbouringChunks( const glm::vec3& position ) const {
    return {
        (chunks.find({position+glm::vec3(1,0,0)}) != chunks.end() ? chunks.at({position+glm::vec3(1,0,0)}) : nullptr),
        (chunks.find({position-glm::vec3(1,0,0)}) != chunks.end() ? chunks.at({position-glm::vec3(1,0,0)}) : nullptr),
        (chunks.find({position+glm::vec3(0,1,0)}) != chunks.end() ? chunks.at({position+glm::vec3(0,1,0)}) : nullptr),
        (chunks.find({position-glm::vec3(0,1,0)}) != chunks.end() ? chunks.at({position-glm::vec3(0,1,0)}) : nullptr),
        (chunks.find({position+glm::vec3(0,0,1)}) != chunks.end() ? chunks.at({position+glm::vec3(0,0,1)}) : nullptr),
        (chunks.find({position-glm::vec3(0,0,1)}) != chunks.end() ? chunks.at({position-glm::vec3(0,0,1)}) : nullptr)
    };
}

/*   - - - -+---+- - - - 
    |   |   | 7 |   |   |
     - -+---+---+---+- - 
    |   | 6 | 2 | 8 |   |
    +---+---+---+---+---+
    | 5 | 1 | 0 | 3 | 9 |
    +---+---+---+---+---+
    |   | 12| 4 | 10|   |
     - -+---+---+---+- - 
    |   |   | 11|   |   |
     - - - -+---+- - - -  */
void    Terrain::addChunksToGenerationList( const glm::vec3& cameraPosition ) {
    int height = this->maxHeight / this->chunkSize.y;
    glm::ivec3 dist = glm::ivec3(this->renderDistance) / this->chunkSize + 3;
    const float e = 0.01;

    for (int y = height-1; y >= 0; y--) {
        for (int d = 0; d <= dist.x; d++) {
            float d2 = 2.*d; float d4 = 4.*d;
            if (d == 0) d4 = d2 = 0.5;
            for (int xz = 0; xz < d4; xz++) {
                float x_triangleWave = 2.*std::abs( std::round(0.5*( ((xz+e)/d2) - (1./d4)      )) - 0.5*( ((xz+e)/d2) - (1./d4)      ) ) * d2;
                float z_triangleWave = 2.*std::abs( std::round(0.5*( ((xz+e)/d2) - (1./d4) + 0.5)) - 0.5*( ((xz+e)/d2) - (1./d4) + 0.5) ) * d2;
                glm::vec3 p = glm::vec3(std::abs(std::round(x_triangleWave))-d, y, std::abs(std::round(z_triangleWave))-d);

                ckey_t key = { this->getChunkPosition(cameraPosition) * glm::vec3(1, 0, 1) + p };
                if (this->chunks.find(key) == this->chunks.end() && this->chunksToLoadSet.find(key) == this->chunksToLoadSet.end()) { /* if chunk was never generated */
                    this->chunksToLoadQueue.push(key);
                    this->chunksToLoadSet.insert(key);
                }
            }
        }
    }
}

void    Terrain::updateChunks( const glm::vec3& cameraPosition ) {
    tTimePoint lastTime = std::chrono::high_resolution_clock::now();
    this->addChunksToGenerationList(cameraPosition);

    /* update light/water of chunk and neighbours */
    while (chunksToUpdateQueue.empty() == false) {
        update_t elem = this->chunksToUpdateQueue.front();
        this->chunksToUpdateQueue.pop();
        std::array<Chunk*, 6> neighbours = this->getNeighbouringChunks(elem.chunk);
        ckey_t key = { elem.chunk };
        if (elem.action == updateType::water)
            this->chunks.at(key)->computeWater(neighbours);
        if (elem.action == updateType::light)
            this->chunks.at(key)->computeLight(neighbours, (neighbours[2] != nullptr ? neighbours[2]->getLightMask() : nullptr) );
        this->chunks.at(key)->rebuildMesh();

        for (int i = 0; i < 6; i++) {
            if (neighbours[i] != nullptr && elem.chunk + neighboursOffsets[i] != elem.from) {
                if ((this->chunks.at(key)->getSidesWaterUpdate() & (0x1 << i)) != 0)
                    this->chunksToUpdateQueue.push({ elem.chunk + neighboursOffsets[i], elem.chunk, updateType::water });
                if ((this->chunks.at(key)->getSidesLightUpdate() & (0x1 << i)) != 0)
                    this->chunksToUpdateQueue.push({ elem.chunk + neighboursOffsets[i], elem.chunk, updateType::light });
            }
        }
        double delta = (static_cast<tMilliseconds>(std::chrono::high_resolution_clock::now() - lastTime)).count();
        if (delta > 10.0)
            break;
    }

    /* generate chunks */
    while (chunksToLoadQueue.empty() == false) {
        ckey_t key = this->chunksToLoadQueue.front();
        /* delete element in load queue & set */
        this->chunksToLoadQueue.pop();
        this->chunksToLoadSet.erase(key);
        /* check if element to load is still in range */
        float distHorizontal = glm::distance(key.p * glm::vec3(1,0,1),  this->getChunkPosition(cameraPosition) * glm::vec3(1,0,1));
        if (distHorizontal > (renderDistance / chunkSize.x))
            continue;
        /* generate terrain and create chunk */
        glm::vec3 position = key.p * (glm::vec3)this->chunkSize;
        this->renderChunkGeneration(position);
        this->chunks.insert( { key, new Chunk(position, this->chunkSize, this->dataBuffer, this->dataMargin) } );
        /* issue update to light and water */
        this->chunksToUpdateQueue.push({ key.p, key.p, updateType::water });
        this->chunksToUpdateQueue.push({ key.p, key.p, updateType::light });

        double delta = (static_cast<tMilliseconds>(std::chrono::high_resolution_clock::now() - lastTime)).count();
        if (delta > this->maxAllocatedTimePerFrame)
            break;
    }
    // std::cout << (static_cast<tMilliseconds>(std::chrono::high_resolution_clock::now() - lastTime)).count() << std::endl;

    /* Debug list sizes */
    std::cout << "---\n" << "    chunks: " << chunks.size() << "\n" << "    update: " << chunksToUpdateQueue.size() << "\n" << \
    "load queue: " << chunksToLoadQueue.size() << "\n" << "  load set: " << chunksToLoadSet.size() << "\n" << std::endl;

    this->deleteOutOfRangeChunks();
}

void    Terrain::deleteOutOfRangeChunks( void ) {
    std::forward_list<ckey_t> toDelete;
    int num = 0;
    for (auto it = this->chunks.begin(); it != this->chunks.end(); ++it)
        if (it->second->isOutOfRange() == true) {
            toDelete.push_front(it->first);
            num++;
        }
    for (auto it = toDelete.begin(); it != toDelete.end(); ++it) {
        delete this->chunks.at(*it);
        this->chunks.erase(*it);
    }
    toDelete.clear();
}

void    Terrain::renderChunks( Shader shader, Camera& camera ) {
    /* copy values to array and sort them from far to front */
    chunkSort_t* sortedChunks = (chunkSort_t*)malloc(sizeof(chunkSort_t) * this->chunks.size());
    int i = 0;
    for (std::pair<ckey_t, Chunk*> chunk : this->chunks)
        sortedChunks[(i++)] = { chunk.second, glm::distance(chunk.first.p, getChunkPosition(camera.getPosition())) };
    std::sort(sortedChunks, sortedChunks+this->chunks.size(), chunkRenderingCompareSort); /* O(n*log(n)) */
    /* test, detect if we're underwater */
    glm::vec3 chunkPosition = getChunkPosition(camera.getPosition());
    glm::ivec3 positionInChunk = glm::ivec3(camera.getPosition() + glm::vec3(0,.5,0) - (chunkPosition * glm::vec3(32)) );
    int underwater = 0;
    int index = ((int)positionInChunk.x+2) + ((int)positionInChunk.z+2) * 36 + ((int)positionInChunk.y+2) * 1296;
    if (this->chunks.find({chunkPosition}) != this->chunks.end() && this->chunks[{chunkPosition}]->getTexture()[index] == 15)
        underwater = 1;
    /* render chunks in order */
    for (int i = 0; i < this->chunks.size(); ++i)
        sortedChunks[i].chunk->render(shader, camera, this->textureAtlas, renderDistance, underwater);
    free(sortedChunks);
    sortedChunks = nullptr;
}

void    Terrain::renderChunkGeneration( const glm::vec3& position ) {
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

    /* read the pixels from the framebuffer (or texture, which seems slightly faster) */
    #if (0)
        glReadPixels(0, 0, this->chunkGenerationFbo.width, this->chunkGenerationFbo.height, GL_RED, GL_UNSIGNED_BYTE, data);
    #else
        glBindTexture(GL_TEXTURE_2D, this->chunkGenerationFbo.id);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, this->dataBuffer);
        glBindTexture(GL_TEXTURE_2D, 0);
    #endif

    /* reset the framebuffer target and size */
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
}

void    Terrain::setupChunkGenerationRenderingQuad( void ) {
    /* create quad */
    std::vector<vertex_t>    vertices;
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
        vertex_t vertex;
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
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex_t), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->chunkGenerationRenderingQuad.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    // position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), static_cast<GLvoid*>(0));
    // texture coord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), reinterpret_cast<GLvoid*>(offsetof(vertex_t, TexCoords)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    vertices.clear();
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
