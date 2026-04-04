/*
----------------------------------------------------------------------------------------------------
 FILE NAME:         VideoPlayer.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Implements Windows Media Foundation backed MP4 playback by decoding frames into
					a reusable OpenGL texture owned by the engine.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include "EngineCore/Logger.hpp"
#include "EngineGraphics/VideoPlayer.hpp"

#ifdef _WIN32
#include <filesystem>
#include <mutex>
#include <wrl/client.h>
#include <combaseapi.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

using Microsoft::WRL::ComPtr;

namespace {
	std::mutex gVideoBootstrapMutex;
	int gMediaFoundationRefCount = 0;

	/**
	 * @brief Starts Media Foundation once for the process and reference-counts later users.
	 * @return `true` when Media Foundation is ready for decoding work.
	 */
	bool AcquireMediaFoundation() {
		std::scoped_lock lock(gVideoBootstrapMutex);
		if (gMediaFoundationRefCount == 0) {
			// Bootstrap Media Foundation only for the first video player that needs it.
			const HRESULT hr = MFStartup(MF_VERSION);
			if (FAILED(hr)) {
				TS_LOG_ERROR("[VideoPlayer] MFStartup failed: 0x" << std::hex << hr);
				return false;
			}
		}

		// Count every active consumer so shutdown happens only after the last one closes.
		++gMediaFoundationRefCount;
		return true;
	}

	/**
	 * @brief Releases one Media Foundation user and shuts the subsystem down when the last one leaves.
	 */
	void ReleaseMediaFoundation() {
		std::scoped_lock lock(gVideoBootstrapMutex);
		if (gMediaFoundationRefCount <= 0) {
			// Guard against mismatched release calls.
			return;
		}

		--gMediaFoundationRefCount;
		if (gMediaFoundationRefCount == 0) {
			// Tear Media Foundation down only after the final player releases it.
			MFShutdown();
		}
	}
}

struct VideoPlayer::Impl {
	ComPtr<IMFSourceReader> reader;
	bool mediaFoundationActive = false;
	bool comInitialized = false;
};
#endif

/**
 * @brief Ensures any active video stream is closed before the player is destroyed.
 */
VideoPlayer::~VideoPlayer() {
	Close();
}

/**
 * @brief Opens a video file for frame-by-frame playback into an OpenGL texture.
 * @param filePath Relative or absolute path to the target video file.
 * @param loop Whether playback should restart automatically after the last frame.
 * @return `true` when the stream and its first uploaded frame are ready to render.
 */
bool VideoPlayer::Open(const std::string& filePath, bool loop) {
	// Always tear down the previous stream first so this instance can be reused safely.
	Close();

	filePath_ = filePath;
	loop_ = loop;
	ended_ = false;
	accumulator_ = 0.0f;

#ifndef _WIN32
	TS_LOG_WARN("[VideoPlayer] MP4 playback is only available on Windows builds.");
	return false;
#else
	impl_ = new Impl();

	const HRESULT comHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (SUCCEEDED(comHr)) {
		// This player owns a fresh COM apartment that must be torn down on Close().
		impl_->comInitialized = true;
	}
	else if (comHr != RPC_E_CHANGED_MODE) {
		TS_LOG_ERROR("[VideoPlayer] CoInitializeEx failed: 0x" << std::hex << comHr);
		Close();
		return false;
	}

	if (!AcquireMediaFoundation()) {
		Close();
		return false;
	}

	// Remember that this instance must release the shared Media Foundation bootstrap later.
	impl_->mediaFoundationActive = true;

	return ReopenStream();
#endif
}

/**
 * @brief Resets playback state and releases decoder-side resources held by the player.
 */
