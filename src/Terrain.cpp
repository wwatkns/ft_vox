#include "Terrain.hpp"
#include "glm/ext.hpp"

Terrain::Terrain( uint8_t renderDistance, uint8_t maxHeight, const glm::vec3& chunkSize ) : renderDistance(renderDistance), maxHeight(maxHeight), chunkSize(chunkSize) {
    this->setupChunkGenerationRenderingQuad();
    this->setupChunkGenerationFbo();
    this->chunkGenerationShader = new Shader("./shader/vertex/screenQuad.vert.glsl", "./shader/fragment/generateChunk.frag.glsl");
    this->noiseSampler = loadTexture("./resource/RGBAnoiseMedium.png");

    this->dataBuffer = static_cast<uint8_t*>(malloc(sizeof(uint8_t) * this->chunkGenerationFbo.width * this->chunkGenerationFbo.height));

    this->update();

    /* Generate all the chunks around player */
    // this->generateChunk(glm::vec3(0, 0, 0));
    // for (uint8_t z = 0; z < renderDistance; ++z)
    //     for (uint8_t x = 0; x < renderDistance; ++x) {
    //         this->generateChunk(glm::vec3(x * this->chunkSize.x, 0, z * this->chunkSize.z)); // TODO: handle y axis also
    //         // std::cout << "generating chunk : [" << static_cast<int>(x) << ", " << static_cast<int>(z) << "]\n";
    //     }
}

Terrain::~Terrain( void ) {
}

void    Terrain::generateChunk( const glm::vec3& position ) {
    this->renderChunkGeneration(position, this->dataBuffer);

    std::vector<tPoint> voxels;
    voxels.reserve(this->chunkSize.x * this->chunkSize.y * this->chunkSize.z);
    for (int i = 0; i < this->chunkSize.x * this->chunkSize.y * this->chunkSize.z; ++i) {
        if (this->dataBuffer[i] != 0) { /* 0 is empty block */
            int x = i % (int)this->chunkSize.x;
            int y = i / (int)(this->chunkSize.x * this->chunkSize.z);
            int z = i / (int)(this->chunkSize.x) % (int)(this->chunkSize.z);
            voxels.push_back( { glm::vec3(x, y, z), (uint8_t)((float)this->dataBuffer[i]/255.0f) } );
        }
    }
    this->chunks.push_back( new Chunk(voxels, position) );
}

/* convert 2d height-map to surface */
// void    Terrain::generateChunk( const glm::vec3& position ) {
//     this->renderChunkGeneration(position, this->dataBuffer);

//     float* data_b = static_cast<float*>(malloc(sizeof(float) * this->chunkSize.x * this->chunkSize.y * this->chunkSize.z));

//     for (int i = 0; i < this->chunkSize.x * this->chunkSize.y * this->chunkSize.z; ++i)
//         data_b[i] = 0;
//     for (int y = 0; y < this->chunkSize.z; ++y)
//         for (int x = 0; x < this->chunkSize.x; ++x) {
//             int index = x + y * this->chunkSize.x;
//             int h = index + this->chunkSize.x * this->chunkSize.z * std::min(std::round(this->dataBuffer[index] * this->chunkSize.y), this->chunkSize.y - 1);
//             for (int f = h; f >= 0; f -= this->chunkSize.x * this->chunkSize.z) { data_b[f] = 1; } /* fill point and below */
//         }
//     /* generate voxels */
//     std::vector<tPoint> voxels;
//     for (int y = 0; y < this->chunkSize.y; ++y)
//         for (int z = 0; z < this->chunkSize.z; ++z)
//             for (int x = 0; x < this->chunkSize.x; ++x) {
//                 int index = x + z * this->chunkSize.x + y * this->chunkSize.x * this->chunkSize.z;
//                 if (data_b[index] != 0) // 0 is empty block
//                     voxels.push_back( { glm::vec3(x, y, z), (uint8_t)data_b[index] } );
//             }
//     this->chunks.push_back( new Chunk(voxels, position) );
//     free(data_b);
//     data_b = nullptr;
// }

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
    this->chunkGenerationShader->setVec3UniformValue("chunkSize", this->chunkSize);
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

void    Terrain::render( Shader shader ) {
    for (unsigned int i = 0; i < this->chunks.size(); ++i)
        this->chunks[i]->render(shader);
}

void    Terrain::update( void ) {
    this->chunks.clear();
    for (uint8_t z = 0; z < this->renderDistance; ++z)
        for (uint8_t x = 0; x < this->renderDistance; ++x) {
            this->generateChunk(glm::vec3(x * this->chunkSize.x, 0, z * this->chunkSize.z)); // TODO: handle y axis also
            // std::cout << "generating chunk : [" << static_cast<int>(x) << ", " << static_cast<int>(z) << "]\n";
        }
    for (uint8_t z = 0; z < this->renderDistance; ++z)
        for (uint8_t y = 0; y < this->maxHeight / this->chunkSize.y; ++y)
            for (uint8_t x = 0; x < this->renderDistance; ++x) {
                this->generateChunk(glm::vec3(x * this->chunkSize.x, y * this->chunkSize.y, z * this->chunkSize.z));
                std::cout << "generating chunk : [" << (int)x << ", " << (int)y << ", " << (int)z << "]\n";
            }
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
    // this->chunkGenerationFbo.height = this->chunkSize.z;

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
