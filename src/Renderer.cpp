#include "Renderer.hpp"
#include "glm/ext.hpp"

Renderer::Renderer( Env* env ) :
env(env),
camera(80, (float)env->getWindow().width / (float)env->getWindow().height, 0.1f, 300.0f) {
    this->shader["default"] = new Shader("./shader/vertex/default.vert.glsl", "./shader/geometry/default.geom.glsl", "./shader/fragment/default.frag.glsl");
    // this->shader["default"] = new Shader("./shader/vertex/defaultQuad.vert.glsl", "./shader/geometry/defaultQuad.geom.glsl", "./shader/fragment/default.frag.glsl");
    this->shader["skybox"]  = new Shader("./shader/vertex/skybox.vert.glsl", "./shader/fragment/skybox.frag.glsl");
    this->shader["fxaa"]  = new Shader("./shader/vertex/screenQuad.vert.glsl", "./shader/fragment/FXAA.frag.glsl");
    this->lastTime = std::chrono::steady_clock::now();
    this->framerate = 1000.0;

    // this->initDepthMap();
    this->fxaa = false;
    if (this->fxaa)
        this->initFramebuffer();
}

Renderer::~Renderer( void ) {
    this->shader.clear();
}

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
        this->env->getController()->update();
        this->camera.handleInputs(this->env->getController()->getKeys(), this->env->getController()->getMouse());

        if (this->fxaa) {
            /* two pass rendering (for FXAA) */
            glBindFramebuffer(GL_FRAMEBUFFER, this->framebuffer.fbo);
            glClear(GL_DEPTH_BUFFER_BIT);
            this->renderLights();
            this->renderSkybox();
            this->renderMeshes();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            this->renderPostFxaa();
        }
        else {
            /* no post-processing */
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            this->renderLights();
            this->renderMeshes();
            this->renderSkybox();
        }
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
    this->shader["default"]->setVec3UniformValue("viewPos", this->camera.getPosition());
    // this->shader["default"]->setMat4UniformValue("lightSpaceMat", this->lightSpaceMat);

    this->env->getTerrain()->renderChunks(*this->shader["default"], this->camera);

    // static bool check = false;
    // if (!check) {
        this->env->getTerrain()->updateChunks(this->camera.getPosition());
        // check = true;
    // }
    
    /* copy the depth buffer to a texture (used in raymarch shader for geometry occlusion of raymarched objects) */
    // glBindTexture(GL_TEXTURE_2D, this->depthMap.id);
    // glReadBuffer(GL_FRONT);
    // glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0, this->depthMap.width, this->depthMap.height, 0);
}

void    Renderer::renderSkybox( void ) {
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    this->shader["skybox"]->use();
    this->shader["skybox"]->setMat4UniformValue("view", glm::mat4(glm::mat3(this->camera.getViewMatrix())));
    this->shader["skybox"]->setMat4UniformValue("projection", this->camera.getProjectionMatrix());
    /* render skybox */
    this->env->getSkybox()->render(*this->shader["skybox"]);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
}

void    Renderer::renderPostFxaa( void ) {
    glDisable(GL_DEPTH_TEST);
    this->shader["fxaa"]->use();
    this->shader["fxaa"]->setFloatUniformValue("near", this->camera.getNear());
    this->shader["fxaa"]->setVec2UniformValue("win_size", glm::vec2(this->env->getWindow().width, this->env->getWindow().height));
    this->env->getPostProcess()->render(*this->shader["fxaa"], this->framebuffer.id);
    glEnable(GL_DEPTH_TEST);
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
    glBindTexture(GL_TEXTURE_2D, 0);
}

void    Renderer::initFramebuffer( void ) {
    /* create FBO (FrameBuffer Object) */
    glGenFramebuffers(1, &this->framebuffer.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, this->framebuffer.fbo);
    /* create a texture */
    glGenTextures(1, &this->framebuffer.id);
    glBindTexture(GL_TEXTURE_2D, this->framebuffer.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, env->getWindow().width, env->getWindow().height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    /* bind the texture to the FBO as a color attachment */
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->framebuffer.id, 0);
    
    /* create a renderbuffer object for depth and stencil attachment (not using it) */
    unsigned int    rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, env->getWindow().width, env->getWindow().height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    /* check if FBO-kun is doing fine */
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return;
    /* unbind FBO as safety */
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}