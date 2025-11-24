/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Animation.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu

 DESCRIPTION:		Time-based sprite animation controller for selecting UV frames in a sprite sheet.

		All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <glm/glm.hpp>
#include <vector>

class Animator2D {
public:
	Animator2D() {
	}

	void SetFrames(const std::vector<glm::vec4>& frames, float frameDuration, bool loop = true) {
		m_Frames = frames;
		m_FrameDuration = frameDuration;
		m_Loop = loop;
		Reset();
		m_Playing = true;
	}

	void Play() {
		m_Playing = true;
	}
	void Pause() {
		m_Playing = false;
	}
	void Stop() {
		m_Playing = false;
		Reset();
	}
	void Reset() {
		m_CurrentFrame = 0;
		m_Accumulator = 0.0f;
	}

	void Update(float deltaTime) {
		if (!m_Playing || m_Frames.empty()) return;
		m_Accumulator += deltaTime;
		if (m_Accumulator >= m_FrameDuration) {
			m_Accumulator -= m_FrameDuration;
			m_CurrentFrame++;
			if (m_CurrentFrame >= (int)m_Frames.size()) {
				m_CurrentFrame = m_Loop?0:(int)m_Frames.size() - 1;
			}
		}
	}

	glm::vec4 GetCurrentFrameUV() const {
		if (m_Frames.empty()) return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
		return m_Frames[m_CurrentFrame];
	}

private:
	std::vector<glm::vec4> m_Frames; // (offsetX, offsetY, scaleX, scaleY)
	float m_FrameDuration = 0.5f;
	float m_Accumulator = 0.0f;
	int m_CurrentFrame = 0;
	bool m_Loop = true;
	bool m_Playing = false;
};
