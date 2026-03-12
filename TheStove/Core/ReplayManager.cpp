#include "ReplayManager.hpp"

#include "EngineRng.hpp"
#include "JSONInclude.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

using nlohmann::json;

void ReplayManager::StartRecording(const std::string& levelPath, bool simulationActive) {
	recording_ = true;
	playback_ = false;
	frames_.clear();
	playbackIndex_ = 0;
	recordedLevelPath_ = levelPath;
	recordedSimulationActive_ = simulationActive;

	seed_ = EngineRng::CreateSeed();
	EngineRng::SetSeed(seed_);

	std::cout << "[Replay] Recording started (seed=" << seed_ << ")\n";
}

bool ReplayManager::StopRecording(const std::string& path) {
	if (!recording_) {
		std::cout << "[Replay] StopRecording ignored (not recording)\n";
		return false;
	}

	recording_ = false;

	const std::filesystem::path replayPath(path);
	if (replayPath.has_parent_path()) {
		std::filesystem::create_directories(replayPath.parent_path());
	}

	json root;
	root["version"] = 1;
	root["seed"] = seed_;
	root["levelPath"] = recordedLevelPath_;
	root["simulationActive"] = recordedSimulationActive_;
	root["frames"] = json::array();

	for (const auto& frame : frames_) {
		json entry;
		entry["dt"] = frame.dt;
		entry["mouseX"] = frame.input.mousePos.x;
		entry["mouseY"] = frame.input.mousePos.y;
		entry["keys"] = frame.input.pressedKeys;
		entry["mouseButtons"] = frame.input.pressedMouseButtons;
		root["frames"].push_back(std::move(entry));
	}

	std::ofstream file(path);
	if (!file.is_open()) {
		std::cout << "[Replay] Failed to write replay: " << path << "\n";
		return false;
	}

	file << root.dump(2);
	std::cout << "[Replay] Recording saved: " << path
		<< " (frames=" << frames_.size() << ")\n";
	return true;
}

bool ReplayManager::StartPlayback(const std::string& path) {
	const bool wasRecording = recording_;

	std::ifstream file(path);
	if (!file.is_open()) {
		std::cout << "[Replay] Failed to open replay: " << path << "\n";
		return false;
	}

	json root;
	file >> root;

	frames_.clear();
	playbackIndex_ = 0;

	seed_ = root.value("seed", 0u);
	recordedLevelPath_ = root.value("levelPath", std::string{});
	recordedSimulationActive_ = root.value("simulationActive", false);
	EngineRng::SetSeed(seed_);

	const auto& frames = root["frames"];
	for (const auto& entry : frames) {
		Frame frame;
		frame.dt = entry.value("dt", 0.0f);
		frame.input.mousePos = {
			entry.value("mouseX", 0.0),
			entry.value("mouseY", 0.0)
		};
		frame.input.pressedKeys = entry.value("keys", std::vector<int>{});
		frame.input.pressedMouseButtons = entry.value("mouseButtons", std::vector<int>{});
		frames_.push_back(std::move(frame));
	}

	recording_ = false;
	playback_ = !frames_.empty();

	if (wasRecording) {
		std::cout << "[Replay] Recording aborted due to playback start\n";
	}

	if (!playback_) {
		std::cout << "[Replay] Playback failed (no frames): " << path << "\n";
		return false;
	}

	std::cout << "[Replay] Playback started: " << path
		<< " (frames=" << frames_.size() << ", seed=" << seed_
		<< ", levelPath=" << recordedLevelPath_
		<< ", simulationActive=" << (recordedSimulationActive_ ? "true" : "false") << ")\n";
	return true;
}

void ReplayManager::StopPlayback() {
	if (!playback_) {
		return;
	}

	playback_ = false;
	playbackIndex_ = 0;
	std::cout << "[Replay] Playback stopped\n";
}

bool ReplayManager::IsRecording() const {
	return recording_;
}

bool ReplayManager::IsPlaybackActive() const {
	return playback_;
}

void ReplayManager::RecordFrame(const InputManager& input, float dt) {
	if (!recording_) {
		return;
	}

	Frame frame;
	frame.dt = dt;
	input.CaptureSnapshot(frame.input);
	frames_.push_back(std::move(frame));
}

bool ReplayManager::GetNextPlaybackFrame(InputManager::Snapshot& outSnapshot, float& outDt) {
	if (!playback_ || playbackIndex_ >= frames_.size()) {
		return false;
	}

	const Frame& frame = frames_[playbackIndex_++];
	outSnapshot = frame.input;
	outDt = frame.dt;
	return true;
}