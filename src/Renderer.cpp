#include "Renderer.hpp"
#include "glm/ext.hpp"

Renderer::Renderer( Env* env ) :
env(env),
camera(80, (float)env->getWindow().width / (float)env->getWindow().height, 0.1f, 300.0f) {
    this->shader["default"] = new Shader("./shader/vertex/default.vert.glsl", "./shader/geometry/default.geom.glsl", "./shader/fragment/default.frag.glsl");
    this->shader["skybox"]  = new Shader("./shader/vertex/skybox.vert.glsl", "./shader/fragment/skybox.frag.glsl");
    this->shader["shadowMap"] = new Shader("./shader/vertex/shadowMap.vert.glsl", "./shader/fragment/shadowMap.frag.glsl");
    this->lastTime = std::chrono::steady_clock::now();
    this->framerate = 60.0;

    // this->initDepthMap();
    // this->initShadowDepthMap(4096, 4096);
    // this->initRenderbuffer();
    
    this->useShadows = 0;
}

Renderer::~Renderer( void ) {
}

/*
    We generate chunks as needed with the shader program "generateChunk" :
    We give it the position of the chunk to generate in world grid (so that we have consistency at chunk borders)
    at it uses a fragment shader and renders to a FBO a texture of size 256*256 (16*16*256).
    We can store this texture in a std::vector<char*> that will contain all the loaded chunks, then we only have
    to decode the texture to fill our VBO that will be used for rendering our meshes.

    Now our rendering pipeline consists of VBOs that will be used in vertex shader, then we have a geometry shader
    to create the faces of our cubes (because we only put the faces positions in the VBO, not the vertices), then
    we have a fragment shader for the end result.
*/

/*
    Rendering voxels : (geometry shader quads)
    we render Faces with GL_POINTS (each point contain information about the face position, blockId and faceId)

    So we have a terrain that is composed of chunks, that are composed of blocks, that are composed of faces.
    those faces contain only information about their position (not vertices, geometry shader will handle that),
    their id and normals.
*/

void	Renderer::loop( void ) {
    static int frames = 0;
    static double last = 0.0;
    glEnable(GL_DEPTH_TEST); /* z-buffering */
    glEnable(GL_FRAMEBUFFER_SRGB); /* gamma correction */
    glEnable(GL_BLEND); /* transparency */
    glEnable(GL_CULL_FACE); /* face culling (back faces are not rendered) */
    glCullFace(GL_BACK);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    while (!glfwWindowShouldClose(this->env->getWindow().ptr)) {
        glfwPollEvents();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        this->env->getController()->update();
        this->camera.handleInputs(this->env->getController()->getKeys(), this->env->getController()->getMouse());
        this->useShadows = this->env->getController()->getKeyValue(GLFW_KEY_P);

        /* rendering passes */
        // this->updateShadowDepthMap();
        this->renderLights();
        this->renderMeshes();
        // this->renderSkybox();

        glfwSwapBuffers(this->env->getWindow().ptr);
        /* display framerate */
        tTimePoint current = std::chrono::steady_clock::now();
        frames++;
        if ((static_cast<tMilliseconds>(current - this->lastTime)).count() > 999) {
            std::cout << frames << " fps" << std::endl;
            this->lastTime = current;
            frames = 0;
        }
        /* cap framerate */
        double delta = std::abs(glfwGetTime()/1000.0 - last);
        if (delta < (1000. / this->framerate))
            std::this_thread::sleep_for(std::chrono::milliseconds((uint64_t)(1000. / this->framerate - delta)));
        last = glfwGetTime()/1000.0;
    }
}

// void    Renderer::updateShadowDepthMap( void ) {
//     Light*  directionalLight = this->env->getDirectionalLight();
//     if (this->useShadows && directionalLight) {
//         glm::mat4 lightProjection, lightView;
//         lightProjection = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, this->camera.getNear(), this->camera.getFar());
//         lightView = glm::lookAt(directionalLight->getPosition(), glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
//         this->lightSpaceMat = lightProjection * lightView;
//         /* render scene from light's point of view */
//         this->shader["shadowMap"]->use();
//         this->shader["shadowMap"]->setMat4UniformValue("lightSpaceMat", this->lightSpaceMat);

//         glViewport(0, 0, this->shadowDepthMap.width, this->shadowDepthMap.height);
//         glBindFramebuffer(GL_FRAMEBUFFER, this->shadowDepthMap.fbo);
//         glClear(GL_DEPTH_BUFFER_BIT);

//         if (this->env->getModels().size() != 0)
//             /* render meshes on shadowMap shader */
//             for (auto it = this->env->getModels().begin(); it != this->env->getModels().end(); it++)
//                 (*it)->render(*this->shader["shadowMap"]);

