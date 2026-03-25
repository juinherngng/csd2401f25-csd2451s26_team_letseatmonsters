/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			ReplayManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Declares `ReplayManager`, a lightweight record/playback utility for deterministic
					input-driven replays.

	Responsibilities:
	  1) Capture per-frame input snapshots and frame delta time while recording.
	  2) Serialize replay data to JSON, including RNG seed and level boot metadata.
	  3) Load replay files and feed frames sequentially during playback.
	  4) Expose recording/playback state and recorded startup metadata.

	Determinism notes:
	  - Recording stores an RNG seed generated at record start.
	  - Playback restores the same seed before frame playback begins.
	  - Replay startup metadata contains level path + simulation active flag so callers
		can restore the same scene state before consuming replay frames.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "InputManager.hpp"

#include <cstdint>
#include <string>
#include <vector>

 /**
  * @brief Records and replays frame-by-frame input snapshots.
  *
  * The manager is intentionally data-oriented:
  * - one replay frame = {dt + input snapshot}
  * - replay file = metadata + frame array
  */
class ReplayManager {
public:
	/**
	 * @brief Single replay frame payload.
	 */
	struct Frame {
		float dt = 0.0f;                 // frame delta time used during recording
		InputManager::Snapshot input{};  // captured input state for this frame
	};

	/**
	 * @brief Begins recording and resets all prior buffered replay data.
	 * @param levelPath Scene/level path active at recording start.
	 * @param simulationActive Simulation state active at recording start.
	 */
	void StartRecording(const std::string& levelPath, bool simulationActive);

	/**
	 * @brief Stops recording and writes replay JSON to disk.
	 * @param path Destination replay file path.
	 * @return True if write succeeded.
	 */
	bool StopRecording(const std::string& path);

	/**
	 * @brief Loads replay data from disk and arms playback.
	 * @param path Source replay file path.
	 * @return True if valid frames were loaded.
	 */
	bool StartPlayback(const std::string& path);

	/**
	 * @brief Stops playback and rewinds playback cursor.
	 */
	void StopPlayback();

	/**
	 * @return True while currently capturing frames.
	 */
	bool IsRecording() const;

	/**
	 * @return True while playback is active and frames remain consumable.
	 */
	bool IsPlaybackActive() const;

	/**
	 * @brief Captures one frame from current input + dt during recording.
	 * @param input Input source to snapshot.
	 * @param dt Frame delta time.
	 */
	void RecordFrame(const InputManager& input, float dt);

	/**
	 * @brief Retrieves next playback frame.
	 * @param outSnapshot Output input snapshot for the frame.
	 * @param outDt Output dt for the frame.
	 * @return True if a frame was produced; false when playback is exhausted/inactive.
	 */
	bool GetNextPlaybackFrame(InputManager::Snapshot& outSnapshot, float& outDt);

	/**
	 * @return Replay RNG seed (recorded or loaded).
	 */
	std::uint32_t GetSeed() const {
		return seed_;
	}

	/**
	 * @return Level path captured in replay metadata.
	 */
	const std::string& GetRecordedLevelPath() const {
		return recordedLevelPath_;
	}

	/**
	 * @return Simulation-active flag captured in replay metadata.
	 */
	bool GetRecordedSimulationActive() const {
		return recordedSimulationActive_;
	}

private:
	bool recording_ = false;                  // recording mode flag
	bool playback_ = false;                   // playback mode flag
	std::uint32_t seed_ = 0;                  // deterministic RNG seed

	std::string recordedLevelPath_;           // startup metadata: level path
	bool recordedSimulationActive_ = false;   // startup metadata: simulation state

	std::vector<Frame> frames_;               // in-memory replay frame buffer
	std::size_t playbackIndex_ = 0;           // playback cursor into frames_
};
