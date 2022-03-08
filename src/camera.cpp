const vec3 WORLD_UP = {0.f, 0.f, 1.f};

const f32 maxPitch = 70.0f * RadiansPerDegree;
const f32 maxPitch_invSine = asin(maxPitch);

void Camera::move(vec3 unitsVec) {
  vec3 delta = unitsVec.x * right + unitsVec.y * forward + unitsVec.z * up;
  pos += delta;
}

void Camera::configVectorsFromForward() {
  // NOTE: forward assumed to be set
  this->right = normalize(cross(this->forward, WORLD_UP));
  this->up = cross(right, forward);
};

// yawDelta is degrees
void Camera::turn(f32 yawDelta, f32 pitchDelta) {
  // NOTE: I am flipping the rotation with negative sines.
  // sin(-x) = -sin(x), cos(-x) = cos(x)
  f32 sineYaw = -sin(yawDelta);
  f32 cosineYaw = cos(yawDelta);

  vec3 newForward{};

  // yaw
  vec2 xyForward = {forward.x, forward.y};
  xyForward = normalize(xyForward);
  newForward.x = (xyForward.x * cosineYaw) + (xyForward.y * -sineYaw);
  newForward.y = (xyForward.x * sineYaw) + (xyForward.y * cosineYaw);

  // pitch
  pitch += pitchDelta;
  pitch = CLAMP(pitch, -maxPitch, maxPitch);
  f32 sinePitch = -sin(pitch);
  f32 cosinePitch = cos(pitch);
  newForward.z = sinePitch;
  newForward.x *= cosinePitch;
  newForward.y *= cosinePitch;

  setForward(newForward);
  configVectorsFromForward();
}

void Camera::setForward(vec3 forward) {
  Assert(magnitude(forward) > 0);
  vec3 newForward = normalize(forward);

  // invalid forward
  if(newForward.z > maxPitch_invSine || newForward.z < -maxPitch_invSine) {
    std::cout << "ERROR: invalid forward vector sent to camera" << std::endl;
    return;
  }

  this->forward = newForward;
  configVectorsFromForward();
}

void Camera::lookAt(vec3 focusPoint) {
  setForward(focusPoint - pos);
}

mat4 Camera::getViewMatrix() {
  mat4 measure{
          right.x, up.x, -forward.x, 0.f,
          right.y, up.y, -forward.y, 0.f,
          right.z, up.z, -forward.z, 0.f,
          0.f, 0.f, 0.f, 1.f
  };

  mat4 translate{
          1.f, 0.f, 0.f, 0.f,
          0.f, 1.f, 0.f, 0.f,
          0.f, 0.f, 1.f, 0.f,
          -pos.x, -pos.y, -pos.z, 1.f
  };
  return measure * translate;
}