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
	/**
	 * @brief Constructs an empty OpenGL texture wrapper with no allocated GPU texture yet.
	 */
	Texture();

	/**
	 * @brief Releases the owned GPU texture handle, if one was created.
	 */
	~Texture();

	// Delete copy constructor and assignment (move-only like your other classes)
	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

	/**
	 * @brief Transfers texture ownership from another wrapper during construction.
	 * @param other Source texture wrapper whose GPU handle should be adopted.
	 */
	Texture(Texture&& other) noexcept;

	/**
	 * @brief Transfers texture ownership from another wrapper during assignment.
	 * @param other Source texture wrapper whose GPU handle should be adopted.
	 * @return Reference to this wrapper after ownership transfer.
	 */
	Texture& operator=(Texture&& other) noexcept;

	/**
	 * @brief Loads an image file from disk and uploads it as a GPU texture.
	 * @param filePath Relative or absolute path to the image file.
	 * @return `true` if decoding and upload both succeed.
	 */
	bool LoadFromFile(const std::string& filePath);

	/**
	 * @brief Uploads already-decoded image bytes by allocating a new GPU texture and filling it.
	 * @param data Raw image bytes in row-major order.
	 * @param imageWidth Image width in pixels.
	 * @param imageHeight Image height in pixels.
	 * @param imageChannels Number of color channels stored in `data`.
	 * @return `true` if allocation and upload succeed.
	 */
	bool LoadFromMemory(const unsigned char* data, int imageWidth, int imageHeight, int imageChannels);

	/**
	 * @brief Allocates an empty GPU texture for later streaming updates.
	 * @param imageWidth Texture width in pixels.
	 * @param imageHeight Texture height in pixels.
	 * @param imageChannels Number of channels to allocate storage for.
	 * @return `true` if the empty texture was allocated successfully.
	 */
	bool AllocateEmpty(int imageWidth, int imageHeight, int imageChannels = 4);

	/**
	 * @brief Replaces the contents of the currently allocated texture without recreating it.
	 * @param data Raw pixel data to upload.
	 * @param imageWidth Width of the incoming image data.
	 * @param imageHeight Height of the incoming image data.
	 * @param inputFormat OpenGL pixel format describing the incoming data layout.
	 * @param inputType OpenGL pixel component type describing the incoming data layout.
	 * @return `true` if the existing texture accepted the update.
	 */
	bool UpdateFromMemory(const unsigned char* data,
		int imageWidth,
		int imageHeight,
		GLenum inputFormat = GL_RGBA,
		GLenum inputType = GL_UNSIGNED_BYTE);

	/**
	 * @brief Decodes an image file into CPU memory without touching OpenGL state.
	 * @param filePath Relative or absolute path to the image file.
	 * @param outData Output buffer containing decoded pixels.
	 * @param outWidth Output image width in pixels.
	 * @param outHeight Output image height in pixels.
	 * @param outChannels Output number of decoded color channels.
	 * @return `true` if the file was decoded successfully.
	 */
	static bool DecodeFile(const std::string& filePath,
		std::vector<unsigned char>& outData,
		int& outWidth,
		int& outHeight,
		int& outChannels);

	/**
	 * @brief Binds the texture to the requested texture unit for rendering.
	 * @param slot Texture unit index to bind to.
	 */
	void Bind(unsigned int slot = 0) const;

	/**
	 * @brief Unbinds any texture from the active 2D texture target.
	 */
	void Unbind() const;

	/**
	 * @brief Returns the currently stored texture width in pixels.
	 * @return Texture width.
	 */
	int GetWidth() const {
		return width;
	}

	/**
	 * @brief Returns the currently stored texture height in pixels.
	 * @return Texture height.
	 */
	int GetHeight() const {
		return height;
	}

	/**
	 * @brief Returns the underlying OpenGL texture handle.
	 * @return GPU texture object ID.
	 */
	GLuint GetID() const {
		return textureID;
	}

private:
	GLuint textureID;
	int width, height, channels;
};
