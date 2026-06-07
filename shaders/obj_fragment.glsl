#version 430 core

in vec2 TexCoord;
flat in int TexIndex;
in vec4 Tint;

uniform sampler2DArray textureArray;

out vec4 FragColor;

void main()
{
	if (TexIndex < 0) {
		FragColor = Tint;
	} else {
		FragColor = texture(textureArray, vec3(TexCoord, TexIndex)) * Tint;
	}
}
