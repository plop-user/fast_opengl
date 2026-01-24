#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <iostream>
#include <string>
#include <glslread.h>


static int gridview;
static int gridpers;
static unsigned int VAO;
static std::vector<float> lines; 
static unsigned int gridprogram;



void creategrid(){

std::string myvertext = readFile("shaders/gridvex.glsl");
const char* vertexfinal = myvertext.c_str();
unsigned int testvertex = glad_glCreateShader(GL_VERTEX_SHADER);
glShaderSource(testvertex, 1, &vertexfinal, NULL);
glCompileShader(testvertex);
int success;
char infoLog[512];
glGetShaderiv(testvertex, GL_COMPILE_STATUS, &success);
if (!success)
{ 
	glGetShaderInfoLog(testvertex, 512, NULL, infoLog);
	std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	} 


	// doing frag shader
std::string fragtext = readFile("shaders/gridfrag.glsl");
const char* fragtextc = fragtext.c_str();
unsigned int fragshader = glad_glCreateShader(GL_FRAGMENT_SHADER);
glShaderSource(fragshader, 1, &fragtextc, NULL);
glCompileShader(fragshader);
glGetShaderiv(fragshader, GL_COMPILE_STATUS, &success);
if (!success)
    {
	glGetShaderInfoLog(fragshader, 512, NULL, infoLog);
	std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }




gridprogram = glCreateProgram();
glAttachShader(gridprogram, testvertex);
glAttachShader(gridprogram, fragshader);
glLinkProgram(gridprogram);








for(int x = -100; x<101; x=x+2){
	
	lines.push_back(-100);
	lines.push_back(0.0f);
	lines.push_back((float)x);


	lines.push_back(100);
	lines.push_back(0.0f);
	lines.push_back((float)x);
	
		


	}
for(int z = -100; z<101; z=z+2){
	
	lines.push_back((float)z);
	lines.push_back(0.0f);
	lines.push_back(-100.0f);


	lines.push_back((float)z);
	lines.push_back(0.0f);
	lines.push_back(100.0f);

	}

unsigned int ll;
glGenVertexArrays(1, &VAO);
glGenBuffers(1, &ll);
glBindVertexArray(VAO);
glBindBuffer(GL_ARRAY_BUFFER, ll);

glBufferData(GL_ARRAY_BUFFER, lines.size()*sizeof(float), &lines[0], GL_STATIC_DRAW);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);
glBindBuffer(GL_ARRAY_BUFFER, 0);
glBindVertexArray(0);
gridview = glGetUniformLocation(gridprogram, "view");
gridpers = glGetUniformLocation(gridprogram, "perspective");
}


void griddraw(glm::mat4 view, glm::mat4 pers){
	glUseProgram(gridprogram);
	glUniformMatrix4fv(gridview, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(gridpers, 1, GL_FALSE, glm::value_ptr(pers));
	glBindVertexArray(VAO);
		
	glDrawArrays(GL_LINES, 0, lines.size() / 3);
}
