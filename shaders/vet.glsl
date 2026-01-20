#version 330 core
layout (location=0) in vec3 apos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec4 locdata;
void main(){
	gl_Position = projection * view * model * vec4(apos, 1.0);
	locdata = gl_Position;
}
