#version 450

layout (location = 0) out vec4 fragColor;

layout( push_constant ) uniform constants
{
	float time;
	float resolutionX;
	float resolutionY;
} pc;

#define MAX_STEPS 60 // less steps will increase performance but narrow viewing distance, and lower detail & overall brightness
#define MISS_DIST 200.0
#define HIT_DIST 0.01
    
int distanceRayToScene(vec3 rayOrigin, vec3 rayDir);
float distPosToScene(vec3 rayPos);

mat2 rotate(float angle);

float sdRect(vec2 rayPos, vec2 dimen);
float sdCross(vec3 rayPos, vec3 dimen);
float sdCube(vec3 rayPos);
float sdMengerNoisePrison(vec3 rayPos);

const vec3 missColor = vec3(0.1, 0.1, 0.1);
const float boxDimen = 20.0;
const float halfBoxDimen = boxDimen / 2.0;

const int mengerSpongeIterations = 3;
const float pi = 3.14159265;
const float piOver4 = pi * 0.25;
const float tau = pi * 2.0f;
const vec3 startingRayOrigin = vec3(0.0, 0.0, -boxDimen);

const float velocityUnit = boxDimen; // velocity in terms of length of the largest menger cube
const float velocity = 0.1; // velocity units per second
const float secondsPerCycle = 4.0 / velocity;
const float cyclesPerSecond = 1.0 / secondsPerCycle;

const float highYLevel[3] = float[](1.0, 0.5 + (3.0 / 18.0), 0.5 + (3.0 / 54.0));
const vec2 upRightLeftDownVecs[4] = vec2[](vec2(0.0, 1.0), vec2(1.0, 0.0), vec2(0.0, -1.0), vec2(-1.0, 0.0));

void main()
{    
    vec2 pixelCoord = gl_FragCoord.xy - (vec2(pc.resolutionX, pc.resolutionY) * 0.5);
    pixelCoord = pixelCoord / pc.resolutionY;
    
    float cycle = pc.time * cyclesPerSecond;
    int cycleTrunc = int(cycle);
    float cosY = -cos(tau * cycle); // cycles per second
    float smoothY = smoothstep(-0.60, 0.60, cosY);
    int upRightLeftDownIndex = cycleTrunc % 4;
    int highYLevelIndex = cycleTrunc % 3;
    float highLevelY = highYLevel[highYLevelIndex];
    
    float offset1d = smoothY * boxDimen * highLevelY;
    vec3 offset3d = vec3(offset1d * upRightLeftDownVecs[upRightLeftDownIndex], pc.time * velocity * velocityUnit);
    vec3 rayOrigin = startingRayOrigin + offset3d;
    vec3 fragmentPos = vec3(pixelCoord.x, pixelCoord.y, 0.0);
    const vec3 focalPoint = vec3(0.0, 0.0, -1.0);
    vec3 rayDir = fragmentPos - focalPoint;
    rayDir = normalize(rayDir);
    
    int iteration = distanceRayToScene(rayOrigin, rayDir);

    if(iteration < MAX_STEPS) { // hit
        vec3 col = vec3(1.0 - (float(iteration)/float(MAX_STEPS)));
        fragColor = vec4(col, 1.0);
    } else { // miss
        fragColor = vec4(missColor, 1.0);
    }
}

// returns num iterations
// NOTE: ray dir arguments are assumed to be normalized
int distanceRayToScene(vec3 rayOrigin, vec3 rayDir) {

	float dist = 0.0;

	for(int i = 0; i < MAX_STEPS; i++) {
        vec3 pos = rayOrigin + (dist * rayDir);
        float posToScene = sdMengerNoisePrison(pos);
        dist += posToScene;
        if(abs(posToScene) < HIT_DIST) return i; // absolute value for posToScene incase the ray makes its way inside an object
        if(posToScene > MISS_DIST) break;
    }

    return MAX_STEPS;
}

float sdCube(vec3 rayPos) {
	const vec3 corner = vec3(halfBoxDimen);
    vec3 ray = abs(rayPos); // fold ray into positive octant
    vec3 cornerToRay = ray - corner;
    float cornerToRayMaxComponent = max(max(cornerToRay.x, cornerToRay.y), cornerToRay.z);
    float distToInsideRay = min(cornerToRayMaxComponent, 0.0);
    vec3 closestToOutsideRay = max(cornerToRay, 0.0);
	return length(closestToOutsideRay) + distToInsideRay;
}


float sdCross(vec3 rayPos) {
    const vec3 corner = vec3(halfBoxDimen);
	vec3 ray = abs(rayPos); // fold ray into positive quadrant
	vec3 cornerToRay = ray - corner;

    float smallestComp = min(min(cornerToRay.x, cornerToRay.y), cornerToRay.z);
	float largestComp = max(max(cornerToRay.x, cornerToRay.y), cornerToRay.z);
	float middleComp = cornerToRay.x + cornerToRay.y + cornerToRay.z
							- smallestComp - largestComp;
            
	vec2 closestOutsidePoint = max(vec2(smallestComp, middleComp), 0.0);
	vec2 closestInsidePoint = min(vec2(middleComp, largestComp), 0.0);

	return (middleComp > 0.0) ? length(closestOutsidePoint) : -length(closestInsidePoint);
}

float sdMengerNoisePrison(vec3 rayPos) {
  vec3 prisonRay = mod(rayPos, boxDimen * 2.0);
  prisonRay -= boxDimen;

  float mengerPrisonDist = sdCross(prisonRay, vec3(halfBoxDimen));

  float scale = 1.0;;
  for(int i = 0; i < 3; ++i) {
    float boxedWorldDimen = boxDimen / scale;
    vec3 ray = mod(rayPos + boxedWorldDimen / 2.0, boxedWorldDimen);
    ray -= boxedWorldDimen * 0.5;
    ray *= scale;
    float crossesDist = sdCross(ray * 3.0, vec3(halfBoxDimen));
    scale *= 3.0;
    crossesDist /= scale;
    mengerPrisonDist = max(mengerPrisonDist, -crossesDist);
  }

  return mengerPrisonDist;
}


float sdRect(vec2 rayPos, vec2 dimen) {
  vec2 rayToCorner = abs(rayPos) - dimen;
  // maxDelta is the maximum negative value if the point exists inside of the box, otherwise 0.0
  float maxDelta = min(max(rayToCorner.x, rayToCorner.y), 0.0);
  return length(max(rayToCorner, 0.0)) + maxDelta;
}

float sdCross(vec3 rayPos, vec3 dimen) {
  float da = sdRect(rayPos.xy, dimen.xy);
  float db = sdRect(rayPos.xz, dimen.xz);
  float dc = sdRect(rayPos.yz, dimen.yz);
  return min(da,min(db,dc));
}

mat2 rotate(float angle) {
    float sine = sin(angle);
    float cosine = cos(angle);
    return mat2(cosine, -sine, sine, cosine);
}