#pragma once

#include <glm/glm.hpp>

class Camera {
public:
    Camera() = delete;
    Camera(glm::mat4* view);
    void rotate(float dAzimuth, float dAltitude);
    void pan(float dX, float dY);
    void zoom(float factor);
    const glm::mat4& view();

private:
    void recalculate();

    float _azimuth;
    float _altitude;
    float _radius;
    glm::vec3 _center;
    glm::vec3 _eyeDir;
    bool _dirty;
    glm::mat4& _view;
};