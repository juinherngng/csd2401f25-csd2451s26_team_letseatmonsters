/*
----------------------------------------------------------------------------------------------------
FILE NAME:			AnimationManager.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Seah Wang Hua, wanghua.seah@digipen.edu
CO-AUTHORS: 		Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:	    Implements AnimationManager. Manages 2D sprite animations for game objects,
					updating frame UVs based on Animator2D components. Supports play/pause
					control and registering animation sets for different entity types.

		All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "AnimationManager.hpp"
#include "EntityManager.hpp"
#include "GameObject.hpp"
#include <iostream>
#include <vector>
#include <string>

// ===== SystemInterface Implementation =====

void AnimationManager::Initialize() {
	std::cout << "[AnimationManager] Initialized as system" << std::endl;
}

void AnimationManager::Update(float deltaTime) {
	if (!entityManager_) {
		std::cerr << "[AnimationManager] Warning: EntityManager not set!" << std::endl;
		return;
	}

	// Only update animations if playing
	if (!isPlaying) {
		// Still apply current frame even when paused (so sprites show correct frame)
		for (auto& [objID, animator] : animators_) {
			GameObject* obj = entityManager_->GetByID(objID);
			if (obj) {
				glm::vec4 uvRect = animator.GetCurrentFrameUV();
				obj->SetUVRect(uvRect);
			}
		}
		return;
	}

	// Update animations (advances frames)
	for (auto& [objID, animator] : animators_) {
		animator.Update(deltaTime);

		GameObject* obj = entityManager_->GetByID(objID);
		if (obj) {
			glm::vec4 uvRect = animator.GetCurrentFrameUV();
			obj->SetUVRect(uvRect);
		}
	}
}

std::string AnimationManager::GetName() {
	return "AnimationManager";
}

void AnimationManager::SetEntityManager(EntityManager* entityMgr) {
	entityManager_ = entityMgr;
	std::cout << "[AnimationManager] EntityManager reference set" << std::endl;
}

// ===== Core Functionality =====

void AnimationManager::Clear() {
	animators_.clear();
	currentAnimations_.clear();
	animationSets_.clear();
}

// ===== Animation Registration =====

void AnimationManager::AttachDinoAnimations(int objectID) {
	// Create animator for this object
	Animator2D& anim = animators_[objectID];

	// Dino sprite sheets: 24 frames in a single row
	// Store all animation sets
	animationSets_[objectID]["IDLE"] = { CreateFrameSequence(0, 3, 24), 0.2f, true };
	animationSets_[objectID]["WALK"] = { CreateFrameSequence(4, 9, 24), 0.15f, true };
	animationSets_[objectID]["ATTACK"] = { CreateFrameSequence(10, 14, 24), 0.1f, true };
	animationSets_[objectID]["HURT"] = { CreateFrameSequence(15, 17, 24), 0.1f, true };
	animationSets_[objectID]["DEATH"] = { CreateFrameSequence(18, 23, 24), 0.15f, true };

	// Set default animation (IDLE)
	const auto& idleAnim = animationSets_[objectID]["IDLE"];
	anim.SetFrames(idleAnim.frames, idleAnim.frameDuration, idleAnim.loop);
	currentAnimations_[objectID] = "IDLE";
	anim.Play();

	std::cout << "[AnimationManager] Attached dino animations to object " << objectID << std::endl;
}

void AnimationManager::AttachPlayerAnimations(int objectID) {

	Animator2D& anim = animators_[objectID];

	// Player sprite sheets: 8 columns x 14 rows
	std::vector<glm::vec4> frontIdleFrames = CreateFrameSequenceRow(0, 0, 7, 14, 8);	// row 0 = front
	std::vector<glm::vec4> backIdleFrames = CreateFrameSequenceRow(1, 0, 7, 14, 8);		// row 1 = back
	std::vector<glm::vec4> leftIdleFrames = CreateFrameSequenceRow(2, 0, 7, 14, 8);		// row 2 = left
	std::vector<glm::vec4> rightIdleFrames = CreateFrameSequenceRow(3, 0, 7, 14, 8);	// row 3 = right

	std::vector<glm::vec4> frontWalkFrames = CreateFrameSequenceRow(4, 0, 7, 14, 8);
	std::vector<glm::vec4> backWalkFrames = CreateFrameSequenceRow(5, 0, 7, 14, 8);
	std::vector<glm::vec4> leftWalkFrames = CreateFrameSequenceRow(6, 0, 7, 14, 8);
	std::vector<glm::vec4> rightWalkFrames = CreateFrameSequenceRow(7, 0, 7, 14, 8);

	animationSets_[objectID]["IDLE_FRONT"] = AnimationSet{ frontIdleFrames, 0.15f, true };
	animationSets_[objectID]["IDLE_BACK"] = AnimationSet{ backIdleFrames, 0.15f, true };
	animationSets_[objectID]["IDLE_LEFT"] = AnimationSet{ leftIdleFrames, 0.15f, true };
	animationSets_[objectID]["IDLE_RIGHT"] = AnimationSet{ rightIdleFrames, 0.15f, true };

	animationSets_[objectID]["WALK_FRONT"] = AnimationSet{ frontWalkFrames, 0.15f, true };
	animationSets_[objectID]["WALK_BACK"] = AnimationSet{ backWalkFrames, 0.15f, true };
	animationSets_[objectID]["WALK_LEFT"] = AnimationSet{ leftWalkFrames, 0.15f, true };
	animationSets_[objectID]["WALK_RIGHT"] = AnimationSet{ rightWalkFrames, 0.15f, true };

	const auto& idleAnim = animationSets_[objectID]["IDLE_FRONT"];
	anim.SetFrames(idleAnim.frames, idleAnim.frameDuration, idleAnim.loop);
	currentAnimations_[objectID] = "IDLE_FRONT";
	anim.Play();

	std::cout << "[AnimationManager] Attached player animations to object " << objectID << std::endl;
}

void AnimationManager::AttachNPCAnimations(int objectID) {
	// NPCs use similar setup to player (texture swapping)
	Animator2D& anim = animators_[objectID];

	std::vector<glm::vec4> singleFrame = { glm::vec4(0.f, 0.f, 1.f, 1.f) };

	animationSets_[objectID]["front"] = { singleFrame, 0.1f, true };
	animationSets_[objectID]["back"] = { singleFrame, 0.1f, true };
	animationSets_[objectID]["left"] = { singleFrame, 0.1f, true };
	animationSets_[objectID]["right"] = { singleFrame, 0.1f, true };

	const auto& frontAnim = animationSets_[objectID]["front"];
	anim.SetFrames(frontAnim.frames, frontAnim.frameDuration, frontAnim.loop);
	currentAnimations_[objectID] = "front";
	anim.Play();
}

// ===== Animation Control =====

void AnimationManager::SetAnimation(int objectID, const std::string& animName) {
	auto animIt = animators_.find(objectID);
	if (animIt == animators_.end()) {
		std::cerr << "[AnimationManager] Warning: No animator found for object " << objectID << std::endl;
		return;
	}

	auto setIt = animationSets_.find(objectID);
	if (setIt == animationSets_.end() || setIt->second.find(animName) == setIt->second.end()) {
		std::cerr << "[AnimationManager] Warning: Animation '" << animName << "' not found for object " << objectID << std::endl;
		return;
	}

	// Switch to the new animation
	const AnimationSet& newAnim = setIt->second[animName];
	animIt->second.SetFrames(newAnim.frames, newAnim.frameDuration, newAnim.loop);
	animIt->second.Play();
	currentAnimations_[objectID] = animName;
}

std::string AnimationManager::GetCurrentAnimation(int objectID) const {
	auto it = currentAnimations_.find(objectID);
	if (it != currentAnimations_.end()) {
		return it->second;
	}
	return "";
}

std::vector<std::string> AnimationManager::GetAnimationNames(int objectID) const {
	std::vector<std::string> names;
	auto it = animationSets_.find(objectID);
	if (it == animationSets_.end())
		return names;

	names.reserve(it->second.size());
	for (const auto& kv : it->second) {
		names.push_back(kv.first);
	}

	return names;
}

bool AnimationManager::HasAnimator(int objectID) const {
	return animators_.find(objectID) != animators_.end();
}

// ===== Helper Functions =====

// Only extracts frames from a single row, works only for single row sprite sheets
std::vector<glm::vec4> AnimationManager::CreateFrameSequence(int startFrame, int endFrame, int totalFrames) {
	std::vector<glm::vec4> frames;
	float frameWidth = 1.0f / totalFrames;

	for (int i = startFrame; i <= endFrame; ++i) {
		float u = i * frameWidth;
		frames.push_back(glm::vec4(u, 0.f, frameWidth, 1.f));
	}

	return frames;
}

// Extracts frames from a specific row in a grid-based sprite sheet
std::vector<glm::vec4> AnimationManager::CreateFrameSequenceRow(int row, int startCol, int endCol, int totalRows = 14, int totalCols = 8) {
	std::vector<glm::vec4> frames;
	float frameWidth = 1.0f / totalCols;    // 0.125
	float frameHeight = 1.0f / totalRows;   // ~0.0714
	float v = row * frameHeight;
	for (int i = startCol; i <= endCol; ++i) {
		float u = i * frameWidth;
		frames.push_back(glm::vec4(u, v, frameWidth, frameHeight));
	}
	return frames;
}



// ===== Play/Pause Control =====

void AnimationManager::Play() {
	isPlaying = true;

	// Resume all animators
	for (auto& [objID, animator] : animators_) {
		animator.Play();
	}
}

void AnimationManager::Pause() {
	isPlaying = false;

	// Pause all animators
	for (auto& [objID, animator] : animators_) {
		animator.Pause();
	}
}

void AnimationManager::Stop() {
	isPlaying = false;

	// Stop and reset all animators to first frame
	for (auto& [objID, animator] : animators_) {
		animator.Stop();  // This calls Reset() internally
	}
}

