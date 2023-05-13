#include "shader.h"
#include <cstring>
#include <cstdio>
#include <iostream>

using namespace std;

Shader::Shader(const char* shaderBasePath) {
    if (BuildShadersFromFiles(shaderBasePath, "basic.vertexShader.glsl", 
        "basic.fragmentShader.glsl") != 0) {
        cout << "Failed to build the pipeline program." << endl;
        exit(1);
    }
}

Shader::Shader(const char* shaderBasePath, const char* vertexShaderFilename,
    const char* fragmentShaderFilename) {
    if (BuildShadersFromFiles(shaderBasePath, vertexShaderFilename,
        fragmentShaderFilename) != 0) {
        cout << "Failed to build the pipeline program." << endl;
        exit(1);
    }
}

Shader::~Shader() {
    glDeleteProgram(programHandle);
}

int Shader::BuildShadersFromFiles(const char* filenameBasePath,
    const char* vertexShaderFilename,
    const char* fragmentShaderFilename) {

    // load vertex shader
    char vertex_shader_code[128 * 1024];
    char vertex_shader_filepath[1024] = { 0 };
    strcpy(vertex_shader_filepath, filenameBasePath);
    strcat(vertex_shader_filepath, "/");
    strcat(vertex_shader_filepath, vertexShaderFilename);
    // load vertex shader
    if (LoadShader(vertex_shader_filepath, vertex_shader_code, 128 * 1024) != 0) {
        cout << "shader " << vertex_shader_filepath << " file not found." << endl;
        return 1;
    }

    // load fragment shader
    char fragment_shader_code[128 * 1024];
    char fragment_shader_filepath[1024] = { 0 };
    strcpy(fragment_shader_filepath, filenameBasePath);
    strcat(fragment_shader_filepath, "/");
    strcat(fragment_shader_filepath, fragmentShaderFilename);
    // load vertex shader
    if (LoadShader(fragment_shader_filepath, fragment_shader_code, 128 * 1024) != 0) {
        cout << "shader " << fragment_shader_filepath << " file not found." << endl;
        return 1;
    }
   
    // create a program handle
    programHandle = glCreateProgram();
    if (programHandle == 0) {
        cout << "shader initialize failed" << endl;
        return -1;
    }

    // compile vertex shader
    GLuint h_vertex_shaders;
    if (CompileShader(vertex_shader_code, GL_VERTEX_SHADER, h_vertex_shaders) != 0) {
        cout << "vertex shader compile error" << endl;
        return 1;
    }
    else
        glAttachShader(programHandle, h_vertex_shaders);

    // compile fragment shader
    GLuint h_fragment_shaders;
    if (CompileShader(fragment_shader_code, GL_FRAGMENT_SHADER, h_fragment_shaders) != 0) {
        cout << "fragment shader compile error" << endl;
        return 1;
    }
    else
        glAttachShader(programHandle, h_fragment_shaders);

    // link the program
    glLinkProgram(programHandle);
    // get program error if possible
    int status;
    glGetProgramiv(programHandle, GL_LINK_STATUS, &status);
    if (status == 0) {
        GLchar infoLog[512];
        glGetProgramInfoLog(programHandle, 512, nullptr, infoLog);
        cout << "Errors:\n" << infoLog << endl;
        return 1;
    }
    // delete shaders
    glDeleteShader(h_vertex_shaders);
    glDeleteShader(h_fragment_shaders);

    // get model view matrix handle and projection matrix handle
    h_modelViewMatrix = glGetUniformLocation(programHandle, "modelViewMatrix");
    if (h_modelViewMatrix == -1)
        cout << "model view Matrix not found \n" << endl;
    h_projectionMatrix = glGetUniformLocation(programHandle, "projectionMatrix");
    if (h_modelViewMatrix == -1)
        cout << "projection Matrix not found \n" << endl;
    
    return 0;
}

void Shader::Bind() {
    glUseProgram(programHandle);
}

int Shader::LoadShader(const char* filename, char* code, int len) {
    FILE* file = fopen(filename, "rb");
    if (file == nullptr) {
        return 1;
    }
    size_t length = fread(code, 1, len, file);
    code[length] = '\0';
    fclose(file);
    return 0;
}

int Shader::CompileShader(const char* shaderCode, GLenum shaderType, GLuint& shaderHandle) {
    shaderHandle = glCreateShader(shaderType);
    if (shaderHandle == 0) {
        cout << "shader creation failed" << endl;
        return 1;
    }
    const int numShaderCodes = 1;
    const GLchar* shaderCodes[] = { shaderCode };
    GLint codeLength[] = { (GLint)strlen(shaderCode) };

    // compile the shader (the entire source is in one string)
    glShaderSource(shaderHandle, numShaderCodes, shaderCodes, codeLength);
    glCompileShader(shaderHandle);
    // check if compilation was successful
    GLint status;
    glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shaderHandle, 512, nullptr, infoLog);
        cout << infoLog << endl;
        return 1;
    }
    return 0;
}

void Shader::SetModelViewMatrix(const float* m)
{
    glUniformMatrix4fv(h_modelViewMatrix, 1, GL_FALSE, m);
}

void Shader::SetProjectionMatrix(const float* m)
{
    glUniformMatrix4fv(h_projectionMatrix, 1, GL_FALSE, m);
}

