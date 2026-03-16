#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoord;

// Instance matrix (mat4 takes 4 attribute slots)
layout (location = 3) in vec4 iMat0;
layout (location = 4) in vec4 iMat1;
layout (location = 5) in vec4 iMat2;
layout (location = 6) in vec4 iMat3;

// Per-instance texture layer
layout (location = 7) in float texIndex;

uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;
flat out int TexIndex;

void main()
{
    mat4 model = mat4(iMat0, iMat1, iMat2, iMat3);

    gl_Position = projection * view * model * vec4(aPos, 1.0);

    TexCoord = aTexCoord;
    TexIndex = int(texIndex);
}
