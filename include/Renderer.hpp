#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <thread>

#include "Exception.hpp"
#include "Env.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "Terrain.hpp"

typedef std::unordered_map<std::string, Shader*> shadermap_t;
typedef std::chrono::duration<double,std::milli> milliseconds_t;
typedef std::chrono::steady_clock::time_point timepoint_t;

class Renderer {

public:
    Renderer( Env* env );
    ~Renderer( void );

    void	loop( void );
    void    renderLights( void );
    void    renderMeshes( void );
    void    renderSkybox( void );
    void    renderPostFxaa( void );

private:
    Env*            env;
    Camera          camera;
    shadermap_t     shader;
    framebuffer_t   framebuffer;
    glm::mat4       lightSpaceMat;
    float           framerate;
    bool            fxaa;

    timepoint_t     lastTime;

    void    initFramebuffer( void );

};
