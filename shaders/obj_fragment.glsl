#version 430 core

in vec2 TexCoord;

uniform sampler2D diffuseTexture;
uniform bool useTexture;
uniform vec4 baseColor;
uniform vec4 instanceTint;

out vec4 FragColor;

void main()
{
    vec4 color = baseColor;
    if (useTexture) {
        color *= texture(diffuseTexture, TexCoord);
    }
    FragColor = color * instanceTint;
}
