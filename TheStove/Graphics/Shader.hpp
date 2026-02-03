/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Shader.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu

 DESCRIPTION:		GLSL program wrapper: compile/link from files, bind, and set uniforms.

		 All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
	Shader(const std::string& vertexFile, const std::string& fragmentFile);
	~Shader();

	void Use() const;

	// Set uniforms by cached location
	void SetModelMatrix(const glm::mat4& mat) const;
	void SetViewMatrix(const glm::mat4& mat) const;
	void SetProjectionMatrix(const glm::mat4& mat) const;
	void SetTexture(const std::string& name, int textureUnit) const;
	void SetColorTint(const glm::vec4& color) const;  // For tinting textures

	void SetUVOffset(const glm::vec2& offset) const;
	void SetUVScale(const glm::vec2& scale) const;

	GLuint GetProgramID() const { return programID; }

private:
	GLuint programID;

	// Cached uniform locations for performance
	GLint uniformModelMatrix;
	GLint uniformViewMatrix;
	GLint uniformProjMatrix;

	void InitUniforms();

	std::string ReadFile(const std::string& filepath);
	GLuint CompileShader(GLenum type, const std::string& source);
	GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);
};
