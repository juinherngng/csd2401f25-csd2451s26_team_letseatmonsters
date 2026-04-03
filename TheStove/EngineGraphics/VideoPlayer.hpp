/*
----------------------------------------------------------------------------------------------------
 FILE NAME:         VideoPlayer.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Declares a lightweight MP4 video player that decodes frames into an engine
					Texture for fullscreen menu and cutscene playback.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <string>

#include "EngineGraphics/Texture.hpp"

class VideoPlayer {
public:
	/**
	 * @brief Creates an empty video player wrapper.
	 */
	VideoPlayer() = default;

	/**
	 * @brief Releases decoder state, COM/MF resources, and the streaming texture.
	 */
	~VideoPlayer();

	/**
	 * @brief Opens an MP4-style video file and prepares the first frame for rendering.
	 * @param filePath Relative or absolute path to the video file.
	 * @param loop Whether playback should restart automatically after reaching the end.
	 * @return `true` if the stream was opened and its first frame was uploaded successfully.
	 */
	bool Open(const std::string& filePath, bool loop = false);

	/**
	 * @brief Stops playback and releases any active decode/upload resources.
	 */
	void Close();

	/**
	 * @brief Advances playback based on elapsed frame time and uploads any newly due frames.
	 * @param deltaTime Frame delta time in seconds.
	 */
	void Update(float deltaTime);

	/**
	 * @brief Returns whether a video stream is currently open and ready for playback.
	 * @return `true` when the player still owns a valid open stream.
	 */
	bool IsOpen() const {
		return open_;
	}

	/**
	 * @brief Returns whether playback has reached the end of the current stream.
	 * @return `true` once the video has ended and is not looping.
	 */
	bool HasEnded() const {
		return ended_;
	}

	/**
	 * @brief Returns the streaming texture that currently holds the most recently decoded frame.
	 * @return Texture pointer when the stream is open, otherwise `nullptr`.
	 */
	Texture* GetTexture() {
		return open_ ? &texture_ : nullptr;
	}

	/**
	 * @brief Returns the streaming texture that currently holds the most recently decoded frame.
	 * @return Texture pointer when the stream is open, otherwise `nullptr`.
	 */
	const Texture* GetTexture() const {
		return open_ ? &texture_ : nullptr;
	}

	/**
	 * @brief Returns the decoded frame width in pixels.
	 * @return Current video width.
	 */
	int GetWidth() const {
		return width_;
	}

	/**
	 * @brief Returns the decoded frame height in pixels.
	 * @return Current video height.
	 */
	int GetHeight() const {
		return height_;
	}

private:
	/**
	 * @brief Reads the next available sample from Media Foundation and uploads it into the texture.
	 * @return `true` if a frame was uploaded, `false` if playback stopped or failed.
	 */
	bool ReadAndUploadNextFrame();

	/**
	 * @brief Recreates the source reader and streaming texture for the current file path.
	 * @return `true` if the stream was reopened and primed successfully.
	 */
	bool ReopenStream();

	Texture texture_;
	std::string filePath_;
	float fps_ = 30.0f;
	float frameDuration_ = 1.0f / 30.0f;
	float accumulator_ = 0.0f;
	int width_ = 0;
	int height_ = 0;
	bool open_ = false;
	bool ended_ = false;
	bool loop_ = false;

#ifdef _WIN32
	struct Impl;
	Impl* impl_ = nullptr;
#endif
};
