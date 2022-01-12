#version 450

layout (location = 0) out vec4 fragColor;

layout( push_constant ) uniform constants
{
	float time;
	float resolutionX;
	float resolutionY;
} pc;

const vec3 colors[] = {
  vec3(0.588, 0.808, 0.706),
  vec3(0.118, 0.317, 0.157)
};

float sqrt3 = 1.732;

float HexDist(vec2 p){
    p = abs(p);
    
    float rightEdge = p.x;
    float upperRightEdge = dot(p, normalize(vec2(1.0, sqrt3)));
    return max(rightEdge, upperRightEdge);
}

void main()
{
    float resXYRatio = pc.resolutionX / pc.resolutionY;
    vec2 uv = ((gl_FragCoord.xy/pc.resolutionY) - 0.5*vec2(resXYRatio, 1)) * 2.0;
    
    vec2 uvScaled = uv * 3.0;
    vec2 gridScale = vec2(1.0, sqrt3);
    vec2 a = (mod(uvScaled, gridScale) - 0.5 * gridScale) * 2.0;
    vec2 b = (mod(uvScaled + 0.5*gridScale, gridScale) - 0.5 * gridScale) * 2.0;
    
    vec2 hexUV = dot(a,a) < dot(b,b) ? a : b;
    vec2 hexOffset = uv - hexUV; // uv rather then scaled UV for aesthetic reasons only
    float breath = (-cos(pc.time * 0.1) + 1.0) * 0.5 * 40.0; // "breath" that goes from 0 to 40 using -cos
    
    float on = 1.0 - step(cos(breath * hexOffset.x * hexOffset.y), HexDist(hexUV));
    vec3 color = colors[1] * on;

    // Output to screen
    fragColor = vec4(color, 1.0);
}