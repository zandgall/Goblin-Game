#shader vertex
#version 330 core

layout (location = 0) in vec3 aPos;

out vec3 pos;
out vec2 uv;

uniform mat4 screenspace, transform;

void main() {
	gl_Position = screenspace * transform * vec4(aPos, 1);
	pos = aPos;
	uv = pos.xy * 0.5 + 0.5;
}

#shader fragment
#version 330 core

in vec3 pos;
in vec2 uv;

float blur9(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) {
	float color = 0.0;
	vec2 off1 = vec2(1.3846153846) * direction;
	vec2 off2 = vec2(3.2307692308) * direction;
	color += texture2D(image, uv).w * 0.2270270270;
	color += texture2D(image, uv + (off1 / resolution)).w * 0.3162162162;
	color += texture2D(image, uv - (off1 / resolution)).w * 0.3162162162;
	color += texture2D(image, uv + (off2 / resolution)).w * 0.0702702703;
	color += texture2D(image, uv - (off2 / resolution)).w * 0.0702702703;
	return color;
}

uniform vec4 col;
uniform sampler2D tex;
out vec4 glColor;
void main() {
	ivec2 resolution = textureSize(tex, 0);
	float color = blur9(tex, uv, resolution, vec2(1, 0));
	color += blur9(tex, uv, resolution, vec2(0, 1));
	glColor = col * vec4(1, 1, 1, color*0.75);
}