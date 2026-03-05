#version 330 core
in vec2 TexCoords;
in float TexIndex;
in vec4 FragInstanceColor;
out vec4 FragColor;

// Note: sampler2DArray instead of sampler2D
uniform sampler2DArray textureArray; 
void main() {
    if (TexIndex < 0.0) {
        // Mode 1: Solid Color (No Texture)
        FragColor = FragInstanceColor;
    } else {
        // Mode 2: Textured
        // We multiply by FragInstanceColor. 
        // If the color is White (1,1,1,1), the texture looks normal.
        // If the color is Red (1,0,0,1), the texture gets tinted red!
        vec4 texColor = texture(textureArray, vec3(TexCoords, TexIndex));
        FragColor = texColor * FragInstanceColor;
    }
}
