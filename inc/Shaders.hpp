#pragma once

#include <GLAD/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

std::string readShaderSource(const std::string& filepath);
GLuint compileShader(GLenum type, const std::string& source); 
GLuint createShaderProgram(const std::string& vertexSource, const std::string& fragmentSource);
void setBool(unsigned int ID, const std::string &name, bool value) ;
void setInt(unsigned int ID, const std::string &name, int value) ;
void setFloat(unsigned int ID, const std::string &name, float value);
void setFloat2(unsigned int ID, const std::string &name, float value1, float value2);
void setVec3(unsigned int ID, const std::string& name, const glm::vec3& vector);
void setMat4(unsigned int ID, const std::string& name, const glm::mat4& matrix);
