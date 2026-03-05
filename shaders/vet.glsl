#version 330 core
layout (location=0) in vec3 apos;
layout (location = 1) in vec2 aoffset;
layout (location = 2) in vec2 intexcords;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec2 texcoord;
void main(){
	vec4 localPosition = model * vec4(apos, 1.0);
	vec3 newpos = vec3(localPosition) + vec3(aoffset.x, 0.5, aoffset.y);
	gl_Position = projection * view * vec4(newpos, 1.0);
	texcoord = intexcords;
}
