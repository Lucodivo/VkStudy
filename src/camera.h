#pragma once

class Camera {
public:
  vec3 pos;
  vec3 forward, up, right;
  mat4 projection;
  f32 pitch = 0.0f;

  void move(vec3 unitsVec); // -1/1, x = left/right, y = back/forward, z = down/up

  void turn(f32 yawDelta, f32 pitchDelta);

  mat4 getViewMatrix();

  void setForward(vec3 forward);
  void lookAt(vec3 focusPoint);

private:
  void configVectorsFromForward();
};