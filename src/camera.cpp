#include "camera.h"

#include <glm/gtx/transform.hpp>

const glm::vec3 WORLD_UP = { 0.f, 0.f, 1.f };

glm::mat2 rotate(float angle) {
  float sine = sin(angle);
  float cosine = cos(angle);
  return glm::mat2(cosine, -sine, sine, cosine);
}

void Camera::move(glm::vec3 unitsVec) {
  glm::vec3 delta = unitsVec.x * right + unitsVec.y * forward + unitsVec.z * up;
  pos += delta;
}

// yawDelta is degrees
void Camera::turn(f32 yawDelta) {
  float sineYaw = -sin(yawDelta);
  float cosineYaw = cos(yawDelta);
  glm::vec3 newForward;
  newForward.z = forward.z;
  newForward.x = (forward.x * cosineYaw) + (forward.y * -sineYaw);
  newForward.y = (forward.x * sineYaw) + (forward.y * cosineYaw);
  setForward(newForward);
}

void Camera::setForward(glm::vec3 forward) {
  this->forward = glm::normalize(forward);
  this->right = glm::normalize(glm::cross(this->forward, WORLD_UP));
  this->up = glm::cross(right, forward);
}

glm::mat4 Camera::getViewMatrix() {
  return glm::lookAt(pos, forward + pos, glm::vec3(0.0f, 0.0f, 1.0f));
}