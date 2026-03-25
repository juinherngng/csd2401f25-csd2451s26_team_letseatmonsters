/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Texture.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		2D texture resource wrapper providing load, bind to texture unit, and sampler params.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <glad/glad.h>
#include <string>
#include <vector>

class Texture {
public:
	// Construct an empty OpenGL texture wrapper.
	Texture();

	// Destroy owned OpenGL texture handle, if any.
	~Texture();

	// Delete copy constructor and assignment (move-only like your other classes)
	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

	// Allow move operations
	// Move-construct texture ownership from another wrapper.
	Texture(Texture&& other) noexcept;

	// Move-assign texture ownership from another wrapper.
	Texture& operator=(Texture&& other) noexcept;

	// Load texture data from disk and upload it to OpenGL.
	bool LoadFromFile(const std::string& filePath);

	// Upload pre-decoded image bytes to OpenGL.
	bool LoadFromMemory(const unsigned char* data, int imageWidth, int imageHeight, int imageChannels);

	// Decode an image file into CPU memory (no OpenGL calls).
	static bool DecodeFile(const std::string& filePath,
		std::vector<unsigned char>& outData,
		int& outWidth,
		int& outHeight,
		int& outChannels);

	// Bind this texture to a texture unit slot.
	void Bind(unsigned int slot = 0) const;

	// Unbind any texture from the active texture target.
	void Unbind() const;

	// Get loaded texture width in pixels.
	int GetWidth() const {
		return width;
	}

	// Get loaded texture height in pixels.
	int GetHeight() const {
		return height;
	}

	// Get the underlying OpenGL texture object ID.
	GLuint GetID() const {
		return textureID;
	}

private:
	GLuint textureID;
	int width, height, channels;
};

