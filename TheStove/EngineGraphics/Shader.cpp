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

 /**
  * @brief Builds a shader program from vertex and fragment shader files.
  * @param vertexFile Path to the vertex shader source file.
  * @param fragmentFile Path to the fragment shader source file.
  */
Shader::Shader(const std::string& vertexFile, const std::string& fragmentFile) {
	// Load both shader stages from disk before attempting compilation.
	std::string vertexSource = ReadFile(vertexFile);
	std::string fragmentSource = ReadFile(fragmentFile);

	// Compile each stage independently so errors can be reported with stage context.
	GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
	GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

	// Link the compiled stages into one executable GPU program.
	programID = LinkProgram(vertexShader, fragmentShader);

	// Release the intermediate shader objects once linking succeeds or fails.
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Cache uniform locations that are queried frequently during rendering.
	InitUniforms();
}

/**
 * @brief Releases the OpenGL shader program.
 */
Shader::~Shader() {
	// Return the linked program object to OpenGL when this wrapper is destroyed.
	glDeleteProgram(programID);
}

/**
 * @brief Reads an entire text file into memory.
 * @param filepath Path of the file to load.
 * @return File contents as a string, or an empty string on failure.
 */
std::string Shader::ReadFile(const std::string& filepath) {
	// Open the shader source file as text so the GLSL compiler can consume it directly.
	std::ifstream file(filepath);
	if (!file.is_open()) {
		TS_LOG_ERROR("[Shader] Failed to open shader file: " << filepath);
		TS_LOG_ERROR("[Shader] Current working directory: " << std::filesystem::current_path());
		return "";
	}

	// Slurp the full file into one string buffer for the OpenGL API.
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content = buffer.str();

	if (content.empty()) {
		// Warn when the file exists but does not contain usable GLSL source.
		TS_LOG_WARN("[Shader] Shader file is empty: " << filepath);
	}

	return content;
}

/**
 * @brief Compiles one GLSL shader stage.
 * @param type OpenGL shader-stage enum to compile.
 * @param source GLSL source code to compile.
 * @return OpenGL shader object ID.
 */
GLuint Shader::CompileShader(GLenum type, const std::string& source) {
	if (source.empty()) {
		// Log the failure context explicitly so empty-file mistakes are easy to trace.
		TS_LOG_ERROR("[Shader] Attempting to compile empty shader source.");
		TS_LOG_ERROR("[Shader] Shader type: " << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT"));
	}

	// Create the GL shader object and hand over the source text for compilation.
	GLuint shader = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		// Surface the compiler error log so shader authoring mistakes are visible at runtime.
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		TS_LOG_ERROR("[Shader] Error compiling shader (" << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT") << "): " << infoLog);
	}
	return shader;
}

/**
 * @brief Links compiled shader stages into a complete program.
 * @param vertexShader Compiled vertex shader object ID.
 * @param fragmentShader Compiled fragment shader object ID.
 * @return Linked OpenGL program object ID.
 */
GLuint Shader::LinkProgram(GLuint vertexShader, GLuint fragmentShader) {
	// Attach both compiled stages to a fresh program object before linking.
	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		// Surface the linker log when stage interfaces or uniforms do not match up.
		char infoLog[512];
		glGetProgramInfoLog(program, 512, nullptr, infoLog);
		TS_LOG_ERROR("[Shader] Error linking program: " << infoLog);
	}
	return program;
}

/**
 * @brief Caches the core uniform locations from the linked program.
 */
void Shader::InitUniforms() {
	// Cache the common transform uniforms to avoid repeated lookup cost during rendering.
	uniformModelMatrix = glGetUniformLocation(programID, "u_Model");
	uniformViewMatrix = glGetUniformLocation(programID, "u_View");
	uniformProjMatrix = glGetUniformLocation(programID, "u_Projection");

	if (uniformModelMatrix == -1 || uniformViewMatrix == -1 || uniformProjMatrix == -1) {
		// Instanced or specialized shaders may legitimately omit one or more of these uniforms.
		TS_LOG_WARN("[Shader] One or more uniform locations are invalid (ignore if using instanced shader).");
	}
}

/**
 * @brief Binds this shader program for subsequent draw calls.
 */
void Shader::Use() const {
	// Make this program the active one for future draw-state updates.
	glUseProgram(programID);

	// Surface GL program-binding failures through the shared logger.
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		TS_LOG_ERROR("[Shader] glUseProgram error: " << error);
	}
}

/**
 * @brief Updates the model-matrix uniform.
 * @param mat Model transform matrix to upload.
 */
void Shader::SetModelMatrix(const glm::mat4& mat) const {
	// Upload the caller's model transform into the cached uniform slot.
	glUniformMatrix4fv(uniformModelMatrix, 1, GL_FALSE, glm::value_ptr(mat));
}

/**
 * @brief Updates the view-matrix uniform.
 * @param mat View transform matrix to upload.
 */
void Shader::SetViewMatrix(const glm::mat4& mat) const {
	// Upload the caller's camera view matrix into the cached uniform slot.
	glUniformMatrix4fv(uniformViewMatrix, 1, GL_FALSE, glm::value_ptr(mat));
}

/**
 * @brief Updates the projection-matrix uniform.
 * @param mat Projection matrix to upload.
 */
void Shader::SetProjectionMatrix(const glm::mat4& mat) const {
	// Upload the caller's projection matrix into the cached uniform slot.
	glUniformMatrix4fv(uniformProjMatrix, 1, GL_FALSE, glm::value_ptr(mat));
}

/**
 * @brief Binds a named sampler uniform to a texture unit index.
 * @param name Sampler uniform name to update.
 * @param textureUnit Texture unit index that the sampler should read from.
 */
void Shader::SetTexture(const std::string& name, int textureUnit) const {
	// Resolve the sampler by name so different shaders can share this helper.
	GLint location = glGetUniformLocation(programID, name.c_str());
	if (location != -1) {
		// Point the sampler uniform at the requested bound texture unit.
		glUniform1i(location, textureUnit);
	}
}

/**
 * @brief Updates the color-tint uniform used by textured rendering paths.
 * @param tint RGBA tint multiplier to upload.
 */
void Shader::SetColorTint(const glm::vec4& tint) const {
	// Update the preferred tint uniform used by current textured sprite shaders.
	const GLint loc = glGetUniformLocation(programID, "u_ColorTint");
	if (loc != -1) {
		glUniform4fv(loc, 1, &tint[0]);
	}

	// Also support the legacy uniform name used by older shader variants.
	GLint locColor = glGetUniformLocation(programID, "u_Color");
	if (locColor != -1) {
		glUniform4fv(locColor, 1, glm::value_ptr(tint));
	}
}

/**
 * @brief Updates the UV offset uniform for atlas or animation sampling.
 * @param offset UV offset to upload.
 */
void Shader::SetUVOffset(const glm::vec2& offset) const {
	// Shift the sampled atlas window without changing the shader program itself.
	GLint loc = glGetUniformLocation(programID, "u_UVOffset");
	if (loc != -1) {
		glUniform2fv(loc, 1, &offset[0]);
	}
}

/**
 * @brief Updates the UV scale uniform for atlas or animation sampling.
 * @param scale UV scale to upload.
 */
void Shader::SetUVScale(const glm::vec2& scale) const {
	// Resize the sampled atlas window to match the active frame region.
	GLint loc = glGetUniformLocation(programID, "u_UVScale");
	if (loc != -1) {
		glUniform2fv(loc, 1, &scale[0]);
	}
}
