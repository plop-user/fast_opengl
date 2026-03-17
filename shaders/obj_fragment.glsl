#version 430 core

in vec2 TexCoord;
flat in int TexIndex;

uniform sampler2DArray textureArray;

out vec4 FragColor;

void main()
{
//FragColor = vec4(TexCoord, 0.0, 1.0);
	FragColor = texture(textureArray, vec3(TexCoord, TexIndex));
}
