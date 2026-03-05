#version 330 core

in vec4 sdata;
out vec4 fragcolor;

void main(){
	fragcolor = sdata+vec4(0.1f, 0.5f, 0.4f, 0.0f);
}
