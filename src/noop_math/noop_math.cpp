#include "noop_math.h"

// TODO: No optimizations have been made in this file. Ideas: intrinsics, sse, better usage of temporary memory.

namespace noop {

// floating point
  b32 epsilonComparison(f32 a, f32 b, f32 epsilon) {
    f32 diff = a - b;
    return (diff <= epsilon && diff >= -epsilon);
  }

  b32 epsilonComparison(f64 a, f64 b, f64 epsilon) {
    f64 diff = a - b;
    return (diff <= epsilon && diff >= -epsilon);
  }

  f32 step(f32 edge, f32 x) {
    return x < edge ? 0.0f : 1.0f;
  }

  f32 clamp(f32 minVal, f32 maxVal, f32 x) {
    return Min(maxVal, Max(minVal, x));
  }

  f32 smoothStep(f32 edge1, f32 edge2, f32 x) {
    f32 t = clamp((x - edge1) / (edge2 - edge1), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
  }

  f32 lerp(f32 a, f32 b, f32 t) {
    Assert(t >= 0.0f && t <= 1.0f);
    return a - ((a + b) * t);
  }

  f32 sign(f32 x) {
    if(x > 0.0f) return (1.0f);
    if(x < 0.0f) return (-1.0f);
    return (0.0f);
  }

  f32 radians(f32 degrees) {
    return RadiansPerDegree * degrees;
  }

// vec2
  b32 operator==(const vec2& v1, const vec2& v2) {
    return epsilonComparison(v1.x, v2.x) &&
           epsilonComparison(v1.y, v2.y);
  }

  f32 dot(vec2 xy1, vec2 xy2) {
    return (xy1.x * xy2.x) + (xy1.y * xy2.y);
  }

  f32 magnitudeSquared(vec2 xy) {
    return (xy.x * xy.x) + (xy.y * xy.y);
  }

  f32 magnitude(vec2 xy) {
    return sqrtf((xy.x * xy.x) + (xy.y * xy.y));
  }

  vec2 normalize(const vec2& xy) {
    f32 mag = magnitude(xy);
    Assert(mag != 0.0f);
    f32 magInv = 1.0f / mag;
    return vec2{xy.x * magInv, xy.y * magInv};
  }

  vec2 normalize(f32 x, f32 y) {
    f32 mag = sqrtf(x * x + y * y);
    Assert(mag != 0.0f);
    f32 magInv = 1.0f / mag;
    return vec2{x * magInv, y * magInv};
  }

// less than a 90 degree angle between the two vectors
// v2 exists in the same half circle that centers around v1
  b32 similarDirection(vec2 v1, vec2 v2) {
    return dot(v1, v2) > 0.0f;
  }

  vec2 operator-(const vec2& xy) {
    return vec2{-xy.x, -xy.y};
  }

  vec2 operator-(const vec2& xy1, const vec2& xy2) {
    return vec2{xy1.x - xy2.x, xy1.y - xy2.y};
  }

  vec2 operator+(const vec2& xy1, const vec2& xy2) {
    return vec2{xy1.x + xy2.x, xy1.y + xy2.y};
  }

  void operator-=(vec2& xy1, const vec2& xy2) {
    xy1.x -= xy2.x;
    xy1.y -= xy2.y;
  }

  void operator+=(vec2& xy1, const vec2& xy2) {
    xy1.x += xy2.x;
    xy1.y += xy2.y;
  }

  vec2 operator*(f32 s, vec2 xy) {
    return vec2{xy.x * s, xy.y * s};
  }

  vec2 operator*(vec2 xy, f32 s) {
    return vec2{xy.x * s, xy.y * s};
  }

  void operator*=(vec2& xy, f32 s) {
    xy.x *= s;
    xy.y *= s;
  }

  vec2 operator/(const vec2& xy, const f32 s) {
    Assert(s != 0);
    f32 scaleInv = 1.0f / s;
    return {xy.x * scaleInv, xy.y * scaleInv};
  }

  vec2 operator/(const vec2& xy1, const vec2& xy2) {
    Assert(xy2.x != 0 && xy2.y != 0);
    return {xy1.x / xy2.x, xy1.y / xy2.y};
  }

  vec2 lerp(const vec2& a, const vec2& b, f32 t) {
    Assert(t >= 0.0f && t <= 1.0f);
    return a - ((a + b) * t);
  }

// vec3
  vec3 Vec3(vec2 xy, f32 z) {
    return vec3{xy.x, xy.y, z};
  }

  vec3 Vec3(f32 value) {
    return vec3{value, value, value};
  }

  b32 operator==(const vec3& v1, const vec3& v2) {
    return epsilonComparison(v1.x, v2.x) &&
           epsilonComparison(v1.y, v2.y) &&
           epsilonComparison(v1.z, v2.z);
  }

  vec3 operator-(const vec3& xyz) {
    return vec3{-xyz.x, -xyz.y, -xyz.z};
  }

  vec3 operator-(const vec3& xyz1, const vec3& xyz2) {
    return vec3{xyz1.x - xyz2.x, xyz1.y - xyz2.y, xyz1.z - xyz2.z};
  }

  vec3 operator+(const vec3& xyz1, const vec3& xyz2) {
    return vec3{xyz1.x + xyz2.x, xyz1.y + xyz2.y, xyz1.z + xyz2.z};
  }

  void operator-=(vec3& xyz1, const vec3& xyz2) {
    xyz1.x -= xyz2.x;
    xyz1.y -= xyz2.y;
    xyz1.z -= xyz2.z;
  }

  void operator+=(vec3& xyz1, const vec3& xyz2) {
    xyz1.x += xyz2.x;
    xyz1.y += xyz2.y;
    xyz1.z += xyz2.z;
  }

  vec3 operator*(f32 s, vec3 xyz) {
    return vec3{xyz.x * s, xyz.y * s, xyz.z * s};
  }

  vec3 operator*(vec3 xyz, f32 s) {
    return vec3{xyz.x * s, xyz.y * s, xyz.z * s};
  }

  void operator*=(vec3& xyz, f32 s) {
    xyz.x *= s;
    xyz.y *= s;
    xyz.z *= s;
  }

  vec3 operator/(const vec3& xyz, const f32 s) {
    Assert(s != 0);
    f32 scaleInv = 1.0f / s;
    return {xyz.x * scaleInv, xyz.y * scaleInv, xyz.z * scaleInv};
  }

  vec3 operator/(const vec3& xyz1, const vec3& xyz2) {
    Assert(xyz2.x != 0 && xyz2.y != 0 && xyz2.z != 0);
    return {xyz1.x / xyz2.x, xyz1.y / xyz2.y, xyz1.z / xyz2.z};
  }

  f32 dot(vec3 xyz1, vec3 xyz2) {
    return (xyz1.x * xyz2.x) + (xyz1.y * xyz2.y) + (xyz1.z * xyz2.z);
  }

  vec3 hadamard(const vec3& xyz1, const vec3& xyz2) {
    return vec3{xyz1.x * xyz2.x, xyz1.y * xyz2.y, xyz1.z * xyz2.z};
  }

  vec3 cross(const vec3& xyz1, const vec3& xyz2) {
    return vec3{(xyz1.y * xyz2.z - xyz2.y * xyz1.z),
                (xyz1.z * xyz2.x - xyz2.z * xyz1.x),
                (xyz1.x * xyz2.y - xyz2.x * xyz1.y)};
  }

  f32 magnitudeSquared(vec3 xyz) {
    return (xyz.x * xyz.x) + (xyz.y * xyz.y) + (xyz.z * xyz.z);
  }

  f32 magnitude(vec3 xyz) {
    return sqrtf((xyz.x * xyz.x) + (xyz.y * xyz.y) + (xyz.z * xyz.z));
  }

  vec3 normalize(const vec3& xyz) {
    f32 mag = magnitude(xyz);
    Assert(mag != 0.0f);
    f32 magInv = 1.0f / mag;
    return magInv * xyz;
  }

  vec3 normalize(f32 x, f32 y, f32 z) {
    f32 mag = sqrtf(x * x + y * y + z * z);
    Assert(mag != 0.0f);
    f32 magInv = 1.0f / mag;
    return vec3{x * magInv, y * magInv, z * magInv};
  }

  b32 degenerate(const vec3& v) {
    return v == vec3{0.0f, 0.0f, 0.0f};
  }

// v2 is assumed to be normalized
  vec3 projection(const vec3& v1, const vec3& ontoV2) {
    f32 dotV1V2 = dot(v1, ontoV2);
    return dotV1V2 * ontoV2;
  }

// v2 is assumed to be normalized
  vec3 perpendicularTo(const vec3& v1, const vec3& ontoV2) {
    vec3 parallel = projection(v1, ontoV2);
    return v1 - parallel;
  }

// less than a 90 degree angle between the two vectors
// v2 exists in the same hemisphere that centers around v1
  b32 similarDirection(const vec3& v1, const vec3& v2) {
    return dot(v1, v2) > 0.0f;
  }

  vec3 lerp(const vec3& a, const vec3& b, f32 t) {
    Assert(t >= 0.0f && t <= 1.0f);
    return a - ((a + b) * t);
  }

// vec4
  vec4 Vec4(vec3 xyz, f32 w) {
    return vec4{xyz.x, xyz.y, xyz.z, w};
  }

  vec4 Vec4(f32 x, vec3 yzw) {
    return vec4{x, yzw.val[0], yzw.val[1], yzw.val[2]};
  }

  vec4 Vec4(vec2 xy, vec2 zw) {
    return vec4{xy.x, xy.y, zw.val[0], zw.val[1]};
  }

  b32 operator==(const vec4& v1, const vec4& v2) {
    return epsilonComparison(v1.x, v2.x) &&
           epsilonComparison(v1.y, v2.y) &&
           epsilonComparison(v1.z, v2.z) &&
           epsilonComparison(v1.w, v2.w);
  }

  f32 dot(vec4 xyzw1, vec4 xyzw2) {
    return (xyzw1.x * xyzw2.x) + (xyzw1.y * xyzw2.y) + (xyzw1.z * xyzw2.z) + (xyzw1.w * xyzw2.w);
  }

  f32 magnitudeSquared(vec4 xyzw) {
    return (xyzw.x * xyzw.x) + (xyzw.y * xyzw.y) + (xyzw.z * xyzw.z) + (xyzw.w * xyzw.w);
  }

  f32 magnitude(vec4 xyzw) {
    return sqrtf((xyzw.x * xyzw.x) + (xyzw.y * xyzw.y) + (xyzw.z * xyzw.z) + (xyzw.w * xyzw.w));
  }

  vec4 normalize(const vec4& xyzw) {
    f32 mag = magnitude(xyzw);
    Assert(mag != 0.0f);
    f32 magInv = 1.0f / mag;
    return vec4{xyzw.x * magInv, xyzw.y * magInv, xyzw.z * magInv, xyzw.w * magInv};
  }

  vec4 normalize(f32 x, f32 y, f32 z, f32 w) {
    f32 mag = sqrtf(x * x + y * y + z * z + w * w);
    Assert(mag != 0.0f);
    f32 magInv = 1.0f / mag;
    return vec4{x * magInv, y * magInv, z * magInv, w * magInv};
  }

  vec4 operator-(const vec4& xyzw) {
    return vec4{-xyzw.x, -xyzw.y, -xyzw.z, -xyzw.w};
  }

  vec4 operator-(const vec4& xyzw1, const vec4& xyzw2) {
    return vec4{xyzw1.x - xyzw2.x, xyzw1.y - xyzw2.y, xyzw1.z - xyzw2.z, xyzw1.w - xyzw2.w};
  }

  vec4 operator+(const vec4& xyzw1, const vec4& xyzw2) {
    return vec4{xyzw1.x + xyzw2.x, xyzw1.y + xyzw2.y, xyzw1.z + xyzw2.z, xyzw1.w + xyzw2.w};
  }

  void operator-=(vec4& xyzw1, const vec4& xyzw2) {
    xyzw1.x -= xyzw2.x;
    xyzw1.y -= xyzw2.y;
    xyzw1.z -= xyzw2.z;
    xyzw1.w -= xyzw2.w;
  }

  void operator+=(vec4& xyzw1, const vec4& xyzw2) {
    xyzw1.x += xyzw2.x;
    xyzw1.y += xyzw2.y;
    xyzw1.z += xyzw2.z;
    xyzw1.w += xyzw2.w;
  }

  vec4 operator*(f32 s, vec4 xyzw) {
    return vec4{xyzw.x * s, xyzw.y * s, xyzw.z * s, xyzw.w * s};
  }

  vec4 operator*(vec4 xyzw, f32 s) {
    return vec4{xyzw.x * s, xyzw.y * s, xyzw.z * s, xyzw.w * s};
  }

  void operator*=(vec4& xyzw, f32 s) {
    xyzw.x *= s;
    xyzw.y *= s;
    xyzw.z *= s;
    xyzw.w *= s;
  }

  vec4 operator/(const vec4& xyzw, const f32 s) {
    Assert(s != 0);
    f32 scaleInv = 1.0f / s;
    return {xyzw.x * scaleInv, xyzw.y * scaleInv, xyzw.z * scaleInv, xyzw.w * scaleInv};
  }

  vec4 operator/(const vec4& xyzw1, const vec4& xyzw2) {
    Assert(xyzw2.x != 0 && xyzw2.y != 0 && xyzw2.z != 0 && xyzw2.w != 0);
    return {xyzw1.x / xyzw2.x, xyzw1.y / xyzw2.y, xyzw1.z / xyzw2.z, xyzw1.w / xyzw2.w};
  }

  vec4 lerp(const vec4& a, const vec4& b, f32 t) {
    Assert(t >= 0.0f && t <= 1.0f);
    return a - ((a + b) * t);
  }

  vec4 min(const vec4& xyzw1, const vec4& xyzw2) {
    return {Min(xyzw1.x, xyzw2.x), Min(xyzw1.y, xyzw2.y), Min(xyzw1.z, xyzw2.z), Min(xyzw1.w, xyzw2.w)};
  }

  vec4 max(const vec4& xyzw1, const vec4& xyzw2) {
    return {Max(xyzw1.x, xyzw2.x), Max(xyzw1.y, xyzw2.y), Max(xyzw1.z, xyzw2.z), Max(xyzw1.w, xyzw2.w)};
  }

// Complex
// This angle represents a counter-clockwise rotation
  complex Complex(f32 angle) {
    return {cosf(angle), sinf(angle)};
  }

  b32 operator==(const complex& c1, const complex& c2) {
    return epsilonComparison(c1.r, c2.r) &&
           epsilonComparison(c1.i, c2.i);
  }

  f32 magnitudeSquared(complex c) {
    return (c.r * c.r) + (c.i * c.i);
  }

  f32 magnitude(complex c) {
    return sqrtf(magnitudeSquared(c));
  }

// ii = -1
  vec2 operator*(const complex& ri, vec2 xy) {
#ifndef NDEBUG
    // Assert that we are only ever dealing with unit quaternions when rotating a vector
    f32 qMagn = magnitudeSquared(ri);
    Assert(qMagn < 1.001f && qMagn > .999f);
#endif

    return {
            (ri.r * xy.r) + -(ri.i * xy.i), // real
            (ri.r * xy.i) + (ri.i * xy.r) // imaginary
    };
  }

  void operator*=(vec2& xy, const complex& ri) {
#ifndef NDEBUG
    // Assert that we are only ever dealing with unit quaternions when rotating a vector
    f32 qMagn = magnitudeSquared(ri);
    Assert(qMagn < 1.001f && qMagn > .999f);
#endif

    f32 imaginaryTmp = (ri.i * xy.r);
    xy.x = (ri.r * xy.r) + -(ri.i * xy.i);
    xy.y = (ri.r * xy.i) + imaginaryTmp;
  }

// Quaternions
  quaternion Quaternion(vec3 v, f32 angle) {
    vec3 n = normalize(v);

    f32 halfA = angle * 0.5f;
    f32 cosHalfA = cosf(halfA);
    f32 sinHalfA = sinf(halfA);

    quaternion result;
    result.r = cosHalfA;
    result.ijk = sinHalfA * n;

    return result;
  }

  b32 operator==(const quaternion& q1, const quaternion& q2) {
    return epsilonComparison(q1.r, q2.r) &&
           epsilonComparison(q1.i, q2.i) &&
           epsilonComparison(q1.j, q2.j) &&
           epsilonComparison(q1.k, q2.k);
  }

  quaternion identity_quaternion() {
    return {1.0f, 0.0f, 0.0f, 0.0f};
  }

  f32 magnitudeSquared(quaternion rijk) {
    return (rijk.r * rijk.r) + (rijk.i * rijk.i) + (rijk.j * rijk.j) + (rijk.k * rijk.k);
  }

  f32 magnitude(quaternion q) {
    return sqrtf(magnitudeSquared(q));
  }

// NOTE: Conjugate is equal to the inverse when the quaternion is unit length
  quaternion conjugate(quaternion q) {
    return {q.r, -q.i, -q.j, -q.k};
  }

  f32 dot(const quaternion& q1, const quaternion& q2) {
    return (q1.r * q2.r) + (q1.i * q2.i) + (q1.j * q2.j) + (q1.k * q2.k);
  }

  quaternion normalize(const quaternion& q) {
    f32 mag = sqrtf((q.r * q.r) + (q.i * q.i) + (q.j * q.j) + (q.k * q.k));
    Assert(mag != 0.0f);
    f32 magInv = 1.0f / mag;
    return {q.r * magInv, q.i * magInv, q.j * magInv, q.k * magInv};
  }

  quaternion operator*(const quaternion& q1, const quaternion& q2) {
/*
 * Quaternion multiplication definitions
 * ii = jj = kk = ijk = -1
 * ij = -ji = k
 * jk = -kj = i
 * ki = -ik = j
 */

    quaternion q1q2;
    q1q2.r = (q1.r * q2.r) + -(q1.i * q2.i) + -(q1.j * q2.j) + -(q1.k * q2.k);
    q1q2.i = (q1.i * q2.r) + (q1.r * q2.i) + -(q1.k * q2.j) + (q1.j * q2.k);
    q1q2.j = (q1.j * q2.r) + (q1.r * q2.j) + (q1.k * q2.i) + -(q1.i * q2.k);
    q1q2.k = (q1.k * q2.r) + (q1.r * q2.k) + -(q1.j * q2.i) + (q1.i * q2.j);
    return q1q2;
  }

  vec3 rotate(const vec3& v, const quaternion& q) {
#ifndef NDEBUG
    // Assert that we are only ever dealing with unit quaternions when rotating a vector
    f32 qMagn = magnitudeSquared(q);
    Assert(qMagn < 1.001f && qMagn > .999f);
#endif

    quaternion result;
    quaternion qv;
    quaternion qInv = conjugate(q);

/*
 * Quaternion multiplication definitions
 * ii = jj = kk = ijk = -1
 * ij = -ji = k
 * jk = -kj = i
 * ki = -ik = j
 */

    qv.r = -(q.i * v.i) + -(q.j * v.j) + -(q.k * v.k);
    qv.i = (q.r * v.i) + -(q.k * v.j) + (q.j * v.k);
    qv.j = (q.r * v.j) + (q.k * v.i) + -(q.i * v.k);
    qv.k = (q.r * v.k) + -(q.j * v.i) + (q.i * v.j);

    // result.r = (qv.r * qInv.r) + -(qv.i * qInv.i) + -(qv.j * qInv.j) + -(qv.k * qInv.k); \\ equates to zero
    result.i = (qv.i * qInv.r) + (qv.r * qInv.i) + -(qv.k * qInv.j) + (qv.j * qInv.k);
    result.j = (qv.j * qInv.r) + (qv.k * qInv.i) + (qv.r * qInv.j) + -(qv.i * qInv.k);
    result.k = (qv.k * qInv.r) + -(qv.j * qInv.i) + (qv.i * qInv.j) + (qv.r * qInv.k);

    return result.ijk;
  }

  quaternion operator+(const quaternion& q1, const quaternion& q2) {
    return {q1.r + q2.r, q1.i + q2.i, q1.j + q2.j, q1.k + q2.k};
  }

  quaternion operator-(const quaternion& q1, const quaternion& q2) {
    return {q1.r - q2.r, q1.i - q2.i, q1.j - q2.j, q1.k - q2.k};
  }

  quaternion operator*(const quaternion& q1, const f32 s) {
    return {q1.r * s, q1.i * s, q1.j * s, q1.k * s};
  }

  quaternion operator*(const f32 s, const quaternion& q1) {
    return {q1.r * s, q1.i * s, q1.j * s, q1.k * s};
  }

  quaternion operator/(const quaternion& q1, const f32 s) {
    Assert(s != 0);
    f32 scaleInv = 1.0f / s;
    return {q1.r * scaleInv, q1.i * scaleInv, q1.j * scaleInv, q1.k * scaleInv};
  }

  quaternion lerp(quaternion a, quaternion b, f32 t) {
    Assert(t >= 0.0f && t <= 1.0f);
    return normalize(a - ((a + b) * t));
  }

// spherical linear interpolation
// this gives the shortest art on a 4d unit sphere
// great for smooth animations between two orientations defined by two quaternions
  quaternion slerp(quaternion a, quaternion b, f32 t) {
    Assert(t >= 0.0f && t <= 1.0f);
    f32 theta = acosf(dot(a, b));

    return ((sinf(theta * (1.0f - t)) * a) + (sinf(theta * t) * b)) / sinf(theta);
  }

// TODO: There is a whole 360 degrees of rotation around the endOrientation axis that will all result in a "correct"
// TODO: end orientation as described by the function parameters.
// TODO: A reasonable approach might be ensuring the result is essentially a yaw followed by a pitch with no roll.
  quaternion orient(const vec3& startOrientation, const vec3& endOrientation) {
    // TODO: more robust handling of close orientations
    if(startOrientation == endOrientation) {
      return identity_quaternion();
    } else if(startOrientation == -endOrientation) {
      return Quaternion(vec3{0.0f, 0.0f, 1.0f}, 180.0f * RadiansPerDegree);
    }
    vec3 unitStartOrientation = normalize(startOrientation);
    vec3 unitEndOrientation = normalize(endOrientation);
    f32 theta = acosf(dot(unitStartOrientation, unitEndOrientation));
    vec3 unitPerp = normalize(cross(unitStartOrientation, unitEndOrientation));
    return Quaternion(unitPerp, theta);
  }

// mat2
  b32 operator==(const mat2& A, const mat2& B) {
    return A.col[0] == B.col[0] &&
           A.col[1] == B.col[1];
  }

  mat2 identity_mat2() {
    return mat2{
            1.0f, 0.0f,
            0.0f, 1.0f,
    };
  }

  mat2 scale_mat2(f32 scale) {
    return mat2{
            scale, 0.0f,
            0.0f, scale,
    };
  }

  mat2 scale_mat2(vec3 scale) {
    return mat2{
            scale.x, 0.0f,
            0.0f, scale.y,
    };
  }

  mat2 transpose(const mat2& A) {
    return mat2{
            A.val2d[0][0], A.val2d[1][0],
            A.val2d[0][1], A.val2d[1][1],
    };
  }

  mat2 rotate(f32 radians) {
    f32 sine = (f32)sin(radians);
    f32 cosine = (f32)cos(radians);
    return mat2{cosine, -sine, sine, cosine};
  }

// mat3
  b32 operator==(const mat3& A, const mat3& B) {
    return A.col[0] == B.col[0] &&
           A.col[1] == B.col[1] &&
           A.col[2] == B.col[2];
  }

  mat3 identity_mat3() {
    return mat3{
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f,
    };
  }

  mat3 scale_mat3(f32 scale) {
    return mat3{
            scale, 0.0f, 0.0f,
            0.0f, scale, 0.0f,
            0.0f, 0.0f, scale,
    };
  }

  mat3 scale_mat3(vec3 scale) {
    return mat3{
            scale.x, 0.0f, 0.0f,
            0.0f, scale.y, 0.0f,
            0.0f, 0.0f, scale.z,
    };
  }

  mat3 transpose(const mat3& A) {
    return mat3{
            A.val2d[0][0], A.val2d[1][0], A.val2d[2][0],
            A.val2d[0][1], A.val2d[1][1], A.val2d[2][1],
            A.val2d[0][2], A.val2d[1][2], A.val2d[2][2],
    };
  }

// radians in radians
  mat3 rotate_mat3(f32 radians, vec3 v) {
    vec3 axis(normalize(v));

    f32 const cosA = cosf(radians);
    f32 const sinA = sinf(radians);
    vec3 const axisTimesOneMinusCos = axis * (1.0f - cosA);

    mat3 rotate;
    rotate.val2d[0][0] = axis.x * axisTimesOneMinusCos.x + cosA;
    rotate.val2d[0][1] = axis.x * axisTimesOneMinusCos.y + sinA * axis.z;
    rotate.val2d[0][2] = axis.x * axisTimesOneMinusCos.z - sinA * axis.y;

    rotate.val2d[1][0] = axis.y * axisTimesOneMinusCos.x - sinA * axis.z;
    rotate.val2d[1][1] = axis.y * axisTimesOneMinusCos.y + cosA;
    rotate.val2d[1][2] = axis.y * axisTimesOneMinusCos.z + sinA * axis.x;

    rotate.val2d[2][0] = axis.z * axisTimesOneMinusCos.x + sinA * axis.y;
    rotate.val2d[2][1] = axis.z * axisTimesOneMinusCos.y - sinA * axis.x;
    rotate.val2d[2][2] = axis.z * axisTimesOneMinusCos.z + cosA;

    return rotate;
  }

  mat3 rotate_mat3(quaternion q) {
    mat3 resultMat{}; // zero out matrix
    resultMat.xTransform = rotate(vec3{1.0f, 0.0f, 0.0f}, q);
    resultMat.yTransform = rotate(vec3{0.0f, 1.0f, 0.0f}, q);
    resultMat.zTransform = rotate(vec3{0.0f, 0.0f, 1.0f}, q);
    return resultMat;
  }

  vec3 operator*(const mat3& M, const vec3& v) {
    vec3 result = M.col[0] * v.x;
    result += M.col[1] * v.y;
    result += M.col[2] * v.z;
    return result;
  }

  mat3 operator*(const mat3& A, const mat3& B) {
    mat3 result;

    mat3 transposeA = transpose(A); // cols <=> rows
    result.val2d[0][0] = dot(transposeA.col[0], B.col[0]);
    result.val2d[0][1] = dot(transposeA.col[1], B.col[0]);
    result.val2d[0][2] = dot(transposeA.col[2], B.col[0]);
    result.val2d[1][0] = dot(transposeA.col[0], B.col[1]);
    result.val2d[1][1] = dot(transposeA.col[1], B.col[1]);
    result.val2d[1][2] = dot(transposeA.col[2], B.col[1]);
    result.val2d[2][0] = dot(transposeA.col[0], B.col[2]);
    result.val2d[2][1] = dot(transposeA.col[1], B.col[2]);
    result.val2d[2][2] = dot(transposeA.col[2], B.col[2]);

    return result;
  }

// mat4
  mat4 Mat4(mat3 M) {
    return mat4{
            M.val2d[0][1], M.val2d[0][1], M.val2d[0][1], 0.0f,
            M.val2d[0][1], M.val2d[0][1], M.val2d[0][1], 0.0f,
            M.val2d[0][1], M.val2d[0][1], M.val2d[0][1], 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
    };
  }

  b32 operator==(const mat4& A, const mat4& B) {
    return A.col[0] == B.col[0] &&
           A.col[1] == B.col[1] &&
           A.col[2] == B.col[2] &&
           A.col[3] == B.col[3];
  }

  mat4 identity_mat4() {
    return mat4{
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
    };
  }

  mat4 scale_mat4(f32 scale) {
    return mat4{
            scale, 0.0f, 0.0f, 0.0f,
            0.0f, scale, 0.0f, 0.0f,
            0.0f, 0.0f, scale, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
    };
  }

  mat4 scale_mat4(vec3 scale) {
    return mat4{
            scale.x, 0.0f, 0.0f, 0.0f,
            0.0f, scale.y, 0.0f, 0.0f,
            0.0f, 0.0f, scale.z, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
    };
  }

  mat4 translate_mat4(vec3 translation) {
    return mat4{
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            translation.x, translation.y, translation.z, 1.0f,
    };
  }

  mat4 transpose(const mat4& A) {
    return mat4{
            A.val2d[0][0], A.val2d[1][0], A.val2d[2][0], A.val2d[3][0],
            A.val2d[0][1], A.val2d[1][1], A.val2d[2][1], A.val2d[3][1],
            A.val2d[0][2], A.val2d[1][2], A.val2d[2][2], A.val2d[3][2],
            A.val2d[0][3], A.val2d[1][3], A.val2d[2][3], A.val2d[3][3],
    };
  }

// radians in radians
  mat4 rotate_xyPlane_mat4(f32 radians) {
    f32 const cosA = cosf(radians);
    f32 const sinA = sinf(radians);

    mat4 rotate;
    rotate.xTransform = {cosA, sinA, 0.0f, 0.0f};
    rotate.yTransform = {-sinA, cosA, 0.0f, 0.0f};
    rotate.zTransform = {0.0f, 0.0f, 1.0f, 0.0f};
    rotate.translation = {0.0f, 0.0f, 0.0f, 1.0f};

    return rotate;
  }

// radians in radians
  mat4 rotate_mat4(f32 radians, vec3 v) {
    vec3 axis(normalize(v));

    f32 const cosA = cosf(radians);
    f32 const sinA = sinf(radians);
    vec3 const axisTimesOneMinusCos = axis * (1.0f - cosA);

    mat4 rotate;
    rotate.val2d[0][0] = axis.x * axisTimesOneMinusCos.x + cosA;
    rotate.val2d[0][1] = axis.x * axisTimesOneMinusCos.y + sinA * axis.z;
    rotate.val2d[0][2] = axis.x * axisTimesOneMinusCos.z - sinA * axis.y;
    rotate.val2d[0][3] = 0;

    rotate.val2d[1][0] = axis.y * axisTimesOneMinusCos.x - sinA * axis.z;
    rotate.val2d[1][1] = axis.y * axisTimesOneMinusCos.y + cosA;
    rotate.val2d[1][2] = axis.y * axisTimesOneMinusCos.z + sinA * axis.x;
    rotate.val2d[1][3] = 0;

    rotate.val2d[2][0] = axis.z * axisTimesOneMinusCos.x + sinA * axis.y;
    rotate.val2d[2][1] = axis.z * axisTimesOneMinusCos.y - sinA * axis.x;
    rotate.val2d[2][2] = axis.z * axisTimesOneMinusCos.z + cosA;
    rotate.val2d[2][3] = 0;

    rotate.col[3] = {0.0f, 0.0f, 0.0f, 1.0f};

    return rotate;
  }

  mat4 scaleTrans_mat4(const f32 scale, const vec3& translation) {
    return mat4{
            scale, 0.0f, 0.0f, 0.0f,
            0.0f, scale, 0.0f, 0.0f,
            0.0f, 0.0f, scale, 0.0f,
            translation.x, translation.y, translation.z, 1.0f,
    };
  }

  mat4 scaleTrans_mat4(const vec3& scale, const vec3& translation) {
    return mat4{
            scale.x, 0.0f, 0.0f, 0.0f,
            0.0f, scale.y, 0.0f, 0.0f,
            0.0f, 0.0f, scale.z, 0.0f,
            translation.x, translation.y, translation.z, 1.0f,
    };
  }

  mat4 scaleRotTrans_mat4(const vec3& scale, const vec3& rotAxis, const f32 angle, const vec3& translation) {
    vec3 axis(normalize(rotAxis));

    f32 const cosA = cosf(angle);
    f32 const sinA = sinf(angle);
    vec3 const axisTimesOneMinusCos = axis * (1.0f - cosA);

    mat4 scaleRotTransMat;
    scaleRotTransMat.val2d[0][0] = (axis.x * axisTimesOneMinusCos.x + cosA) * scale.x;
    scaleRotTransMat.val2d[0][1] = (axis.x * axisTimesOneMinusCos.y + sinA * axis.z) * scale.x;
    scaleRotTransMat.val2d[0][2] = (axis.x * axisTimesOneMinusCos.z - sinA * axis.y) * scale.x;
    scaleRotTransMat.val2d[0][3] = 0;

    scaleRotTransMat.val2d[1][0] = (axis.y * axisTimesOneMinusCos.x - sinA * axis.z) * scale.y;
    scaleRotTransMat.val2d[1][1] = (axis.y * axisTimesOneMinusCos.y + cosA) * scale.y;
    scaleRotTransMat.val2d[1][2] = (axis.y * axisTimesOneMinusCos.z + sinA * axis.x) * scale.y;
    scaleRotTransMat.val2d[1][3] = 0;

    scaleRotTransMat.val2d[2][0] = (axis.z * axisTimesOneMinusCos.x + sinA * axis.y) * scale.z;
    scaleRotTransMat.val2d[2][1] = (axis.z * axisTimesOneMinusCos.y - sinA * axis.x) * scale.z;
    scaleRotTransMat.val2d[2][2] = (axis.z * axisTimesOneMinusCos.z + cosA) * scale.z;
    scaleRotTransMat.val2d[2][3] = 0;

    scaleRotTransMat.translation.xyz = translation;
    scaleRotTransMat.val2d[3][3] = 1.0f;

    return scaleRotTransMat;
  }

  mat4 scaleRotTrans_mat4(const vec3& scale, const quaternion& q, const vec3& translation) {
    mat4 scaleRotTransMat;
    scaleRotTransMat.xTransform.xyz = rotate(vec3{1.0f, 0.0f, 0.0f}, q) * scale.x;
    scaleRotTransMat.yTransform.xyz = rotate(vec3{0.0f, 1.0f, 0.0f}, q) * scale.y;
    scaleRotTransMat.zTransform.xyz = rotate(vec3{0.0f, 0.0f, 1.0f}, q) * scale.z;
    scaleRotTransMat.translation.xyz = translation;

    scaleRotTransMat.val2d[0][3] = 0.0f;
    scaleRotTransMat.val2d[1][3] = 0.0f;
    scaleRotTransMat.val2d[2][3] = 0.0f;
    scaleRotTransMat.val2d[3][3] = 1.0f;

    return scaleRotTransMat;
  }

  mat4 rotate_mat4(quaternion q) {
    mat4 resultMat{}; // zero out matrix
    resultMat.xTransform.xyz = rotate(vec3{1.0f, 0.0f, 0.0f}, q);
    resultMat.yTransform.xyz = rotate(vec3{0.0f, 1.0f, 0.0f}, q);
    resultMat.zTransform.xyz = rotate(vec3{0.0f, 0.0f, 1.0f}, q);
    resultMat.val2d[3][3] = 1.0f;
    return resultMat;
  }

  vec4 operator*(const mat4& M, const vec4& v) {
    vec4 result = M.col[0] * v.x;
    result += M.col[1] * v.y;
    result += M.col[2] * v.z;
    result += M.col[3] * v.w;
    return result;
  }

  mat4 operator*(const mat4& A, const mat4& B) {
    mat4 result;

    mat4 transposeA = transpose(A); // cols <=> rows
    result.val2d[0][0] = dot(transposeA.col[0], B.col[0]);
    result.val2d[0][1] = dot(transposeA.col[1], B.col[0]);
    result.val2d[0][2] = dot(transposeA.col[2], B.col[0]);
    result.val2d[0][3] = dot(transposeA.col[3], B.col[0]);
    result.val2d[1][0] = dot(transposeA.col[0], B.col[1]);
    result.val2d[1][1] = dot(transposeA.col[1], B.col[1]);
    result.val2d[1][2] = dot(transposeA.col[2], B.col[1]);
    result.val2d[1][3] = dot(transposeA.col[3], B.col[1]);
    result.val2d[2][0] = dot(transposeA.col[0], B.col[2]);
    result.val2d[2][1] = dot(transposeA.col[1], B.col[2]);
    result.val2d[2][2] = dot(transposeA.col[2], B.col[2]);
    result.val2d[2][3] = dot(transposeA.col[3], B.col[2]);
    result.val2d[3][0] = dot(transposeA.col[0], B.col[3]);
    result.val2d[3][1] = dot(transposeA.col[1], B.col[3]);
    result.val2d[3][2] = dot(transposeA.col[2], B.col[3]);
    result.val2d[3][3] = dot(transposeA.col[3], B.col[3]);

    return result;
  }

// real-time rendering 4.7.2
// ex: screenWidth = 20.0f, screenDist = 30.0f will provide the horizontal field of view
// for a person sitting 30 inches away from a 20 inch screen, assuming the screen is
// perpendicular to the line of sight.
// NOTE: Any units work as long as they are the same. Works for vertical and horizontal.
  f32 fieldOfView(f32 screenWidth, f32 screenDist) {
    f32 phi = 2.0f * atanf(screenWidth / (2.0f * screenDist));
    return phi;
  }

// real-time rendering 4.7.1
// This projection is for a canonical view volume goes from <-1,1>
  mat4 orthographic(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
    // NOTE: Projection matrices are all about creating properly placing
    // objects in the canonical view volume, which in the case of OpenGL
    // is from <-1,-1,-1> to <1,1,1>.
    // The 3x3 matrix is dividing vectors/points by the specified dimensions
    // and multiplying by two. That is because the the canonical view volume
    // in our case actually has dimensions of <2,2,2>. So we map our specified
    // dimensions between the values of 0 and 2
    // The translation value is necessary in order to translate the values from
    // the range of 0 and 2 to the range of -1 and 1
    // The translation may look odd and that is because this matrix is a
    // combination of a Scale matrix, S, and a translation matrix, T, in
    // which the translation must be performed first. Taking the x translation
    // for example, we want to translate it by -(l + r) / 2. Which would
    // effectively move the origin's x value in the center of l & r. However,
    // the scaling changes that translation to [(2 / (r + l)) * (-(r + l) / 2)],
    // which simplifies to what is seen below for the x translation.
    return {
            2 / (r - l), 0, 0, 0,
            0, 2 / (t - b), 0, 0,
            0, 0, 2 / (f - n), 0,
            -(r + l) / (r - l), -(t + b) / (t - b), -(f + n) / (f - n), 1
    };
  }

// real-time rendering 4.7.2
  mat4 perspective(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
    mat4 result;

    result.xTransform = {(2.0f * n) / (r - l), 0.0f, 0.0f, 0.0f};
    result.yTransform = {0.0f, (2.0f * n) / (t - b), 0.0f, 0.0f};
    result.zTransform = {(r + l) / (r - l), (t + b) / (t - b), -(f + n) / (f - n), -1.0f};
    result.translation = {0.0f, 0.0f, -(2.0f * f * n) / (f - n), 0.0f};

    return result;
  }

  mat4 perspectiveInverse(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
    mat4 result;

    result.xTransform = {(r - l) / (2.0f * n), 0.0f, 0.0f, 0.0f};
    result.yTransform = {0.0f, (t - b) / (2.0f * n), 0.0f, 0.0f};
    result.zTransform = {0.0f, 0.0f, 0.0f, -(f - n) / (2 * f * n)};
    result.translation = {(r + l) / (2.0f * n), (t + b) / (2.0f * n), -1.0f, (f + n) / (2 * f * n)};

    return result;
  }

// real-time rendering 4.7.2
// aspect ratio is equivalent to width / height
  mat4 perspective(f32 fovVert, f32 aspect, f32 n, f32 f) {
    mat4 result;

    const f32 c = 1.0f / tanf(fovVert / 2.0f);
    result.xTransform = {(c / aspect), 0.0f, 0.0f, 0.0f};
    result.yTransform = {0.0f, c, 0.0f, 0.0f};
    result.zTransform = {0.0f, 0.0f, -(f + n) / (f - n), -1.0f};
    result.translation = {0.0f, 0.0f, -(2.0f * f * n) / (f - n), 0.0f};

    return result;
  }

  mat4 perspectiveInverse(f32 fovVert, f32 aspect, f32 n, f32 f) {
    mat4 result;

    const f32 c = 1.0f / tanf(fovVert / 2.0f);
    result.xTransform = {aspect / c, 0.0f, 0.0f, 0.0f};
    result.yTransform = {0.0f, 1.0f / c, 0.0f, 0.0f};
    result.zTransform = {0.0f, 0.0f, 0.0f, -(f - n) / (2.0f * f * n)};
    result.translation = {0.0f, 0.0f, -1.0f, (f + n) / (2.0f * f * n)};

    return result;
  }

/*
 * // NOTE: Oblique View Frustum Depth Projection and Clipping by Eric Lengyel (Terathon Software)
 * Arguments:
 * mat4 perspectiveMat: Perspective matrix that is being used for the rest of the scene
 * vec3 planeNormal_viewSpace: Plane normal in view space. MUST be normalized. This is a normal that points INTO the frustum, NOT one that is generally pointing towards the camera.
 * vec3 planePos_viewSpace: Any position on the plane in view space.
 */
  mat4 obliquePerspective(const mat4& perspectiveMat, vec3 planeNormal_viewSpace, vec3 planePos_viewSpace, f32 farPlane) {

    mat4 persp = perspectiveMat;

    // plane = {normal.x, normal.y, normal.z, dist}
    vec4 plane_viewSpace = Vec4(planeNormal_viewSpace, dot(-planeNormal_viewSpace, planePos_viewSpace));

    // clip space plane
    vec4 oppositeFrustumCorner_viewSpace = {
            sign(plane_viewSpace.x) * (1.0f / perspectiveMat.xTransform.x),
            sign(plane_viewSpace.y) * (1.0f / perspectiveMat.yTransform.y),
            -1.0f,
            1.0f / farPlane
    };

    vec4 scaledPlane_viewSpace = ((-2.0f * oppositeFrustumCorner_viewSpace.z) / dot(plane_viewSpace, oppositeFrustumCorner_viewSpace)) * plane_viewSpace;

    persp.val2d[0][2] = scaledPlane_viewSpace.x;
    persp.val2d[1][2] = scaledPlane_viewSpace.y;
    persp.val2d[2][2] = scaledPlane_viewSpace.z + 1.0f;
    persp.val2d[3][2] = scaledPlane_viewSpace.w;

    return persp;
  }

/*
 * // NOTE: Oblique View Frustum Depth Projection and Clipping by Eric Lengyel (Terathon Software)
 * Arguments:
 * vec3 planeNormal_viewSpace: Plane normal in view space. MUST be normalized. This is a normal that points INTO the frustum, NOT one that is generally pointing towards the camera.
 * vec3 planePos_viewSpace: Any position on the plane in view space.
 */
  mat4 obliquePerspective(f32 fovVert, f32 aspect, f32 nearPlane, f32 farPlane, vec3 planeNormal_viewSpace, vec3 planePos_viewSpace) {
    const f32 c = 1.0f / tanf(fovVert / 2.0f);

    // plane = {normal.x, normal.y, normal.z, dist}
    vec4 plane_viewSpace = Vec4(planeNormal_viewSpace, dot(-planeNormal_viewSpace, planePos_viewSpace));

    // clip space plane
    vec4 oppositeFrustumCorner_viewSpace = {
            sign(plane_viewSpace.x) * (aspect / c),
            sign(plane_viewSpace.y) * (1.0f / c),
            -1.0f,
            1.0f / farPlane
    };

    vec4 scaledPlane_viewSpace = ((-2.0f * oppositeFrustumCorner_viewSpace.z) / dot(plane_viewSpace, oppositeFrustumCorner_viewSpace)) * plane_viewSpace;

    mat4 resultMat = {
            (c / aspect), 0.0f, scaledPlane_viewSpace.x, 0.0f,
            0.0f, c, scaledPlane_viewSpace.y, 0.0f,
            0.0f, 0.0f, scaledPlane_viewSpace.z + 1.0f, -1.0f,
            0.0f, 0.0f, scaledPlane_viewSpace.w, 0.0f
    };

    return resultMat;
  }


  void adjustAspectPerspProj(mat4* projectionMatrix, f32 fovVert, f32 aspect) {
    const f32 c = 1.0f / tanf(fovVert / 2.0f);
    (*projectionMatrix).val2d[0][0] = c / aspect;
    (*projectionMatrix).val2d[1][1] = c;
  }

  void adjustNearFarPerspProj(mat4* projectionMatrix, f32 n, f32 f) {
    (*projectionMatrix).val2d[2][2] = -(f + n) / (f - n);
    (*projectionMatrix).val2d[3][2] = -(2.0f * f * n) / (f - n);
  }

// etc
  bool insideRect(BoundingRect boundingRect, vec2 position) {
    const vec2 boundingRectMax = boundingRect.min + boundingRect.diagonal;
    return (position.x > boundingRect.min.x &&
            position.x < boundingRectMax.x &&
            position.y > boundingRect.min.y &&
            position.y < boundingRectMax.y);
  }

  bool insideBox(BoundingBox boundingBox, vec3 position) {
    const vec3 boundingBoxMax = boundingBox.min + boundingBox.diagonal;
    return (position.x > boundingBox.min.x &&
            position.x < boundingBoxMax.x &&
            position.y > boundingBox.min.y &&
            position.y < boundingBoxMax.y &&
            position.z > boundingBox.min.z &&
            position.z < boundingBoxMax.z);
  }

  bool overlap(BoundingRect bbA, BoundingRect bbB) {
    const vec2 bbAMax = bbA.min + bbA.diagonal;
    const vec2 bbBMax = bbB.min + bbB.diagonal;
    return ((bbBMax.x > bbA.min.x && bbB.min.x < bbAMax.x) && // overlap in X
            (bbBMax.y > bbA.min.y && bbB.min.y < bbAMax.x));   // overlap in Y
  }

  bool overlap(BoundingBox bbA, BoundingBox bbB) {
    const vec3 bbAMax = bbA.min + bbA.diagonal;
    const vec3 bbBMax = bbB.min + bbB.diagonal;
    return ((bbBMax.x > bbA.min.x && bbB.min.x < bbAMax.x) && // overlap in X
            (bbBMax.y > bbA.min.y && bbB.min.y < bbAMax.y) && // overlap in X
            (bbBMax.z > bbA.min.z && bbB.min.z < bbAMax.z));   // overlap in Z
  }
}