void VideoPlayer::Close() {
	// Clear the high-level playback state before releasing platform-specific resources.
	open_ = false;
	ended_ = false;
	accumulator_ = 0.0f;
	width_ = 0;
	height_ = 0;
	fps_ = 30.0f;
	frameDuration_ = 1.0f / 30.0f;

#ifdef _WIN32
	if (impl_ != nullptr) {
		// Release the reader first so no MF objects outlive subsystem shutdown.
		impl_->reader.Reset();

		if (impl_->mediaFoundationActive) {
			// Drop this player's reference on the shared Media Foundation bootstrap.
			ReleaseMediaFoundation();
		}

		if (impl_->comInitialized) {
			// Leave the COM apartment if this player created one during Open().
			CoUninitialize();
		}

		// Destroy the platform-specific implementation payload once all resources are released.
		delete impl_;
		impl_ = nullptr;
	}
#endif
}

/**
 * @brief Advances the current video by enough frames to match the elapsed frame time.
 * @param deltaTime Frame delta time in seconds.
 */
void VideoPlayer::Update(float deltaTime) {
	if (!open_ || ended_) {
		return;
	}

	// Accumulate frame time and drain it in fixed frame-duration steps so playback speed stays stable.
	accumulator_ += (deltaTime > 0.0f) ? deltaTime : 0.0f;
	while (accumulator_ >= frameDuration_ && open_ && !ended_) {
		accumulator_ -= frameDuration_;
		if (!ReadAndUploadNextFrame()) {
			break;
		}
	}
}

/**
 * @brief Pulls the next decoded frame from Media Foundation and uploads it into the streaming texture.
 * @return `true` when a frame upload completed, otherwise `false`.
 */
