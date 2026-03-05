#version 330 core

in vec4 FragInstanceColor;
out vec4 FragColor;

uniform sampler2DArray textureArray;

void main(){
		FragColor = FragInstanceColor;
}
