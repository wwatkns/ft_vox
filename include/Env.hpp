#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <regex>

#include "Exception.hpp"
#include "Controller.hpp"
#include "Terrain.hpp"
#include "Cubemap.hpp"
#include "PostProcess.hpp"
#include "Light.hpp"
#include "utils.hpp"

typedef struct  s_window {
    GLFWwindow* ptr;
    int         width;
    int         height;
}               t_window;

class Env {

public:
    Env( void );
    ~Env( void );

    const t_window&                     getWindow( void ) const { return (window); };
    Controller*                         getController( void ) { return (controller); };
    Terrain*                            getTerrain( void ) { return (terrain); };
    std::vector<Light*>&                getLights( void ) { return (lights); };
    Cubemap*                            getSkybox( void ) { return (skybox); };
    PostProcess*                        getPostProcess( void ) { return (postProcess); };
    Light*                              getDirectionalLight( void );

private:
    t_window                        window;
    Controller*                     controller;
    Terrain*                        terrain;
    std::vector<Light*>             lights;
    Cubemap*                        skybox;
    PostProcess*                    postProcess;

    void        initGlfwEnvironment( const std::string& glVersion = "4.0" );
    void        initGlfwWindow( size_t width, size_t height );
    void        setupController( void );
    // callback to be called each time the window is resized to update the viewport size as well
    static void framebufferSizeCallback( GLFWwindow* window, int width, int height );

};
