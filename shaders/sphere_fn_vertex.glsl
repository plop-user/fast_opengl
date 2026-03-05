#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in mat4 instanceMatrix;

layout (location = 7) in vec4 aInstanceColor;




out vec4 FragInstanceColor;

uniform mat4 view;
uniform mat4 projection;


void main(){
	FragInstanceColor = aInstanceColor;


	gl_Position = projection * view * instanceMatrix * vec4(aPos, 1.0);
}
