#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;
// Locations 3, 4, 5, 6 are used by the mat4 instanceMatrix
layout (location = 3) in mat4 instanceMatrix; 
// Location 7 is our new texture index for this specific instance
layout (location = 7) in float aTexIndex; 
layout (location = 8) in vec4 aInstanceColor;


out vec2 TexCoords;
out float TexIndex; // Pass to fragment shader
out vec4 FragInstanceColor;
uniform mat4 view;
uniform mat4 projection;

void main() {
    TexCoords = aTexCoords;
    TexIndex = aTexIndex;
FragInstanceColor = aInstanceColor;
    gl_Position = projection * view * instanceMatrix * vec4(aPos, 1.0);
}
