#include "Camera.hpp"
#include "glm/ext.hpp"

Camera::Camera( float fov, float aspect, float near, float far ) : aspect(aspect), fov(fov), near(near), far(far) {
    this->projectionMatrix = glm::perspective(glm::radians(fov), aspect, near, far);
    this->invProjectionMatrix = glm::inverse(this->projectionMatrix);
    this->position = glm::vec3(0.0f, 0.0f, 3.0f);
    this->cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    this->viewMatrix = glm::lookAt(this->position, this->position + this->cameraFront, glm::vec3(0.0f, 1.0f, 0.0f));
    this->viewProjectionMatrix = this->projectionMatrix * this->viewMatrix;
    this->invViewMatrix = glm::inverse(this->viewMatrix);
    this->pitch = 0;
    this->yaw = 0;
    this->last = std::chrono::steady_clock::now();
    this->speed = 0.02;
    this->speedmod = 1.0;

    this->updateFustrumPlanes();
    this->updateFustrum = false;
}

Camera::Camera( const Camera& rhs ) {
    *this = rhs;
}

Camera& Camera::operator=( const Camera& rhs ) {
    this->projectionMatrix = rhs.getProjectionMatrix();
    this->viewMatrix = rhs.getViewMatrix();
    this->position = rhs.getPosition();
    this->cameraFront = rhs.getCameraFront();
    this->fov = rhs.getFov();
    this->aspect = rhs.getAspect();
    this->near = rhs.getNear();
    this->far = rhs.getFar();
    return (*this);
}

Camera::~Camera( void ) {
}

void    Camera::setFov( float fov ) {
    this->fov = fov;
    this->projectionMatrix = glm::perspective(glm::radians(this->fov), this->aspect, this->near, this->far);
    this->invProjectionMatrix = glm::inverse(this->projectionMatrix);
}

void    Camera::setAspect( float aspect ) {
    this->aspect = aspect;
    this->projectionMatrix = glm::perspective(glm::radians(this->fov), this->aspect, this->near, this->far);
    this->invProjectionMatrix = glm::inverse(this->projectionMatrix);
}

void    Camera::setNear( float near ) {
    this->near = near;
    this->projectionMatrix = glm::perspective(glm::radians(this->fov), this->aspect, this->near, this->far);
    this->invProjectionMatrix = glm::inverse(this->projectionMatrix);
}

void    Camera::setFar( float far ) {
    this->far = far;
    this->projectionMatrix = glm::perspective(glm::radians(this->fov), this->aspect, this->near, this->far);
    this->invProjectionMatrix = glm::inverse(this->projectionMatrix);
}

void    Camera::handleInputs( const std::array<tKey, N_KEY>& keys, const tMouse& mouse ) {
    this->handleKeys(keys);
    this->handleMouse(mouse);
    this->viewMatrix = glm::lookAt(this->position, this->position + this->cameraFront, glm::vec3(0, 1, 0));
    this->viewProjectionMatrix = this->projectionMatrix * this->viewMatrix;
    this->invViewMatrix = glm::inverse(this->viewMatrix);
    this->last = std::chrono::steady_clock::now();

    // if (this->updateFustrum)
        this->updateFustrumPlanes();
    this->updateFustrum = false;
}

void    Camera::handleKeys( const std::array<tKey, N_KEY>& keys ) {
    glm::vec4    translate(
        (float)(keys[GLFW_KEY_A].value - keys[GLFW_KEY_D].value),
        (float)(keys[GLFW_KEY_LEFT_SHIFT].value - keys[GLFW_KEY_SPACE].value),
        (float)(keys[GLFW_KEY_W].value - keys[GLFW_KEY_S].value),
        1.0f
    );
    /* translation is in the same coordinate system as view (moves in same direction) */
    translate = glm::transpose(this->viewMatrix) * glm::normalize(translate);
    this->position = this->position - glm::vec3(translate) * this->getElapsedMilliseconds(this->last).count() * this->speed * this->speedmod;
    this->updateFustrum = (keys[GLFW_KEY_F].value ? true : false);
}

