#version 450

layout (location = 0) out vec4 fragColor;

layout( push_constant ) uniform constants
{
	float time;
	float resolutionX;
	float resolutionY;
} pc;

float sqrt3 = 1.732;
float scale = 10.0;

vec3 colors[4] = vec3[4](
                vec3(255.0, 218.0, 169.0) * (1.0 / 255.0), // yellow
                vec3(111.0, 163.0, 169.0) * (1.0 / 255.0), // turqoise (greenish)
                vec3(95.0, 114.0, 178.0) * (1.0 / 255.0), // blue
                vec3(96.0, 54.0, 111.0) * (1.0 / 255.0) // purple
);

vec3 simpleGamma(vec3 col)
{
    return pow(col, vec3(2.2/1.0));
}

float HexDist(vec2 p){
    p = abs(p);
    
    float rightEdge = p.x;
    float upperRightEdge = dot(p, normalize(vec2(1.0, sqrt3)));
    return max(rightEdge, upperRightEdge);
}

vec2 complexMul(vec2 v, vec2 c) {
    float real = (v.x * c.x) - (v.y * c.y);
    float imaginary = (v.x * c.y) + (v.y * c.x);
    return vec2(real, imaginary);
}

void main()
{
    float time = pc.time * 0.2;
    float resXYRatio = pc.resolutionX / pc.resolutionY;
    vec2 uv = ((gl_FragCoord.xy /pc.resolutionY) - 0.5*vec2(resXYRatio, 1)) * 2.0;
    vec2 cRotateZoom = vec2(cos(time * 0.2), sin(time));
    uv = complexMul(uv, cRotateZoom);
    uv.x += time + 5.0;
    
    vec2 uvScaled = uv * scale;
    vec2 gridScale = vec2(1.0, sqrt3);
    vec2 a = (mod(uvScaled, gridScale) - 0.5 * gridScale);
    vec2 b = (mod(uvScaled + 0.5*gridScale, gridScale) - 0.5 * gridScale);
    
    vec2 hexUV = dot(a,a) < dot(b,b) ? a : b;
    vec2 hexIndex = round(uvScaled - hexUV);
    float breath = (-cos(time * 3.0 + ((hexIndex.x + 17.0) * (hexIndex.y - 19.0))) + 1.0) * 0.5;
    
    vec3 col;
    float hexOn = step(0.45 * breath, HexDist(hexUV));
    uint hexColorIndex = uint((hexIndex.x * 17.0) * (hexIndex.y * 43.0)) % 3u;
    
    col.rgb = (1.0 - hexOn) * colors[hexColorIndex];
    col.rgb += hexOn * colors[3];

    // Output to screen
    fragColor = vec4(col,1.0);
}