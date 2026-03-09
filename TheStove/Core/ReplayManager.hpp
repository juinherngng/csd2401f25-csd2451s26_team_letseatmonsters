#pragma once

#include "InputManager.hpp"

#include <cstdint>
#include <string>
#include <vector>

class ReplayManager {
public:
	struct Frame {
		float dt = 0.0f;
		InputManager::Snapshot input{};
	};

	void StartRecording();
	bool StopRecording(const std::string& path);

	bool StartPlayback(const std::string& path);
	void StopPlayback();

	bool IsRecording() const;
	bool IsPlaybackActive() const;

	void RecordFrame(const InputManager& input, float dt);
	bool GetNextPlaybackFrame(InputManager::Snapshot& outSnapshot, float& outDt);

	std::uint32_t GetSeed() const {
		return seed_;
	}

private:
	bool recording_ = false;
	bool playback_ = false;
	std::uint32_t seed_ = 0;
	std::vector<Frame> frames_;
	std::size_t playbackIndex_ = 0;
};
