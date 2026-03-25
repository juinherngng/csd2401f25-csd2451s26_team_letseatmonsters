/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Texture.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Loads image data, creates GL texture, sets filtering/wrap, bind/unbind/cleanup.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "Texture.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

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

// Static method to decode an image file into CPU memory using stb_image (no OpenGL calls)
bool Texture::DecodeFile(const std::string& filePath,
	std::vector<unsigned char>& outData,
	int& outWidth,
	int& outHeight,
	int& outChannels) {
	unsigned char* raw = stbi_load(filePath.c_str(), &outWidth, &outHeight, &outChannels, 0);
	if (!raw) {
		std::cerr << "Failed to decode texture: " << filePath << std::endl;
		std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
		return false;
	}

	const int rowBytes = outWidth * outChannels;
	outData.assign(raw, raw + (outHeight * rowBytes));
	stbi_image_free(raw);

	// Flip vertically so UVs stay consistent with the existing renderer convention.
	for (int y = 0; y < outHeight / 2; ++y) {
		const int top = y * rowBytes;
		const int bot = (outHeight - 1 - y) * rowBytes;
		for (int i = 0; i < rowBytes; ++i) {
			std::swap(outData[top + i], outData[bot + i]);
		}
	}

	return true;
}

// Upload pre-decoded image bytes to OpenGL and create a texture object
bool Texture::LoadFromMemory(const unsigned char* data, int imageWidth, int imageHeight, int imageChannels) {
	if (!data || imageWidth <= 0 || imageHeight <= 0 || imageChannels <= 0) {
		return false;
	}

	width = imageWidth;
	height = imageHeight;
	channels = imageChannels;

	if (textureID != 0) {
		glDeleteTextures(1, &textureID);
		textureID = 0;
	}

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Set texture wrapping/filtering options
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  // Pixel art friendly

	GLenum format = GL_RGB;
	if (channels == 1)
		format = GL_RED;
	else if (channels == 3)
		format = GL_RGB;
	else if (channels == 4)
		format = GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

	glBindTexture(GL_TEXTURE_2D, 0);
	return true;
}

// Load texture data from disk and upload it to OpenGL
bool Texture::LoadFromFile(const std::string& filePath) {
	std::vector<unsigned char> decoded;
	int imageWidth = 0;
	int imageHeight = 0;
	int imageChannels = 0;

	if (!DecodeFile(filePath, decoded, imageWidth, imageHeight, imageChannels)) {
		return false;
	}

	if (!LoadFromMemory(decoded.data(), imageWidth, imageHeight, imageChannels)) {
		return false;
	}

#ifndef NDEBUG
	std::cout << "Loaded texture: " << filePath << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;
#endif
	return true;
}

// Bind this texture to a texture unit slot (default slot 0)
void Texture::Bind(unsigned int slot) const {

	if (textureID == 0) {
		std::cerr << "Warning: Trying to bind invalid texture (ID = 0)" << std::endl;
		return;  // Don't bind invalid texture
	}

	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, textureID);
}

// Unbind any texture from the active texture target
void Texture::Unbind() const {
	glBindTexture(GL_TEXTURE_2D, 0);
}

