/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			AnimationManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (90%)
 CO-AUTHORS: 		Ng Juin Herng, juinherng.ng@digipen.edu (10%)

 DESCRIPTION:	    Declares AnimationManager, which manages 2D sprite animations for game objects,
					updating frame UVs based on Animator2D components. Supports play/pause control
					and registering animation sets for different entity types.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "../Core/System.hpp"  // For SystemInterface
#include "Animator.hpp"		   // This includes Animator2D

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

class EntityManager;  // Forward declaration

/**
 * @brief Manages animations for all game objects
 *
 * Handles animation registration, playback, and frame updates.
 * Works with EntityManager to apply animations to GameObjects.
 */
class AnimationManager : public CoreFramework::SystemInterface {
public:
	AnimationManager() = default;
	~AnimationManager() = default;

	// SystemInterface implementation
	void Initialize() override;
	void Update(float deltaTime) override;
	std::string GetName() override;

	// Set the EntityManager reference (must be called after construction)
	void SetEntityManager(EntityManager* entityMgr);

	// Core functionality
	void Clear();

	// Play/Pause control
	void Play();
	void Pause();
	void Stop();
	bool IsPlaying() const {
		return isPlaying;
	}

	// Animation registration for specific entity types
	void AttachDinoAnimations(int objectID);
	void AttachPlayerAnimations(int objectID);
	void AttachNPCAnimations(int objectID);
	void AttachCustomersAnimations(int objectID);
	void AttachWorkVfxGrillAnimations(int objectID);
	void AttachWorkVfxCutAnimations(int objectID);
	void AttachWorkVfxStoveAnimations(int objectID);

	// for menu-specific grid animations (6x5 sprite sheet)
	void AttachMenuAnimations(int objectID);

	// Animation control
	void SetAnimation(int objectID, const std::string& animName);
	std::string GetCurrentAnimation(int objectID) const;
	std::vector<std::string> GetAnimationNames(int objectID) const;

	// Query
	bool HasAnimator(int objectID) const;

	void AttachRuntimeAnimation(int objectID,
		const std::vector<glm::vec4>& frames,
		float frameDuration,
		bool loop,
		const std::string& animName = "RUNTIME");

	void RemoveAnimator(int objectID);

private:
	// Reference to EntityManager (set externally)
	EntityManager* entityManager_ = nullptr;

	// Animator storage - each object has ONE animator with multiple named animation sets
	std::unordered_map<int, Animator2D> animators_;  // objectID -> Animator2D

	// Track which animation is active for each object
	std::unordered_map<int, std::string> currentAnimations_;

	// Store all animation sets for each object
	struct AnimationSet {
		std::vector<glm::vec4> frames;
		float frameDuration = 0.0f;
		bool loop = false;
	};
	std::unordered_map<int, std::unordered_map<std::string, AnimationSet>> animationSets_;

	bool isPlaying = false;  // Start paused by default

	// Helpers: Create standard frame sequences
	std::vector<glm::vec4> CreateFrameSequence(int startFrame, int endFrame, int totalFrames);
	std::vector<glm::vec4> CreateFrameSequenceRow(int row, int startCol, int endCol, int totalRows = 15, int totalCols = 8);

	// build entire grid sequence (row-major)
	std::vector<glm::vec4> CreateFullGridSequence(int totalRows, int totalCols);

	// Flip existing frames horizontally (for mirrored animations)
	std::vector<glm::vec4> CreateFlippedFramesX(const std::vector<glm::vec4>& frames);
};