void    Camera::handleMouse( const tMouse& mouse, float sensitivity ) {
    this->pitch += (mouse.prevPos.y - mouse.pos.y) * sensitivity;
    this->pitch = std::min(std::max(this->pitch, -89.0f), 89.0f);
    this->yaw += (mouse.pos.x - mouse.prevPos.x) * sensitivity;
    glm::vec3 front(
        std::cos(glm::radians(pitch)) * std::cos(glm::radians(yaw)),
        std::sin(glm::radians(pitch)),
        std::cos(glm::radians(pitch)) * std::sin(glm::radians(yaw))
    );
    this->cameraFront = glm::normalize(front);
}

tMilliseconds   Camera::getElapsedMilliseconds( tTimePoint last ) {
    return (std::chrono::steady_clock::now() - last);
}

void    Camera::updateFustrumPlanes( void ) {
    glm::mat3 transform = this->viewProjectionMatrix;
    glm::vec3 cameraUp = glm::normalize(glm::row(transform, 1));
    glm::vec3 cameraRight = -glm::normalize(glm::row(transform, 0));

    float tang = std::tan(glm::radians(this->fov / 2.0f));
    float near_h = tang * near;
	float near_w = near_h * this->aspect;
    // float far_h = tang * far;
	// float far_w = far_h * this->aspect;

    glm::vec3 p, n;
	glm::vec3 nc = this->position + this->cameraFront * near;
    glm::vec3 fc = this->position + this->cameraFront * far;
    /* near */
    this->planes[0] = (tPlane){ -this->cameraFront, glm::dot(nc, -this->cameraFront) };
    /* far */
    this->planes[1] = (tPlane){ this->cameraFront, glm::dot(fc, this->cameraFront) };
    /* up */
    p = nc + cameraUp * near_h;
    n = glm::cross(glm::normalize(p - this->position), cameraRight);
    this->planes[2] = (tPlane){ n, glm::dot(p, n) };
    /* down */
    p = nc - cameraUp * near_h;
    n = glm::cross(cameraRight, glm::normalize(p - this->position));
    this->planes[3] = (tPlane){ n, glm::dot(p, n) };
    /* left */
    p = nc - cameraRight * near_w;
    n = glm::cross(glm::normalize(p - this->position), cameraUp);
    this->planes[4] = (tPlane){ n, glm::dot(p, n) };
    /* right */
    p = nc + cameraRight * near_w;
    n = glm::cross(cameraUp, glm::normalize(p - this->position));
    this->planes[5] = (tPlane){ n, glm::dot(p, n) };
}

bool    Camera::pointInFustrum( const glm::vec3& p ) {
	for (int i = 0; i < 6; ++i)
		if (distancePointToPlane(p, this->planes[i]) < 0)
			return false;
	return true;
}

bool    Camera::sphereInFustrum( const glm::vec3& p, float radius ) {
	float distance;
	for (int i = 0; i < 6; ++i) {
		distance = distancePointToPlane(p, this->planes[i]);
		if (distance < -radius)
			return false;
		else if (distance < radius)
			continue; /* intersection with fustrum */
	}
	return true;
}

/* source : http://www.lighthouse3d.com/tutorials/view-frustum-culling/geometric-approach-testing-boxes-ii/ */
static glm::vec3    getVertexMin( const glm::vec3& p, const glm::vec3& size, const glm::vec3& normal ) {
    return glm::vec3(
        (normal.x >= 0 ? p.x - size.x : p.x + size.x),
        (normal.y >= 0 ? p.y - size.y : p.y + size.y),
        (normal.z >= 0 ? p.z - size.z : p.z + size.z)
    );
}

static glm::vec3    getVertexMax( const glm::vec3& p, const glm::vec3& size, const glm::vec3& normal ) {
    return glm::vec3(
        (normal.x >= 0 ? p.x + size.x : p.x - size.x),
        (normal.y >= 0 ? p.y + size.y : p.y - size.y),
        (normal.z >= 0 ? p.z + size.z : p.z - size.z)
    );
}

bool    Camera::aabInFustrum( const glm::vec3& p, const glm::vec3& size ) {
	for (int i = 0; i < 6; ++i) {
		if (distancePointToPlane(getVertexMax(p, size / 2, this->planes[i].normal), this->planes[i]) < 0)
			return false;
		else if (distancePointToPlane(getVertexMin(p, size / 2, this->planes[i].normal), this->planes[i]) < 0)
            continue; /* intersect */
	}
	return true;
}

float   distancePointToPlane( const glm::vec3& point, const tPlane& plane ) {
    return glm::dot(plane.normal, point) + plane.d;
}