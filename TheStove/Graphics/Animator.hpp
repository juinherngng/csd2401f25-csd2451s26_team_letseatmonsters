/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Animator.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Declares the Animator2D helper class, which manages simple 2D sprite animations using a list
    				of UV frames. The animator advances frames over time based on a configurable frame duration,
    				supports looping or one-shot playback, and exposes the current frame’s UV rectangle for use
    				by the renderer.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <glm/glm.hpp>
#include <vector>

class Animator2D {
public:
	// Default-constructed animator starts stopped with no frames
	Animator2D() {
	}

	// Configure the animation sequence
    // frames: list of UV rects (x, y, w, h) in texture space
    // frameDuration: time (seconds) per frame
    // loop: if true, animation restarts after the last frame; otherwise freezes on the last frame
	void SetFrames(const std::vector<glm::vec4>& frames, float frameDuration, bool loop = true) {
		m_Frames = frames;
		m_FrameDuration = frameDuration;
		m_Loop = loop;
		Reset();
		m_Playing = true;
	}

	// Start or resume playing from the current frame
	void Play() {
		m_Playing = true;
	}

	 // Temporarily stop advancing frames (keeps current frame)
	void Pause() {
		m_Playing = false;
	}

	 // Stop playback and rewind to the first frame
	void Stop() {
		m_Playing = false;
		Reset();
	}

	// Reset frame index and internal time accumulator
	void Reset() {
		m_CurrentFrame = 0;
		m_Accumulator = 0.0f;
	}

	// Advance the animation by deltaTime seconds
	void Update(float deltaTime) {
		if (!m_Playing || m_Frames.empty()) {
			return;
		}

		// Basic guard against invalid frame duration
		if (m_FrameDuration <= 0.0f) {
			m_FrameDuration = 0.1f; // default to 10 FPS if bad data
		}

		m_Accumulator += deltaTime;

		// Consume as many frames as the accumulator allows
		while (m_Accumulator >= m_FrameDuration) {
			m_Accumulator -= m_FrameDuration;
			++m_CurrentFrame;
			if (m_CurrentFrame >= static_cast<int>(m_Frames.size())) {
				m_CurrentFrame = m_Loop ? 0 : static_cast<int>(m_Frames.size()) - 1;

				// If not looping, stop advancing and clear accumulator
				if (!m_Loop) {
					m_Accumulator = 0.0f;
					break;
				}
			}
		}
	}

	// Get the UV rect for the current frame
    // Returns full texture (0,0,1,1) if no frames are configured
	glm::vec4 GetCurrentFrameUV() const {
		if (m_Frames.empty()) return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
		return m_Frames[m_CurrentFrame];
	}

private:
	std::vector<glm::vec4> m_Frames; 	// UV frames: (u, v, width, height)
	float m_FrameDuration = 0.5f;		// Seconds each frame stays on screen
	float m_Accumulator = 0.0f;			// Accumulated time since last frame advance
	int m_CurrentFrame = 0;				// Index into m_Frames
	bool m_Loop = true;					// Whether to restart after the last frame
	bool m_Playing = false;				// Whether Update() is allowed to advance frames
};
