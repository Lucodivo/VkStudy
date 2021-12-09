#include "camera.h"

const glm::vec3 WORLD_UP = { 0.f, 0.f, 1.f };

void Camera::move(glm::vec3 unitsVec) {
  glm::vec3 delta = unitsVec.x * right + unitsVec.y * forward + unitsVec.z * up;
  pos += delta;
}

void Camera::setForward(glm::vec3 forward) {

}