bool VideoPlayer::ReadAndUploadNextFrame() {
#ifndef _WIN32
	return false;
#else
	if (impl_ == nullptr || impl_->reader == nullptr) {
		return false;
	}

	while (true) {
		DWORD streamFlags = 0;
		LONGLONG sampleTime = 0;
		ComPtr<IMFSample> sample;
		const HRESULT hr = impl_->reader->ReadSample(
			static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
			0,
			nullptr,
			&streamFlags,
			&sampleTime,
			&sample);

		if (FAILED(hr)) {
			TS_LOG_ERROR("[VideoPlayer] ReadSample failed: 0x" << std::hex << hr);
			ended_ = true;
			open_ = false;
			return false;
		}

		if ((streamFlags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) {
			// Looping reopens the same file and primes a fresh first frame; non-looping playback stops here.
			if (loop_) {
				return ReopenStream();
			}

			ended_ = true;
			open_ = false;
			return false;
		}

		if (!sample) {
			continue;
		}

		ComPtr<IMFMediaBuffer> buffer;
		if (FAILED(sample->ConvertToContiguousBuffer(&buffer)) || !buffer) {
			// Skip samples that cannot be exposed as one CPU-readable buffer.
			continue;
		}

		BYTE* rawData = nullptr;
		DWORD maxLength = 0;
		DWORD currentLength = 0;
		if (FAILED(buffer->Lock(&rawData, &maxLength, &currentLength)) || rawData == nullptr) {
			// Skip this sample when the pixel payload cannot be mapped.
			continue;
		}

		// Media Foundation gives us tightly packed BGRA/BGRX rows for the current sample.
		const size_t rowBytes = static_cast<size_t>(width_) * 4u;
		const size_t expectedBytes = rowBytes * static_cast<size_t>(height_);
		std::vector<unsigned char> pixels(expectedBytes);

		if (currentLength >= expectedBytes) {
			// Flip the image so it matches the renderer's texture orientation convention.
			for (int y = 0; y < height_; ++y) {
				const unsigned char* src = reinterpret_cast<unsigned char*>(rawData) + (static_cast<size_t>(y) * rowBytes);
				unsigned char* dst = pixels.data() + (static_cast<size_t>(height_ - 1 - y) * rowBytes);
				std::memcpy(dst, src, rowBytes);
			}

			// Media Foundation's RGB32 output uses a padding byte that is not guaranteed to contain
			// a valid alpha channel value, so force each uploaded pixel to fully opaque.
			for (size_t i = 3; i < pixels.size(); i += 4) {
				pixels[i] = 255;
			}
		}

		buffer->Unlock();

		// Upload the fully prepared frame into the reusable texture that the cutscene sprite points at.
		if (!pixels.empty() &&
			texture_.UpdateFromMemory(pixels.data(), width_, height_, GL_BGRA, GL_UNSIGNED_BYTE)) {
			return true;
		}
	}
#endif
}

/**
 * @brief Rebuilds the current Media Foundation reader and primes the texture with the first frame.
 * @return `true` if the reader, stream metadata, and initial frame upload all succeeded.
 */
bool VideoPlayer::ReopenStream() {
#ifndef _WIN32
	return false;
#else
	if (impl_ == nullptr) {
		return false;
	}

	impl_->reader.Reset();

	ComPtr<IMFAttributes> attributes;
	HRESULT hr = MFCreateAttributes(&attributes, 1);
	if (FAILED(hr)) {
		TS_LOG_ERROR("[VideoPlayer] MFCreateAttributes failed: 0x" << std::hex << hr);
		return false;
	}

	// Ask the source reader to convert frames into a decoder-friendly video-processing path.
	attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);

	// Recreate the source reader from the current file path so looping can start from frame zero cleanly.
	const std::wstring widePath = std::filesystem::path(filePath_).wstring();
	hr = MFCreateSourceReaderFromURL(widePath.c_str(), attributes.Get(), &impl_->reader);
	if (FAILED(hr) || !impl_->reader) {
		TS_LOG_ERROR("[VideoPlayer] Failed to open video: " << filePath_);
		return false;
	}

	ComPtr<IMFMediaType> outputType;
	hr = MFCreateMediaType(&outputType);
	if (FAILED(hr)) {
		TS_LOG_ERROR("[VideoPlayer] MFCreateMediaType failed: 0x" << std::hex << hr);
		return false;
	}

	outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);

	// Request RGB32 output so the decoded sample can be uploaded directly into the engine texture.
	hr = impl_->reader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, outputType.Get());
	if (FAILED(hr)) {
		TS_LOG_ERROR("[VideoPlayer] SetCurrentMediaType failed: 0x" << std::hex << hr);
		return false;
	}

	ComPtr<IMFMediaType> currentType;
	hr = impl_->reader->GetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), &currentType);
	if (FAILED(hr) || !currentType) {
		TS_LOG_ERROR("[VideoPlayer] GetCurrentMediaType failed: 0x" << std::hex << hr);
		return false;
	}

	UINT32 width = 0;
	UINT32 height = 0;
	hr = MFGetAttributeSize(currentType.Get(), MF_MT_FRAME_SIZE, &width, &height);
	if (FAILED(hr) || width == 0 || height == 0) {
		TS_LOG_ERROR("[VideoPlayer] Failed to read frame size for video: " << filePath_);
		return false;
	}

	UINT32 frameRateNum = 0;
	UINT32 frameRateDen = 0;
	if (FAILED(MFGetAttributeRatio(currentType.Get(), MF_MT_FRAME_RATE, &frameRateNum, &frameRateDen)) ||
		frameRateNum == 0 || frameRateDen == 0) {
		// Fall back to a safe default when stream metadata omits a usable frame rate.
		fps_ = 30.0f;
	}
	else {
		// Convert the rational frame-rate metadata into frames per second.
		fps_ = static_cast<float>(frameRateNum) / static_cast<float>(frameRateDen);
	}

	if (fps_ <= 0.0f || !std::isfinite(fps_)) {
		// Clamp invalid metadata back to a stable default playback rate.
		fps_ = 30.0f;
	}

	// Resize and reset the streaming texture whenever a new stream or loop pass is opened.
	frameDuration_ = 1.0f / fps_;
	width_ = static_cast<int>(width);
	height_ = static_cast<int>(height);

	if (!texture_.AllocateEmpty(width_, height_, 4)) {
		TS_LOG_ERROR("[VideoPlayer] Failed to allocate streaming texture for: " << filePath_);
		return false;
	}

	open_ = true;
	ended_ = false;
	accumulator_ = 0.0f;

	// Prime the texture immediately so the first rendered frame is visible without waiting a full tick.
	return ReadAndUploadNextFrame();
#endif
}
