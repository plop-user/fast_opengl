#version 330 core
layout (location=0) in vec3 apos;
layout (location = 1) in vec2 aoffset;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec4 locdata;
void main(){
	vec4 localPosition = model * vec4(apos, 1.0);
	vec3 newpos = vec3(localPosition) + vec3(aoffset.x, 0.5, aoffset.y);
	gl_Position = projection * view * vec4(newpos, 1.0);
	locdata = gl_Position;
}
