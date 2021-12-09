#version 450

const vec3 positions[] = vec3[6](
	vec3(-1.f, -1.f, 1.f),
	vec3(1.f, 1.f, 1.f),
	vec3(-1.f, 1.f, 1.f),
	vec3(-1.f, -1.f, 1.f),
	vec3(1.f, -1.f, 1.f),
	vec3(1.f, 1.f, 1.f)
);

void main()
{
	gl_Position = vec4(positions[gl_VertexIndex], 1.0f);
}
