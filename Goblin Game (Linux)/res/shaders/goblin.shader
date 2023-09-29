#shader vertex
#version 330 core

layout (location = 0) in vec3 aPos;

out vec3 pos;
out vec2 uv;

uniform mat4 screenspace, camera, transform;

void main() {
	gl_Position = screenspace * camera * transform * vec4(aPos, 1);
	pos = aPos;
	uv = pos.xy * 0.5 + 0.5;
}

#shader fragment
#version 330 core

in vec3 pos;
in vec2 uv;

uniform vec4 col;
uniform sampler2D sheet;
uniform sampler2D coords;
uniform sampler2D color_mask;
uniform float index;

out vec4 glColor;
void main() {
	vec2 texcoords = floor(texture2D(coords, vec2(index, 0)).xy * 32.0) / 32.0 + uv * vec2(32.0 / 256.0);
	glColor = texture2D(sheet, texcoords) * mix(vec4(1), col, texture2D(color_mask, texcoords).x);
}