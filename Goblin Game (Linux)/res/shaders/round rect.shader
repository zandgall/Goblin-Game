#shader vertex
#version 330 core

layout(location = 0) in vec3 aPos;

out vec3 pos;
out vec2 uv;
out vec2 transform_size;

uniform mat4 screenspace, transform;

void main() {
	gl_Position = screenspace * transform * vec4(aPos, 1);
	pos = aPos;
	uv = pos.xy * 0.5 + 0.5;
	transform_size.x = transform[0].x;
	transform_size.y = transform[1].y;
}

#shader fragment
#version 330 core

in vec3 pos;
in vec2 uv;
in vec2 transform_size;

uniform vec4 col;
uniform sampler2D tex;
uniform bool useTex;
uniform float rounding;
out vec4 glColor;
float roundrect(vec2 val, vec2 size, float round) {
	//float d = length(max(abs(size*val)+round, size)) - round;
	float d = length(vec2(max(abs(size.x * val.x) + round, size.x) - size.x, max(abs(size.y * val.y) + round, size.y) - size.y)) - round;
	return max(min(1-d, 1), 0);
}
void main() {
	float alpha = roundrect(uv * 2 - 1, transform_size, rounding);
	if (useTex)
		glColor = col * texture2D(tex, uv);
	else
		glColor = col;
	glColor.w *= alpha;
}