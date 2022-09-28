#shader vertex
#version 460 core

layout (location = 0) in vec3 aPos;

varying vec3 pos;
varying vec2 uv;

uniform mat4 screenspace, canvas_space, view_space, transform;

void main() {
	gl_Position = screenspace * canvas_space * view_space * transform * vec4(aPos, 1);
	pos = aPos;
	uv = pos.xy * 0.5 + 0.5;
}

#shader fragment
#version 460 core

varying vec3 pos;
varying vec2 uv;

uniform vec4 col;
uniform sampler2D tex;
uniform bool useTex;
out vec4 glColor;
void main() {
	if(useTex)
		glColor = col * texture2D(tex, uv);
	else
		glColor = col;
}