//         /* reset viewport and framebuffer*/
//         glBindFramebuffer(GL_FRAMEBUFFER, 0);
//         glViewport(0, 0, this->env->getWindow().width, this->env->getWindow().height);
//         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//     }
// }

void    Renderer::renderLights( void ) {
    /* update shader uniforms */
    this->shader["default"]->use();
    // this->shader["default"]->setIntUniformValue("nPointLights", Light::pointLightCount);

    /* render lights for meshes */
    for (auto it = this->env->getLights().begin(); it != this->env->getLights().end(); it++)
        (*it)->render(*this->shader["default"]);
}

void    Renderer::renderMeshes( void ) {
    /* update shader uniforms */
    this->shader["default"]->use();
    this->shader["default"]->setVec3UniformValue("cameraPos", this->camera.getPosition());
    this->shader["default"]->setVec3UniformValue("cameraPos2", this->camera.getPosition());
    // this->shader["default"]->setMat4UniformValue("lightSpaceMat", this->lightSpaceMat);
    // glActiveTexture(GL_TEXTURE0);
    // this->shader["default"]->setIntUniformValue("shadowMap", 0);
    // this->shader["default"]->setIntUniformValue("state.use_shadows", this->useShadows);
    // glBindTexture(GL_TEXTURE_2D, this->shadowDepthMap.id);

    this->env->getTerrain()->render(*this->shader["default"], this->camera);
    // this->env->getTerrain()->update();
    
    /* copy the depth buffer to a texture (used in raymarch shader for geometry occlusion of raymarched objects) */
    // glBindTexture(GL_TEXTURE_2D, this->depthMap.id);
    // glReadBuffer(GL_FRONT);
    // glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0, this->depthMap.width, this->depthMap.height, 0);
}

// void    Renderer::renderSkybox( void ) {
//     glDepthFunc(GL_LEQUAL);
//     this->shader["skybox"]->use();
//     this->shader["skybox"]->setMat4UniformValue("view", glm::mat4(glm::mat3(this->camera.getViewMatrix())));
//     this->shader["skybox"]->setMat4UniformValue("projection", this->camera.getProjectionMatrix());
//     /* render skybox */
//     this->env->getSkybox()->render(*this->shader["skybox"]);
//     glDepthFunc(GL_LESS);
// }

void    Renderer::initShadowDepthMap( const size_t width, const size_t height ) {
    this->shadowDepthMap.width = width;
    this->shadowDepthMap.height = height;

    glGenFramebuffers(1, &this->shadowDepthMap.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, this->shadowDepthMap.fbo);
    /* create depth texture */
    glGenTextures(1, &this->shadowDepthMap.id);
    glBindTexture(GL_TEXTURE_2D, this->shadowDepthMap.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // attach depth texture as FBO's depth buffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->shadowDepthMap.id, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    this->shader["default"]->use();
    this->shader["default"]->setIntUniformValue("shadowMap", 0);
    this->shader["default"]->setIntUniformValue("texture_diffuse1", 1);
    this->shader["default"]->setIntUniformValue("texture_normal1", 2);
    this->shader["default"]->setIntUniformValue("texture_specular1", 3);
    this->shader["default"]->setIntUniformValue("texture_emissive1", 4);
}

void    Renderer::initDepthMap( void ) {
    this->depthMap.width = this->env->getWindow().width;
    this->depthMap.height = this->env->getWindow().height;

    glGenFramebuffers(1, &this->depthMap.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, this->depthMap.fbo);
    /* create depth texture */
    glGenTextures(1, &this->depthMap.id);
    glBindTexture(GL_TEXTURE_2D, this->depthMap.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, this->depthMap.width, this->depthMap.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // GL_NEAREST
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // attach depth texture as FBO's depth buffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->depthMap.id, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    this->shader["raymarch"]->use();
    this->shader["raymarch"]->setIntUniformValue("depthBuffer", 0);
}

void    Renderer::initRenderbuffer( void ) { 
    this->renderbuffer.width = this->env->getWindow().width;
    this->renderbuffer.height = this->env->getWindow().height;

    /* create RenderBuffer */
    glGenRenderbuffers(1, &this->renderbuffer.id);
    glBindRenderbuffer(GL_RENDERBUFFER, this->renderbuffer.id);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, this->renderbuffer.width, this->renderbuffer.height);

    /* create FrameBuffer, with renderbuffer binded */
    glGenFramebuffers(1, &this->renderbuffer.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, this->renderbuffer.fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, this->renderbuffer.id);
    /* attach depth-buffer component that is also associated to another FBO (one depth-buffer, multiple Fbos) */
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->depthMap.id, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}
