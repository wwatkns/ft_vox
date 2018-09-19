#include "Env.hpp"

Env::Env( void ) {
    try {
        this->initGlfwEnvironment("4.0");
        // this->initGlfwWindow(720, 480); /* 1280x720 */
        this->initGlfwWindow(960, 540); /* 1920x1080 */
        // this->initGlfwWindow(1280, 720); /* 2560x1440 */
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
            throw Exception::InitError("glad initialization failed");
        this->controller = new Controller(this->window.ptr);

        // this->terrain = new Terrain(6, 48, glm::ivec3(16, 16, 16));
        // this->terrain = new Terrain(6, 48, glm::ivec3(4, 4, 4));
        // this->terrain = new Terrain(20, 64, glm::ivec3(16, 16, 16));
        this->terrain = new Terrain(12, 128, glm::ivec3(32, 32, 32));
        // this->terrain = new Terrain(5, 64, glm::ivec3(64, 64, 64));
        // this->terrain = new Terrain(5, 196, glm::ivec3(64, 64, 64));
        // this->terrain = new Terrain(10, 32, glm::ivec3(4, 16, 4));

        // this->skyboxTexture = loadCubemap(std::vector<std::string>{{
        //     "./resource/CloudyLightRays/CloudyLightRaysLeft2048.png",
        //     "./resource/CloudyLightRays/CloudyLightRaysRight2048.png",
        //     "./resource/CloudyLightRays/CloudyLightRaysUp2048.png",
        //     "./resource/CloudyLightRays/CloudyLightRaysDown2048.png",
        //     "./resource/CloudyLightRays/CloudyLightRaysFront2048.png",
        //     "./resource/CloudyLightRays/CloudyLightRaysBack2048.png",
        // }});
        // this->noiseTexture = loadTexture("./resource/RGBAnoiseMedium.png");
        this->lights = {
            new Light(
                glm::vec3(30, 30, 18),
                glm::vec3(0.77f, 0.88f, 1.0f) * 0.075f,
                glm::vec3(1.0f, 0.964f, 0.77f),
                glm::vec3(1.0f, 1.0f, 1.0f),
                eLightType::directional
            )
        };
        // this->skybox = new Model(std::vector<std::string>{{
        //     "./resource/CloudyLightRays/CloudyLightRaysLeft2048.png",
        //     "./resource/CloudyLightRays/CloudyLightRaysRight2048.png",
        //     "./resource/CloudyLightRays/CloudyLightRaysUp2048.png",
        //     "./resource/CloudyLightRays/CloudyLightRaysDown2048.png",
        //     "./resource/CloudyLightRays/CloudyLightRaysFront2048.png",
        //     "./resource/CloudyLightRays/CloudyLightRaysBack2048.png",
        // }});

        this->setupController();
    } catch (const std::exception& err) {
        std::cout << err.what() << std::endl;
    }
}

Env::~Env( void ) {
    delete this->terrain;
    for (size_t i = 0; i < this->lights.size(); ++i)
        delete this->lights[i];
    // delete this->skybox;
    delete this->controller;
    if (glIsTexture(this->skyboxTexture))
        glDeleteTextures(1, &this->skyboxTexture);
    if (glIsTexture(this->noiseTexture))
        glDeleteTextures(1, &this->noiseTexture);
    glfwDestroyWindow(this->window.ptr);
    glfwTerminate();
}

void	Env::initGlfwEnvironment( const std::string& glVersion ) {
	if (!glfwInit())
		throw Exception::InitError("glfw initialization failed");
    if (!std::regex_match(glVersion, static_cast<std::regex>("^[0-9]{1}.[0-9]{1}$")))
        throw Exception::InitError("invalid openGL version specified");
    size_t  p = glVersion.find('.');
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, std::stoi(glVersion.substr(0,p)));
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, std::stoi(glVersion.substr(p+1)));
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
}

void	Env::initGlfwWindow( size_t width, size_t height ) {
	if (!(this->window.ptr = glfwCreateWindow(width, height, "ft_vox", NULL, NULL)))
        throw Exception::InitError("glfwCreateWindow failed");
	glfwMakeContextCurrent(this->window.ptr);
	glfwSetFramebufferSizeCallback(this->window.ptr, this->framebufferSizeCallback);
	glfwSetInputMode(this->window.ptr, GLFW_STICKY_KEYS, 1);
    glfwGetFramebufferSize(this->window.ptr, &this->window.width, &this->window.height);
}

void    Env::setupController( void ) {
    this->controller->setKeyProperties(GLFW_KEY_P, eKeyMode::toggle, 1, 1000);
    this->controller->setKeyProperties(GLFW_KEY_F, eKeyMode::instant, 1, 1000);
}

void    Env::framebufferSizeCallback( GLFWwindow* window, int width, int height ) {
    glViewport(0, 0, width, height);
}

Light*  Env::getDirectionalLight( void ) {
    for (auto it = this->lights.begin(); it != this->lights.end(); it++)
        if ((*it)->getType() == eLightType::directional)
            return (*it);
    return (nullptr);
}
