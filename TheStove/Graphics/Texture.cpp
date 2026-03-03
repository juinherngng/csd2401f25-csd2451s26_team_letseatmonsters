/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Texture.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Loads image data, creates GL texture, sets filtering/wrap, bind/unbind/cleanup.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "Texture.hpp"

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "../extern/stb_image/stb_image.h"

Texture::Texture() : textureID(0), width(0), height(0), channels(0) {
}

Texture::~Texture() {
	if (textureID != 0) {
		glDeleteTextures(1, &textureID);
	}
}

// Move constructor
Texture::Texture(Texture&& other) noexcept
	: textureID(other.textureID), width(other.width), height(other.height), channels(other.channels) {
	other.textureID = 0;
	other.width = 0;
	other.height = 0;
	other.channels = 0;
}

// Move assignment
Texture& Texture::operator=(Texture&& other) noexcept {
	if (this != &other) {
		if (textureID != 0) {
			glDeleteTextures(1, &textureID);
		}

		textureID = other.textureID;
		width = other.width;
		height = other.height;
		channels = other.channels;

		other.textureID = 0;
		other.width = 0;
		other.height = 0;
		other.channels = 0;
	}
	return *this;
}

bool Texture::LoadFromFile(const std::string& filePath) {
	// Flip image vertically (OpenGL expects texture coordinates to start from bottom-left)
	stbi_set_flip_vertically_on_load(true);

	unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);

	if (!data) {
		std::cerr << "Failed to load texture: " << filePath << std::endl;
		std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
		return false;
	}

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Set texture wrapping/filtering options
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  // Pixel art friendly

	// Upload texture data
	GLenum format = GL_RGB;
	if (channels == 1)
		format = GL_RED;
	else if (channels == 3)
		format = GL_RGB;
	else if (channels == 4)
		format = GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);

	std::cout << "Loaded texture: " << filePath << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;
	return true;
}

void Texture::Bind(unsigned int slot) const {

	if (textureID == 0) {
		std::cerr << "Warning: Trying to bind invalid texture (ID = 0)" << std::endl;
		return;  // Don't bind invalid texture
	}

	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, textureID);
}

void Texture::Unbind() const {
	glBindTexture(GL_TEXTURE_2D, 0);
}
