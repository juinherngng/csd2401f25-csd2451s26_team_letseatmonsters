/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Texture.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (70%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	    (30%)

 DESCRIPTION:		Loads image data, creates GL texture, sets filtering/wrap, bind/unbind/cleanup.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <algorithm>
#include <vector>

#include "EngineCore/Logger.hpp"
#include "EngineGraphics/Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "EngineGraphics/stb_image.h"

/**
 * @brief Initializes the wrapper with no active GPU texture.
 */
Texture::Texture() : textureID(0), width(0), height(0), channels(0), samplingMode(SamplingMode::PixelArt) {}

/**
 * @brief Deletes the owned GPU texture when the wrapper goes out of scope.
 */
Texture::~Texture() {
	if (textureID != 0) {
		glDeleteTextures(1, &textureID);
	}
}

/**
 * @brief Transfers GPU texture ownership from another wrapper during construction.
 * @param other Source wrapper whose texture handle is adopted.
 */
Texture::Texture(Texture&& other) noexcept
	: textureID(other.textureID), width(other.width), height(other.height), channels(other.channels), samplingMode(other.samplingMode) {
	other.textureID = 0;
	other.width = 0;
	other.height = 0;
	other.channels = 0;
	other.samplingMode = SamplingMode::PixelArt;
}

/**
 * @brief Transfers GPU texture ownership from another wrapper during assignment.
 * @param other Source wrapper whose texture handle is adopted.
 * @return Reference to this wrapper after ownership transfer.
 */
Texture& Texture::operator=(Texture&& other) noexcept {
	if (this != &other) {
		// Release any texture this wrapper already owns before adopting the incoming one.
		if (textureID != 0) {
			glDeleteTextures(1, &textureID);
		}

		textureID = other.textureID;
		width = other.width;
		height = other.height;
		channels = other.channels;
		samplingMode = other.samplingMode;

		other.textureID = 0;
		other.width = 0;
		other.height = 0;
		other.channels = 0;
		other.samplingMode = SamplingMode::PixelArt;
	}
	return *this;
}

/**
 * @brief Decodes an image file into CPU memory using stb_image without issuing any OpenGL calls.
 * @param filePath Relative or absolute path to the image file.
 * @param outData Output vector filled with decoded pixel data.
 * @param outWidth Output image width in pixels.
 * @param outHeight Output image height in pixels.
 * @param outChannels Output channel count of the decoded image.
 * @return `true` if the file was decoded successfully.
 */
bool Texture::DecodeFile(const std::string& filePath,
	std::vector<unsigned char>& outData,
	int& outWidth,
	int& outHeight,
	int& outChannels) {
	unsigned char* raw = stbi_load(filePath.c_str(), &outWidth, &outHeight, &outChannels, 0);
	if (!raw) {
		TS_LOG_ERROR("[Texture] Failed to decode texture: " << filePath);
		TS_LOG_ERROR("[Texture] STB Error: " << stbi_failure_reason());
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

/**
 * @brief Allocates a texture and uploads pre-decoded image bytes into it.
 * @param data Raw pixel data to upload.
 * @param imageWidth Image width in pixels.
 * @param imageHeight Image height in pixels.
 * @param imageChannels Number of channels stored in `data`.
 * @return `true` if the texture was allocated and filled successfully.
 */
bool Texture::LoadFromMemory(const unsigned char* data,
	int imageWidth,
	int imageHeight,
	int imageChannels,
	SamplingMode requestedSamplingMode) {
	if (!data || imageWidth <= 0 || imageHeight <= 0 || imageChannels <= 0) {
		return false;
	}

	if (!AllocateEmpty(imageWidth, imageHeight, imageChannels, requestedSamplingMode)) {
		return false;
	}

	GLenum format = GL_RGB;
	if (imageChannels == 1)
		format = GL_RED;
	else if (imageChannels == 3)
		format = GL_RGB;
	else if (imageChannels == 4)
		format = GL_RGBA;

	return UpdateFromMemory(data, imageWidth, imageHeight, format, GL_UNSIGNED_BYTE);
}

/**
 * @brief Allocates empty GPU storage for a texture that will be updated later.
 * @param imageWidth Texture width in pixels.
 * @param imageHeight Texture height in pixels.
 * @param imageChannels Channel count used to choose the internal storage format.
 * @return `true` if allocation succeeded.
 */
bool Texture::AllocateEmpty(int imageWidth, int imageHeight, int imageChannels, SamplingMode requestedSamplingMode) {
	if (imageWidth <= 0 || imageHeight <= 0 || imageChannels <= 0) {
		return false;
	}

	width = imageWidth;
	height = imageHeight;
	channels = imageChannels;
	samplingMode = requestedSamplingMode;

	if (textureID != 0) {
		glDeleteTextures(1, &textureID);
		textureID = 0;
	}

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	ApplySamplingParameters();

	GLenum format = GL_RGB;
	if (channels == 1)
		format = GL_RED;
	else if (channels == 3)
		format = GL_RGB;
	else if (channels == 4)
		format = GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);

	glBindTexture(GL_TEXTURE_2D, 0);
	return true;
}

void Texture::ApplySamplingParameters() const {
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (samplingMode == SamplingMode::Smooth) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		return;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

/**
 * @brief Updates an already allocated texture with new pixel data.
 * @param data Raw pixel data to upload.
 * @param imageWidth Width of the incoming data.
 * @param imageHeight Height of the incoming data.
 * @param inputFormat OpenGL format describing the incoming pixel layout.
 * @param inputType OpenGL component type describing the incoming pixel layout.
 * @return `true` if the texture dimensions matched and the upload was issued.
 */
bool Texture::UpdateFromMemory(const unsigned char* data,
	int imageWidth,
	int imageHeight,
	GLenum inputFormat,
	GLenum inputType) {
	if (!data || textureID == 0 || imageWidth <= 0 || imageHeight <= 0) {
		return false;
	}

	if (imageWidth != width || imageHeight != height) {
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, inputFormat, inputType, data);
	if (samplingMode == SamplingMode::Smooth) {
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	return true;
}

/**
 * @brief Loads an image from disk, decodes it in CPU memory, and uploads it into a GPU texture.
 * @param filePath Relative or absolute path to the image file.
 * @return `true` if decode and upload both succeed.
 */
bool Texture::LoadFromFile(const std::string& filePath, SamplingMode requestedSamplingMode) {
	std::vector<unsigned char> decoded;
	int imageWidth = 0;
	int imageHeight = 0;
	int imageChannels = 0;

	if (!DecodeFile(filePath, decoded, imageWidth, imageHeight, imageChannels)) {
		return false;
	}

	if (!LoadFromMemory(decoded.data(), imageWidth, imageHeight, imageChannels, requestedSamplingMode)) {
		return false;
	}

#ifndef NDEBUG
	TS_LOG_DEBUG("[Texture] Loaded texture: " << filePath << " (" << width << "x" << height << ", " << channels << " channels)");
#endif
	return true;
}

/**
 * @brief Binds this texture to the requested texture unit.
 * @param slot Texture unit index to bind to.
 */
void Texture::Bind(unsigned int slot) const {

	if (textureID == 0) {
		TS_LOG_WARN("[Texture] Trying to bind invalid texture (ID = 0)");
		return;  // Don't bind invalid texture
	}

	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, textureID);
}

/**
 * @brief Unbinds any currently bound 2D texture from the active texture unit.
 */
void Texture::Unbind() const {
	glBindTexture(GL_TEXTURE_2D, 0);
}
