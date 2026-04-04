/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Shader.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (75%)
 CO-AUTHORS:		Ng Juin Herng, juinherng.ng@digipen.edu (15%)
					Yat Chun Wee, y.chunwee@digipen.edu		(10%)

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
	/**
	 * @brief Builds a shader program from vertex and fragment shader files.
	 * @param vertexFile Path to the vertex shader source file.
	 * @param fragmentFile Path to the fragment shader source file.
	 */
	Shader(const std::string& vertexFile, const std::string& fragmentFile);

	/**
	 * @brief Releases the OpenGL shader program.
	 */
	~Shader();

	/**
	 * @brief Binds this shader program for subsequent draw calls.
	 */
	void Use() const;

	/**
	 * @brief Updates the model-matrix uniform.
	 * @param mat Model transform matrix to upload.
	 */
	void SetModelMatrix(const glm::mat4& mat) const;

	/**
	 * @brief Updates the view-matrix uniform.
	 * @param mat View transform matrix to upload.
	 */
	void SetViewMatrix(const glm::mat4& mat) const;

	/**
	 * @brief Updates the projection-matrix uniform.
	 * @param mat Projection matrix to upload.
	 */
	void SetProjectionMatrix(const glm::mat4& mat) const;

	/**
	 * @brief Binds a named sampler uniform to a texture unit index.
	 * @param name Sampler uniform name to update.
	 * @param textureUnit Texture unit index that the sampler should read from.
	 */
	void SetTexture(const std::string& name, int textureUnit) const;

	/**
	 * @brief Updates the color-tint uniform used by textured rendering paths.
	 * @param color RGBA tint multiplier to upload.
	 */
	void SetColorTint(const glm::vec4& color) const; // For tinting textures

	/**
	 * @brief Updates the UV offset uniform for atlas or animation sampling.
	 * @param offset UV offset to upload.
	 */
	void SetUVOffset(const glm::vec2& offset) const;

	/**
	 * @brief Updates the UV scale uniform for atlas or animation sampling.
	 * @param scale UV scale to upload.
	 */
	void SetUVScale(const glm::vec2& scale) const;

	/**
	 * @brief Returns the raw OpenGL program identifier.
	 * @return Linked OpenGL program ID owned by this wrapper.
	 */
	GLuint GetProgramID() const {
		// Expose the linked program handle for low-level integrations.
		return programID;
	}

private:
	GLuint programID;

	// Cached uniform locations for performance
	GLint uniformModelMatrix;
	GLint uniformViewMatrix;
	GLint uniformProjMatrix;

	/**
	 * @brief Caches the core uniform locations from the linked program.
	 */
	void InitUniforms();

	/**
	 * @brief Reads an entire text file into memory.
	 * @param filepath Path of the file to load.
	 * @return File contents as a string, or an empty string on failure.
	 */
	std::string ReadFile(const std::string& filepath);

	/**
	 * @brief Compiles one GLSL shader stage.
	 * @param type OpenGL shader-stage enum to compile.
	 * @param source GLSL source code to compile.
	 * @return OpenGL shader object ID.
	 */
	GLuint CompileShader(GLenum type, const std::string& source);

	/**
	 * @brief Links compiled shader stages into a complete program.
	 * @param vertexShader Compiled vertex shader object ID.
	 * @param fragmentShader Compiled fragment shader object ID.
	 * @return Linked OpenGL program object ID.
	 */
	GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);
};
