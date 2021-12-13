#pragma once

#include <glm/glm.hpp>
#include "types.h"

class Camera {
public:
  glm::vec3 pos;
  glm::vec3 forward, up, right;
  glm::mat4 projection;

  void move(glm::vec3 unitsVec); // -1/1, x = left/right, y = back/forward, z = down/up
  
  void turn(f32 yawDelta);

  glm::mat4 getViewMatrix();

  void setForward(glm::vec3 forward);

private:

};