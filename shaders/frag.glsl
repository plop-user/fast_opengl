#version 330 core

in vec2 texcoord;
uniform sampler2D cubetex; 
out vec4 FragColor;
    void main()
    {
   //  FragColor = vec4(texcoord, 0.0, 1.0);
	 FragColor = texture(cubetex, texcoord);
}
