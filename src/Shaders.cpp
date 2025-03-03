#include <Shaders.hpp>

std::string readShaderSource(const std::string& filepath) 
{
    std::ifstream file;
    file.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    try{
        file.open(filepath);
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        return buffer.str();
    }    
    catch(std::ifstream::failure e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        return nullptr;
    }
}

GLuint compileShader(GLenum type, const std::string& source) 
{
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();

    // attach shader source code to the shader object
    glShaderSource(shader, 1, &src, nullptr);
    // compile the shader
    glCompileShader(shader);

    // Check for compilation errors
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation error:\n" << infoLog << std::endl;
    }

    return shader;
}

// Create a shader program from vertex and fragment shaders
GLuint createShaderProgram(const std::string& vertexSource, const std::string& fragmentSource) 
{
    GLuint vertexShader   = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    // Link both shaders into a shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking error:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void setBool(unsigned int ID, const std::string &name, bool value) 
{         
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value); 
}

void setInt(unsigned int ID, const std::string &name, int value) 
{ 
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value); 
}

void setFloat(unsigned int ID, const std::string &name, float value)
{ 
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value); 
}

void setFloat2(unsigned int ID, const std::string &name, float value1, float value2)
{ 
    glUniform2f(glGetUniformLocation(ID, name.c_str()), value1, value2); 
}

void setVec3(unsigned int ID, const std::string& name, const glm::vec3& vector) 
{
    unsigned int loc = glGetUniformLocation(ID, name.c_str());
    glUniform3fv(loc, 1, glm::value_ptr(vector));
}

void setMat4(unsigned int ID, const std::string& name, const glm::mat4& matrix) 
{
    unsigned int loc = glGetUniformLocation(ID, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));
}
