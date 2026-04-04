/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Shader.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (75%)
 CO-AUTHORS:		Ng Juin Herng, juinherng.ng@digipen.edu (15%)
					Yat Chun Wee, y.chunwee@digipen.edu		(10%)

 DESCRIPTION:		Implements shader file loading, compilation, linking, use(), and uniform helpers.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <filesystem>
#include <fstream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>

#include "EngineCore/Logger.hpp"
#include "EngineGraphics/Shader.hpp"

Shader::Shader(const std::string& vertexFile, const std::string& fragmentFile) {
	std::string vertexSource = ReadFile(vertexFile);
	std::string fragmentSource = ReadFile(fragmentFile);

	GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
	GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

	programID = LinkProgram(vertexShader, fragmentShader);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	InitUniforms();
}

Shader::~Shader() {
	glDeleteProgram(programID);
}

std::string Shader::ReadFile(const std::string& filepath) {
	std::ifstream file(filepath);
	if (!file.is_open()) {
		TS_LOG_ERROR("[Shader] Failed to open shader file: " << filepath);
		TS_LOG_ERROR("[Shader] Current working directory: " << std::filesystem::current_path());
		return "";
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content = buffer.str();

	if (content.empty()) {
		TS_LOG_WARN("[Shader] Shader file is empty: " << filepath);
	}

	return content;
}

GLuint Shader::CompileShader(GLenum type, const std::string& source) {
	if (source.empty()) {
		TS_LOG_ERROR("[Shader] Attempting to compile empty shader source.");
		TS_LOG_ERROR("[Shader] Shader type: " << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT"));
	}

	GLuint shader = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		TS_LOG_ERROR("[Shader] Error compiling shader (" << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT") << "): " << infoLog);
	}
	return shader;
}

GLuint Shader::LinkProgram(GLuint vertexShader, GLuint fragmentShader) {
	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetProgramInfoLog(program, 512, nullptr, infoLog);
		TS_LOG_ERROR("[Shader] Error linking program: " << infoLog);
	}
	return program;
}

void Shader::InitUniforms() {

	// Cache uniform locations
	uniformModelMatrix = glGetUniformLocation(programID, "u_Model");
	uniformViewMatrix = glGetUniformLocation(programID, "u_View");
	uniformProjMatrix = glGetUniformLocation(programID, "u_Projection");

	if (uniformModelMatrix == -1 || uniformViewMatrix == -1 || uniformProjMatrix == -1) {
		TS_LOG_WARN("[Shader] One or more uniform locations are invalid (ignore if using instanced shader).");
	}

}

void Shader::Use() const {
	glUseProgram(programID);

	// Surface GL program-binding failures through the shared logger.
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		TS_LOG_ERROR("[Shader] glUseProgram error: " << error);
	}
}

void Shader::SetModelMatrix(const glm::mat4& mat) const {
	glUniformMatrix4fv(uniformModelMatrix, 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::SetViewMatrix(const glm::mat4& mat) const {
	glUniformMatrix4fv(uniformViewMatrix, 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::SetProjectionMatrix(const glm::mat4& mat) const {
	glUniformMatrix4fv(uniformProjMatrix, 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::SetTexture(const std::string& name, int textureUnit) const {
	GLint location = glGetUniformLocation(programID, name.c_str());
	if (location != -1) {
		glUniform1i(location, textureUnit);
	}
}

void Shader::SetColorTint(const glm::vec4& tint) const {
	const GLint loc = glGetUniformLocation(programID, "u_ColorTint");
	if (loc != -1) {
		glUniform4fv(loc, 1, &tint[0]);
	}

	GLint locColor = glGetUniformLocation(programID, "u_Color");
	if (locColor != -1) {
		glUniform4fv(locColor, 1, glm::value_ptr(tint));
	}
}

void Shader::SetUVOffset(const glm::vec2& offset) const {
	GLint loc = glGetUniformLocation(programID, "u_UVOffset");
	if (loc != -1) {
		glUniform2fv(loc, 1, &offset[0]);
	}
}

void Shader::SetUVScale(const glm::vec2& scale) const {
	GLint loc = glGetUniformLocation(programID, "u_UVScale");
	if (loc != -1) {
		glUniform2fv(loc, 1, &scale[0]);
	}
}
