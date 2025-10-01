/**
 * @file shader.cpp
 * @brief Helper functions to load, compile, and link GLSL shaders (vertex, fragment, geometry).
 */

#include <cstdio>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <cstring>

#include <glad/glad.h>
#include <shader.h>

#if USE_CSD3151_AUTOMATION == 1
 // This automation hook reads the shader from the submission tutorial's shader directory as a string literal.
 // It requires an automation script to convert the shader files from file format to string literal format.
 // After conversion, the file names must be changed to my-shader.vert and my-shader.frag.
std::string const assignment_vs = {
  #include "../shaders/my-shader.vert"
};
std::string const assignment_fs = {
  #include "../shaders/my-shader.frag"
};
#endif

//////////////////////////////////////////////////////////////////////////

/**
 * @brief Loads and compiles vertex and fragment shaders from files, then links them into a shader program.
 *
 * @param vertex_file_path Path to the vertex shader source file.
 * @param fragment_file_path Path to the fragment shader source file.
 * @return GLuint Program ID of the compiled and linked shader program.
 */

GLuint LoadShaders(const std::string& p_vertexShaderCode, const std::string& p_fragmentShaderCode, bool p_loadFromFile = false)
{
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    std::string vertexShaderCode;
    std::string fragmentShaderCode;

    if (p_loadFromFile) {
        std::ifstream vertexShaderStream(p_vertexShaderCode.c_str(), std::ios::in);
        if (vertexShaderStream.is_open()) {
            std::string line;
            while (std::getline(vertexShaderStream, line)) {
                vertexShaderCode += "\n" + line;
            }
            vertexShaderStream.close();
        }
        else {
            std::cerr << "Unable to open vertex shader file: " << p_vertexShaderCode << std::endl;
            return 0;
        }

        std::ifstream fragmentShaderStream(p_fragmentShaderCode.c_str(), std::ios::in);
        if (fragmentShaderStream.is_open()) {
            std::string line;
            while (std::getline(fragmentShaderStream, line)) {
                fragmentShaderCode += "\n" + line;
            }
            fragmentShaderStream.close();
        }
        else {
            std::cerr << "Unable to open fragment shader file: " << p_fragmentShaderCode << std::endl;
            return 0;
        }
    }
    else {
        vertexShaderCode = p_vertexShaderCode;
        fragmentShaderCode = p_fragmentShaderCode;
    }

    GLint result = GL_FALSE;
    int infoLogLength;

    // Compile Vertex Shader
    std::cout << "Compiling vertex shader" << std::endl;
    const char* vertexSourcePointer = vertexShaderCode.c_str();
    glShaderSource(vertexShaderId, 1, &vertexSourcePointer, nullptr);
    glCompileShader(vertexShaderId);
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &result);
    glGetShaderiv(vertexShaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
        std::vector<char> errorMsg(infoLogLength + 1);
        glGetShaderInfoLog(vertexShaderId, infoLogLength, nullptr, &errorMsg[0]);
        std::cerr << errorMsg.data() << std::endl;
    }

    // Compile Fragment Shader
    std::cout << "Compiling fragment shader" << std::endl;
    const char* fragmentSourcePointer = fragmentShaderCode.c_str();
    glShaderSource(fragmentShaderId, 1, &fragmentSourcePointer, nullptr);
    glCompileShader(fragmentShaderId);
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &result);
    glGetShaderiv(fragmentShaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
        std::vector<char> errorMsg(infoLogLength + 1);
        glGetShaderInfoLog(fragmentShaderId, infoLogLength, nullptr, &errorMsg[0]);
        std::cerr << errorMsg.data() << std::endl;
    }

    // Link Program
    std::cout << "Linking program" << std::endl;
    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);
    glLinkProgram(programId);
    glGetProgramiv(programId, GL_LINK_STATUS, &result);
    glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
        std::vector<char> programError(infoLogLength + 1);
        glGetProgramInfoLog(programId, infoLogLength, nullptr, &programError[0]);
        std::cerr << programError.data() << std::endl;
    }

    glDetachShader(programId, vertexShaderId);
    glDetachShader(programId, fragmentShaderId);
    glDeleteShader(vertexShaderId);
    glDeleteShader(fragmentShaderId);

    return programId;
}
//////////////////////////////////////////////////////////////////////////

/**
 * @brief Loads shaders and sets up a separable shader pipeline (useful for modern OpenGL).
 *
 * @param vertex_file_path Path to vertex shader.
 * @param fragment_file_path Path to fragment shader.
 * @param programIDs Output array to hold created program IDs (index 0: vertex, index 1: fragment).
 * @return GLuint ID of created program pipeline object.
 */
GLuint LoadPipeline(const char* vertex_file_path, const char* fragment_file_path, GLuint* programIDs)
{
    // --- Vertex Shader ---
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if (VertexShaderStream.is_open()) {
        std::string Line;
        while (getline(VertexShaderStream, Line))
            VertexShaderCode += "\n" + Line;
        VertexShaderStream.close();
    }
    else {
        printf("Unable to open %s\n", vertex_file_path);
        getchar();
        return 0;
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    printf("Compiling shader: %s\n", vertex_file_path);
#if USE_CSD3151_AUTOMATION == 1
    const char* VertexSourcePointer = assignment_vs.c_str();
#else
    const char* VertexSourcePointer = VertexShaderCode.c_str();
#endif

    glShaderSource(VertexShaderID, 1, &VertexSourcePointer, nullptr);
    glCompileShader(VertexShaderID);

    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> ErrorMsg(InfoLogLength + 1);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, nullptr, &ErrorMsg[0]);
        printf("%s\n", &ErrorMsg[0]);
    }

    programIDs[0] = glCreateProgram();
    glAttachShader(programIDs[0], VertexShaderID);
    glProgramParameteri(programIDs[0], GL_PROGRAM_SEPARABLE, GL_TRUE);
    glLinkProgram(programIDs[0]);

    // Check program link
    glGetProgramiv(programIDs[0], GL_LINK_STATUS, &Result);
    glGetProgramiv(programIDs[0], GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> ErrorMsg(InfoLogLength + 1);
        glGetProgramInfoLog(programIDs[0], InfoLogLength, nullptr, &ErrorMsg[0]);
        printf("%s\n", &ErrorMsg[0]);
    }

    // --- Fragment Shader ---
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if (FragmentShaderStream.is_open()) {
        std::string Line;
        while (getline(FragmentShaderStream, Line))
            FragmentShaderCode += "\n" + Line;
        FragmentShaderStream.close();
    }

    printf("Compiling shader: %s\n", fragment_file_path);
#if USE_CSD3151_AUTOMATION == 1
    const char* FragmentSourcePointer = assignment_fs.c_str();
#else
    const char* FragmentSourcePointer = FragmentShaderCode.c_str();
#endif
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, nullptr);
    glCompileShader(FragmentShaderID);

    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> ErrorMsg(InfoLogLength + 1);
        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, nullptr, &ErrorMsg[0]);
        printf("%s\n", &ErrorMsg[0]);
    }

    programIDs[1] = glCreateProgram();
    glAttachShader(programIDs[1], FragmentShaderID);
    glProgramParameteri(programIDs[1], GL_PROGRAM_SEPARABLE, GL_TRUE);
    glLinkProgram(programIDs[1]);

    GLuint pipeLineID;
    glGenProgramPipelines(1, &pipeLineID);

    glUseProgramStages(pipeLineID, GL_VERTEX_SHADER_BIT, programIDs[0]);
    glUseProgramStages(pipeLineID, GL_FRAGMENT_SHADER_BIT, programIDs[1]);

    return pipeLineID;
}