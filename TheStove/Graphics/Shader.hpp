/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Shader.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		GLSL program wrapper: compile/link from files, bind, and set uniforms.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
	/** @brief Build a shader program from vertex and fragment shader files. */
	Shader(const std::string& vertexFile, const std::string& fragmentFile);

	/** @brief Release the OpenGL shader program. */
	~Shader();

	/** @brief Bind this shader program for subsequent draw calls. */
	void Use() const;

	/** @brief Set model matrix uniform. */
	void SetModelMatrix(const glm::mat4& mat) const;

	/** @brief Set view matrix uniform. */
	void SetViewMatrix(const glm::mat4& mat) const;

	/** @brief Set projection matrix uniform. */
	void SetProjectionMatrix(const glm::mat4& mat) const;

	/** @brief Bind a sampler uniform name to a texture unit index. */
	void SetTexture(const std::string& name, int textureUnit) const;

	/** @brief Set RGBA tint multiplier for textured rendering. */
	void SetColorTint(const glm::vec4& color) const; // For tinting textures

	/** @brief Set UV offset for atlas/animation sampling. */
	void SetUVOffset(const glm::vec2& offset) const;

	/** @brief Set UV scale for atlas/animation sampling. */
	void SetUVScale(const glm::vec2& scale) const;

	/** @brief Access the raw OpenGL program ID. */
	GLuint GetProgramID() const {
		return programID;
	}

private:
	GLuint programID;

	// Cached uniform locations for performance
	GLint uniformModelMatrix;
	GLint uniformViewMatrix;
	GLint uniformProjMatrix;

	/** @brief Cache required uniform locations from the linked program. */
	void InitUniforms();

	/** @brief Read an entire text file into memory. */
	std::string ReadFile(const std::string& filepath);

	/** @brief Compile one GLSL shader stage and return its object ID. */
	GLuint CompileShader(GLenum type, const std::string& source);

	/** @brief Link compiled shader stages into a complete program. */
	GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);
};

