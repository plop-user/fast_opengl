#version 330 core
// for grid in xz plane
layout (location = 0) in vec3 gridpos;

uniform mat4 view;
uniform mat4 perspective;

out vec4 gridfinal;

void main(){

	gl_Position = perspective * view * vec4(gridpos, 1.0);
}
