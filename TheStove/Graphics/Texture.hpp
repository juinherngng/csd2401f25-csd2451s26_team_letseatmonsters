/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Texture.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		2D texture resource wrapper providing load, bind to texture unit, and sampler params.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <glad/glad.h>
#include <string>

class Texture {
public:
	/** @brief Construct an empty OpenGL texture wrapper. */
	Texture();

	/** @brief Destroy owned OpenGL texture handle, if any. */
	~Texture();

	// Delete copy constructor and assignment (move-only like your other classes)
	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

	// Allow move operations
	/** @brief Move-construct texture ownership from another wrapper. */
	Texture(Texture&& other) noexcept;

	/** @brief Move-assign texture ownership from another wrapper. */
	Texture& operator=(Texture&& other) noexcept;

	/** @brief Load texture data from disk and upload it to OpenGL. */
	bool LoadFromFile(const std::string& filePath);

	/** @brief Bind this texture to a texture unit slot. */
	void Bind(unsigned int slot = 0) const;

	/** @brief Unbind any texture from the active texture target. */
	void Unbind() const;

	/** @brief Get loaded texture width in pixels. */
	int GetWidth() const {
		return width;
	}

	/** @brief Get loaded texture height in pixels. */
	int GetHeight() const {
		return height;
	}

	/** @brief Get the underlying OpenGL texture object ID. */
	GLuint GetID() const {
		return textureID;
	}

private:
	GLuint textureID;
	int width, height, channels;
};
