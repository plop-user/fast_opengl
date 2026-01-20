#version 330 core
layout (location=0) in vec3 apos;
layout (location = 1) in vec2 aoffset;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec4 locdata;
void main(){
	vec3 newpos = apos + vec3(aoffset, 1.0);
	gl_Position = projection * view * model * vec4(newpos, 1.0);
	locdata = gl_Position;
}
