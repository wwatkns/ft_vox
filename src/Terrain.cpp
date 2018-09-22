#include "Terrain.hpp"
#include "glm/ext.hpp"

Terrain::Terrain( uint renderDistance, uint maxHeight, const glm::ivec3& chunkSize ) : renderDistance(renderDistance), maxHeight(maxHeight), chunkSize(chunkSize) {
    this->setupChunkGenerationRenderingQuad();
    this->setupChunkGenerationFbo();
    this->chunkGenerationShader = new Shader("./shader/vertex/screenQuad.vert.glsl", "./shader/fragment/generateChunk.frag.glsl");
    this->noiseSampler = loadTexture("./resource/RGBAnoiseMedium.png");
    this->textureAtlas = loadTexture("./resource/terrain.png", GL_NEAREST);
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

glm::vec3   Terrain::getChunkPosition( const glm::vec3& position ) {
    glm::vec3 chunkPosition;
    // ceil the y position to max height
    chunkPosition.x = std::floor(position.x / (float)this->chunkSize.x);
    chunkPosition.y = std::max(std::min(std::floor(position.y / (float)this->chunkSize.y), this->maxHeight / (float)this->chunkSize.y - 1.0f), 0.0f);
    chunkPosition.z = std::floor(position.z / (float)this->chunkSize.z);
    return chunkPosition;
}

void    Terrain::generateChunkTextures( const Camera& camera ) {
    /*  naive algorithm : 
        * iterate on all the positions around player in render distance range and create chunk if it does not exist.
    */
    int height = this->maxHeight / this->chunkSize.y;
    glm::ivec3 dist = glm::ivec3(this->renderDistance) / this->chunkSize;
    for (int y = 0; y < height; ++y) /* we build a column of chunks each time */
        for (int z = -dist.z; z < dist.z; ++z)
            for (int x = -dist.x; x < dist.x; ++x) {
                /* if chunk was never generated */
                Key key = { this->getChunkPosition(camera.getPosition()) * glm::vec3(1, 0, 1) + glm::vec3(x, y, z) };
                if (this->chunks.find(key) == this->chunks.end()) {
                    /* if we maximum number of chunks stored is reached, delete one to make room */
                    // if (this->chunks.size() >= (renderDistance / this->chunkSize.x) * (renderDistance / this->chunkSize.z) * height) {
                    //     // std::cout << "deleting chunk" << std::endl;
                    //     // this->chunks.erase( {  } );
                    // }
                    glm::vec3 position = key.p * (glm::vec3)this->chunkSize;
                    this->renderChunkGeneration(position, this->dataBuffer);
                    this->chunks.insert( { key, new Chunk(position, this->chunkSize, this->dataBuffer) } );
                    // std::cout << "> BUILDING: (" << key.p.x << ", " << key.p.y << ", "<< key.p.z << ")" << std::endl;
                }
            }
}

void    Terrain::generateChunkMeshes( void ) {
    for (std::pair<Key, Chunk*> chunk : this->chunks) {
        if (!chunk.second->isMeshed()) {
            const std::array<const uint8_t*, 6> adjacentChunks = {
                ( chunks.find({chunk.first.p - glm::vec3(1, 0, 0)}) != chunks.end() ? chunks[{chunk.first.p - glm::vec3(1, 0, 0)}]->getTexture() : nullptr), // left
                ( chunks.find({chunk.first.p + glm::vec3(1, 0, 0)}) != chunks.end() ? chunks[{chunk.first.p + glm::vec3(1, 0, 0)}]->getTexture() : nullptr), // right
                ( chunks.find({chunk.first.p - glm::vec3(0, 0, 1)}) != chunks.end() ? chunks[{chunk.first.p - glm::vec3(0, 0, 1)}]->getTexture() : nullptr), // back
                ( chunks.find({chunk.first.p + glm::vec3(0, 0, 1)}) != chunks.end() ? chunks[{chunk.first.p + glm::vec3(0, 0, 1)}]->getTexture() : nullptr), // front
                ( chunks.find({chunk.first.p - glm::vec3(0, 1, 0)}) != chunks.end() ? chunks[{chunk.first.p - glm::vec3(0, 1, 0)}]->getTexture() : nullptr), // down
                ( chunks.find({chunk.first.p + glm::vec3(0, 1, 0)}) != chunks.end() ? chunks[{chunk.first.p + glm::vec3(0, 1, 0)}]->getTexture() : nullptr)  // up
            };
            chunk.second->buildMesh(adjacentChunks);
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
    this->chunkGenerationShader->setFloatUniformValue("uTime", glfwGetTime());
    this->chunkGenerationShader->setVec3UniformValue("chunkPosition", position);
    this->chunkGenerationShader->setVec3UniformValue("chunkSize", glm::vec3(this->chunkSize) );
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

void    Terrain::updateChunks( const Camera& camera ) {
    this->generateChunkTextures(camera);
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
    this->chunkGenerationFbo.width = this->chunkSize.x;
    this->chunkGenerationFbo.height = this->chunkSize.y * this->chunkSize.z;

    glGenFramebuffers(1, &this->chunkGenerationFbo.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, this->chunkGenerationFbo.fbo);
    /* create color texture */
    glGenTextures(1, &this->chunkGenerationFbo.id);
    glBindTexture(GL_TEXTURE_2D, this->chunkGenerationFbo.id);
    // glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, this->chunkGenerationFbo.width, this->chunkGenerationFbo.height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->chunkGenerationFbo.id, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
