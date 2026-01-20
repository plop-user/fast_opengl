#version 330 core

in vec4 locdata;
out vec4 FragColor;
    void main()
    {
       FragColor = vec4(locdata) * vec4(0.99f, 0.86f, 0.73f, 1.0f);
    }
