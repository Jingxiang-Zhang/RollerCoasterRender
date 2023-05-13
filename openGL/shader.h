#ifndef _LOAD_SHADER_H_
#define _LOAD_SHADER_H_
#include <GL/glew.h>

// load vertex and fragment shader
class Shader
{
public:
    Shader(const char* shaderBasePath);
    Shader(const char* filenameBasePath, const char* vertexShaderFilename, 
        const char* fragmentShaderFilename);
    ~Shader();
    int BuildShadersFromFiles(const char* filenameBasePath,
        const char* vertexShaderFilename, const char* fragmentShaderFilename);
    void Bind();
    GLuint GetProgramHandle() { return programHandle; }
    void SetModelViewMatrix(const float* m); // m is column-major
    void SetProjectionMatrix(const float* m); // m is column-major

private:
    // handle to the pipeline program
    GLuint programHandle; 
    // handle to the projection Matrix variable in the shader
    GLint h_projectionMatrix; 
    // handle to the modelView Matrix variable in the shader
    GLint h_modelViewMatrix;

    // load shader from a file into a string
    int LoadShader(const char* filename, char* code, int len);

    // compile a shader
    // input:
    //   code: the shader code
    //   shaderType: the type of shader: GL_VERTEX_SHADER, GL_FRAGMENT_SHADER
    // output:
    //   return value: 0=success, non-zero: failure
    int CompileShader(const char* code, GLenum shaderType, GLuint& shaderHandle); 
    
};

#endif

