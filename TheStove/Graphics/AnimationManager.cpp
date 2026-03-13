/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			AnimationManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (90%)
 CO-AUTHORS: 		Ng Juin Herng, juinherng.ng@digipen.edu (10%)

 DESCRIPTION:	    Implements AnimationManager. Manages 2D sprite animations for game objects,
					updating frame UVs based on Animator2D components. Supports play/pause
					control and registering animation sets for different entity types.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "AnimationManager.hpp"
#include "EntityManager.hpp"
#include "GameObject.hpp"

#include <iostream>
#include <string>
#include <vector>

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

	// Player sprite sheets: 8 columns x 17 rows
	std::vector<glm::vec4> backIdleFrames = CreateFrameSequenceRow(16, 0, 7, 17, 8);
	std::vector<glm::vec4> frontIdleFrames = CreateFrameSequenceRow(15, 0, 7, 17, 8);
	std::vector<glm::vec4> leftIdleFrames = CreateFrameSequenceRow(14, 0, 7, 17, 8);
	std::vector<glm::vec4> rightIdleFrames = CreateFrameSequenceRow(13, 0, 7, 17, 8);

	std::vector<glm::vec4> backWalkFrames = CreateFrameSequenceRow(12, 0, 7, 17, 8);
	std::vector<glm::vec4> frontWalkFrames = CreateFrameSequenceRow(11, 0, 7, 17, 8);
	std::vector<glm::vec4> leftWalkFrames = CreateFrameSequenceRow(10, 0, 7, 17, 8);
	std::vector<glm::vec4> rightWalkFrames = CreateFrameSequenceRow(9, 0, 7, 17, 8);

	std::vector<glm::vec4> backCarryFrames = CreateFrameSequenceRow(8, 0, 7, 17, 8);
	std::vector<glm::vec4> leftCarryFrames = CreateFrameSequenceRow(7, 0, 7, 17, 8);
	std::vector<glm::vec4> rightCarryFrames = CreateFrameSequenceRow(6, 0, 7, 17, 8);
	std::vector<glm::vec4> frontCarryFrames = CreateFrameSequenceRow(5, 0, 7, 17, 8);

	std::vector<glm::vec4> choppingFrames = CreateFrameSequenceRow(4, 0, 4, 17, 8);

	std::vector<glm::vec4> leftCarryIdleFrames = CreateFrameSequenceRow(3, 0, 7, 17, 8);
	std::vector<glm::vec4> rightCarryIdleFrames = CreateFrameSequenceRow(2, 0, 7, 17, 8);
	std::vector<glm::vec4> frontCarryIdleFrames = CreateFrameSequenceRow(1, 0, 7, 17, 8);
	std::vector<glm::vec4> backCarryIdleFrames = CreateFrameSequenceRow(0, 0, 7, 17, 8);


	animationSets_[objectID]["IDLE_FRONT"] = AnimationSet{ frontIdleFrames, 0.15f, true };
	animationSets_[objectID]["IDLE_BACK"] = AnimationSet{ backIdleFrames, 0.15f, true };
	animationSets_[objectID]["IDLE_LEFT"] = AnimationSet{ leftIdleFrames, 0.15f, true };
	animationSets_[objectID]["IDLE_RIGHT"] = AnimationSet{ rightIdleFrames, 0.15f, true };

	animationSets_[objectID]["WALK_FRONT"] = AnimationSet{ frontWalkFrames, 0.15f, true };
	animationSets_[objectID]["WALK_BACK"] = AnimationSet{ backWalkFrames, 0.15f, true };
	animationSets_[objectID]["WALK_LEFT"] = AnimationSet{ leftWalkFrames, 0.15f, true };
	animationSets_[objectID]["WALK_RIGHT"] = AnimationSet{ rightWalkFrames, 0.15f, true };

	animationSets_[objectID]["CARRY_BACK"] = AnimationSet{ backCarryFrames, 0.15f, true };
	animationSets_[objectID]["CARRY_LEFT"] = AnimationSet{ leftCarryFrames, 0.15f, true };
	animationSets_[objectID]["CARRY_RIGHT"] = AnimationSet{ rightCarryFrames, 0.15f, true };
	animationSets_[objectID]["CARRY_FRONT"] = AnimationSet{ frontCarryFrames, 0.15f, true };

	animationSets_[objectID]["CHOP"] = AnimationSet{ choppingFrames, 0.05f, true };

	animationSets_[objectID]["IDLE_LEFT_CARRY"] = AnimationSet{ leftCarryIdleFrames, 0.15f, true };
	animationSets_[objectID]["IDLE_RIGHT_CARRY"] = AnimationSet{ rightCarryIdleFrames, 0.15f, true };
	animationSets_[objectID]["IDLE_FRONT_CARRY"] = AnimationSet{ frontCarryIdleFrames, 0.15f, true };
	animationSets_[objectID]["IDLE_BACK_CARRY"] = AnimationSet{ backCarryIdleFrames, 0.15f, true };


	const auto& idleAnim = animationSets_[objectID]["IDLE_FRONT"];
	anim.SetFrames(idleAnim.frames, idleAnim.frameDuration, idleAnim.loop);
	currentAnimations_[objectID] = "IDLE_FRONT";
	anim.Play();

	std::cout << "[AnimationManager] Attached player animations to object " << objectID << std::endl;
}

void AnimationManager::AttachNPCAnimations(int objectID) {
	// NPCs use textutre swapping for now
	Animator2D& anim = animators_[objectID];

	// NPC sprite sheets: 8 columns x 5 rows
	std::vector<glm::vec4> frontIdleFrames = CreateFrameSequenceRow(4, 0, 7, 14, 8);
	std::vector<glm::vec4> leftIdleFrames = CreateFrameSequenceRow(3, 0, 7, 14, 8);
	std::vector<glm::vec4> rightIdleFrames = CreateFrameSequenceRow(2, 0, 7, 14, 8);

	std::vector<glm::vec4> leftWalkFrames = CreateFrameSequenceRow(1, 0, 7, 14, 8);
	std::vector<glm::vec4> rightWalkFrames = CreateFrameSequenceRow(0, 0, 7, 14, 8);

	// Not in use currently
	animationSets_[objectID]["IDLE_FRONT"] = AnimationSet{ frontIdleFrames, 0.15f, true };
	animationSets_[objectID]["IDLE_LEFT"] = AnimationSet{ leftIdleFrames, 0.15f, true };
	animationSets_[objectID]["IDLE_RIGHT"] = AnimationSet{ rightIdleFrames, 0.15f, true };

	animationSets_[objectID]["WALK_LEFT"] = AnimationSet{ leftWalkFrames, 0.15f, true };
	animationSets_[objectID]["WALK_RIGHT"] = AnimationSet{ rightWalkFrames, 0.15f, true };

	// For texture swapping (temporary)
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

namespace {
	struct CustomerAnimProfile {
		int totalRows = 9;
		int totalCols = 8;

		// inclusive end column
		int locomotionEndCol = 7; // idle/walk clips
		int eatEndCol = 4;        // eat clips

		int idleFrontRow = -1;
		int idleBackRow = -1;
		int idleLeftRow = -1;
		int idleRightRow = -1;

		int walkFrontRow = -1;
		int walkBackRow = -1;
		int walkLeftRow = -1;
		int walkRightRow = -1;

		int eatLeftRow = -1;
		int eatRightRow = -1;
	};
	CustomerAnimProfile GetCustomerAnimProfile(const std::string& texturePath) {
		CustomerAnimProfile p{};

		// IMPORTANT:
		// CreateFrameSequenceRow() uses row 0 as the BOTTOM row of the sheet.
		// So if you describe rows from TOP to BOTTOM, we must convert them.

		if (texturePath.find("goat-Sheet") != std::string::npos) {
			// Goat sheet: 9 rows, 8 columns
			p.totalRows = 9;
			p.totalCols = 8;
			p.locomotionEndCol = 7; // 8 frames: 0..7
			p.eatEndCol = 4;        // 5 frames: 0..4

			// Existing goat mapping already appears correct for your engine
			p.idleFrontRow = 8;
			p.idleBackRow = -1; // fallback to front
			p.idleLeftRow = 5;
			p.idleRightRow = 3;

			p.walkFrontRow = 7;
			p.walkBackRow = 6;
			p.walkLeftRow = 1;
			p.walkRightRow = 0;

			p.eatLeftRow = 4;
			p.eatRightRow = 2;
			return p;
		}

		if (texturePath.find("anteater") != std::string::npos) {
			// Anteater:
			// Top-to-bottom order given by you:
			// 1 idle front
			// 2 walk front
			// 3 walk back
			// 4 walk left
			// 5 walk right
			// 6 eat left
			// 7 eat right
			//
			// Engine row 0 = bottom, so convert:
			// top row 1 -> engine row 6
			// top row 2 -> engine row 5
			// ...
			// top row 7 -> engine row 0

			p.totalRows = 7;
			p.totalCols = 8;        // use 7 only if the sheet really has 7 columns
			p.locomotionEndCol = 6; // 7 frames: 0..6
			p.eatEndCol = 4;        // 5 frames: 0..4

			p.idleFrontRow = 6;
			p.idleBackRow = 6;   // use idle front
			p.idleLeftRow = 6;   // use idle front
			p.idleRightRow = 6;  // use idle front

			p.walkFrontRow = 5;
			p.walkBackRow = 4;
			p.walkLeftRow = 3;
			p.walkRightRow = 2;

			p.eatLeftRow = 1;
			p.eatRightRow = 0;
			return p;
		}

		if (texturePath.find("tiger-Sheet") != std::string::npos) {
			// Tiger:
			// Top-to-bottom order given by you:
			// 1 idle front
			// 2 walk front
			// 3 walk back
			// 4 walk left
			// 5 eat left
			// 6 walk right
			// 7 eat right

			p.totalRows = 7;
			p.totalCols = 8;        // use 7 only if the sheet really has 7 columns
			p.locomotionEndCol = 6; // 7 frames: 0..6
			p.eatEndCol = 4;        // 5 frames: 0..4

			p.idleFrontRow = 6;
			p.idleBackRow = 6;   // use idle front
			p.idleLeftRow = 6;   // use idle front
			p.idleRightRow = 6;  // use idle front

			p.walkFrontRow = 5;
			p.walkBackRow = 4;
			p.walkLeftRow = 3;
			p.walkRightRow = 1;

			p.eatLeftRow = 2;
			p.eatRightRow = 0;
			return p;
		}

		// default fallback
		return GetCustomerAnimProfile("../assets/goat-Sheet.png");
	}
}

void AnimationManager::AttachCustomersAnimations(int objectID, const std::string& texturePath) {
	Animator2D& anim = animators_[objectID];
	const CustomerAnimProfile profile = GetCustomerAnimProfile(texturePath);

	auto makeRow = [&](int row, int startCol, int endCol) -> std::vector<glm::vec4> {
		if (row < 0) {
			return {};
		}
		return CreateFrameSequenceRow(row, startCol, endCol, profile.totalRows, profile.totalCols);
		};

	auto firstNonEmpty = [](const std::vector<glm::vec4>& a,
		const std::vector<glm::vec4>& b,
		const std::vector<glm::vec4>& c = {},
		const std::vector<glm::vec4>& d = {}) -> std::vector<glm::vec4> {
			if (!a.empty()) return a;
			if (!b.empty()) return b;
			if (!c.empty()) return c;
			return d;
		};

	auto idleFrontRaw = makeRow(profile.idleFrontRow, 0, profile.locomotionEndCol);
	auto idleBackRaw = makeRow(profile.idleBackRow, 0, profile.locomotionEndCol);
	auto idleLeftRaw = makeRow(profile.idleLeftRow, 0, profile.locomotionEndCol);
	auto idleRightRaw = makeRow(profile.idleRightRow, 0, profile.locomotionEndCol);

	auto walkFrontRaw = makeRow(profile.walkFrontRow, 0, profile.locomotionEndCol);
	auto walkBackRaw = makeRow(profile.walkBackRow, 0, profile.locomotionEndCol);
	auto walkLeftRaw = makeRow(profile.walkLeftRow, 0, profile.locomotionEndCol);
	auto walkRightRaw = makeRow(profile.walkRightRow, 0, profile.locomotionEndCol);

	auto eatLeftRaw = makeRow(profile.eatLeftRow, 0, profile.eatEndCol);
	auto eatRightRaw = makeRow(profile.eatRightRow, 0, profile.eatEndCol);

	// Back idle fallback
	auto idleBack = firstNonEmpty(idleBackRaw, idleFrontRaw);

	// Left/right fallbacks
	auto idleLeft = firstNonEmpty(idleLeftRaw,
		!idleRightRaw.empty() ? CreateFlippedFramesX(idleRightRaw) : std::vector<glm::vec4>{},
		idleFrontRaw,
		idleBack);

	auto idleRight = firstNonEmpty(idleRightRaw,
		!idleLeftRaw.empty() ? CreateFlippedFramesX(idleLeftRaw) : std::vector<glm::vec4>{},
		idleFrontRaw,
		idleBack);

	auto walkLeft = firstNonEmpty(walkLeftRaw,
		!walkRightRaw.empty() ? CreateFlippedFramesX(walkRightRaw) : std::vector<glm::vec4>{},
		walkFrontRaw,
		walkBackRaw);

	auto walkRight = firstNonEmpty(walkRightRaw,
		!walkLeftRaw.empty() ? CreateFlippedFramesX(walkLeftRaw) : std::vector<glm::vec4>{},
		walkFrontRaw,
		walkBackRaw);

	auto eatLeft = firstNonEmpty(eatLeftRaw,
		!eatRightRaw.empty() ? CreateFlippedFramesX(eatRightRaw) : std::vector<glm::vec4>{},
		idleLeft,
		idleFrontRaw);

	auto eatRight = firstNonEmpty(eatRightRaw,
		!eatLeftRaw.empty() ? CreateFlippedFramesX(eatLeftRaw) : std::vector<glm::vec4>{},
		idleRight,
		idleFrontRaw);

	const float idleDur = 0.15f;
	const float walkDur = 0.12f;
	const float eatDur = 0.15f;

	animationSets_[objectID]["IDLE_FRONT"] = { idleFrontRaw, idleDur, true };
	animationSets_[objectID]["IDLE_BACK"] = { idleBack,     idleDur, true };
	animationSets_[objectID]["IDLE_LEFT"] = { idleLeft,     idleDur, true };
	animationSets_[objectID]["IDLE_RIGHT"] = { idleRight,    idleDur, true };

	animationSets_[objectID]["WALK_FRONT"] = { walkFrontRaw, walkDur, true };
	animationSets_[objectID]["WALK_BACK"] = { walkBackRaw,  walkDur, true };
	animationSets_[objectID]["WALK_LEFT"] = { walkLeft,     walkDur, true };
	animationSets_[objectID]["WALK_RIGHT"] = { walkRight,    walkDur, true };

	animationSets_[objectID]["EAT_LEFT"] = { eatLeft,      eatDur,  true };
	animationSets_[objectID]["EAT_RIGHT"] = { eatRight,     eatDur,  true };

	const auto& clip = animationSets_[objectID]["IDLE_FRONT"];
	anim.SetFrames(clip.frames, clip.frameDuration, clip.loop);
	currentAnimations_[objectID] = "IDLE_FRONT";
	anim.Play();
}

void AnimationManager::AttachWorkVfxCutAnimations(int objectID) {
	Animator2D& anim = animators_[objectID];

	auto frames = CreateFrameSequenceRow(1, 0, 3, 6, 6);
	animationSets_[objectID]["LOOP"] = { frames, 0.08f, true };

	const auto& clip = animationSets_[objectID]["LOOP"];
	anim.SetFrames(clip.frames, clip.frameDuration, clip.loop);
	currentAnimations_[objectID] = "LOOP";
	anim.Play();
}

void AnimationManager::AttachWorkVfxGrillAnimations(int objectID) {
	Animator2D& anim = animators_[objectID];

	auto frames = CreateFrameSequenceRow(4, 0, 3, 6, 6);
	animationSets_[objectID]["LOOP"] = { frames, 0.08f, true };

	const auto& clip = animationSets_[objectID]["LOOP"];
	anim.SetFrames(clip.frames, clip.frameDuration, clip.loop);
	currentAnimations_[objectID] = "LOOP";
	anim.Play();
}

void AnimationManager::AttachWorkVfxStoveAnimations(int objectID) {
	Animator2D& anim = animators_[objectID];

	auto frames = CreateFrameSequenceRow(5, 0, 3, 6, 6);
	animationSets_[objectID]["LOOP"] = { frames, 0.08f, true };

	const auto& clip = animationSets_[objectID]["LOOP"];
	anim.SetFrames(clip.frames, clip.frameDuration, clip.loop);
	currentAnimations_[objectID] = "LOOP";
	anim.Play();
}

std::vector<glm::vec4> AnimationManager::CreateFullGridSequence(int totalRows, int totalCols) {
	std::vector<glm::vec4> frames;
	frames.reserve(static_cast<size_t>(totalRows * totalCols));
	float frameWidth = 1.0f / static_cast<float>(totalCols);
	float frameHeight = 1.0f / static_cast<float>(totalRows);

	for (int row = 0; row < totalRows; ++row) {
		float v = row * frameHeight;
		for (int col = 0; col < totalCols; ++col) {
			float u = col * frameWidth;
			frames.push_back(glm::vec4(u, v, frameWidth, frameHeight));
		}
	}
	return frames;
}

// Generates a full grid sequence with controllable row/column directions.
// topFirst: true = start at the top row of the image; false = bottom row first.
// leftToRight: true = increase column index left→right; false = right→left.
static std::vector<glm::vec4> CreateFullGridSequenceDir(int totalRows, int totalCols, bool topFirst, bool leftToRight) {
	std::vector<glm::vec4> frames;
	frames.reserve(static_cast<size_t>(totalRows * totalCols));

	const float frameWidth = 1.0f / static_cast<float>(totalCols);
	const float frameHeight = 1.0f / static_cast<float>(totalRows);

	// Map a logical row index to UV v based on desired direction.
	auto rowToV = [&](int logicalRow) -> float {
		// OpenGL v=0 at bottom; image top row should map to v = (totalRows-1) * frameHeight
		int actualRow = topFirst ? (totalRows - 1 - logicalRow) : logicalRow;
		return actualRow * frameHeight;
		};

	for (int r = 0; r < totalRows; ++r) {
		const float v = rowToV(r);

		if (leftToRight) {
			for (int c = 0; c < totalCols; ++c) {
				const float u = c * frameWidth;
				frames.push_back(glm::vec4(u, v, frameWidth, frameHeight));
			}
		}
		else {
			for (int c = totalCols - 1; c >= 0; --c) {
				const float u = static_cast<float>(c) * frameWidth;
				frames.push_back(glm::vec4(u, v, frameWidth, frameHeight));
			}
		}
	}

	return frames;
}

void AnimationManager::AttachMenuAnimations(int objectID) {
	Animator2D& anim = animators_[objectID];

	const int totalRows = 5;
	const int totalCols = 6;

	// Top row first, left-to-right across each row
	std::vector<glm::vec4> fullFrames = CreateFullGridSequenceDir(totalRows, totalCols, /*topFirst=*/true, /*leftToRight=*/true);

	animationSets_[objectID]["FULL"] = AnimationSet{ fullFrames, 0.04f, true };

	const auto& clip = animationSets_[objectID]["FULL"];
	anim.SetFrames(clip.frames, clip.frameDuration, clip.loop);
	currentAnimations_[objectID] = "FULL";
	anim.Play();

	std::cout << "[AnimationManager] Attached 6x5 full-sheet menu animation to object " << objectID << std::endl;
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
std::vector<glm::vec4> AnimationManager::CreateFrameSequenceRow(int row, int startCol, int endCol, int totalRows, int totalCols) {
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

// Flips frames horizontally
std::vector<glm::vec4> AnimationManager::CreateFlippedFramesX(const std::vector<glm::vec4>& frames) {
	std::vector<glm::vec4> flipped;
	flipped.reserve(frames.size());

	for (const auto& frame : frames) {
		const float flippedU = frame.x + frame.z;  // move to right edge
		const float flippedW = -frame.z;          // negative width = mirror
		flipped.push_back(glm::vec4(flippedU, frame.y, flippedW, frame.w));
	}

	return flipped;
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

void AnimationManager::AttachRuntimeAnimation(int objectID,
	const std::vector<glm::vec4>& frames,
	float frameDuration,
	bool loop,
	const std::string& animName) {
	if (frames.empty()) {
		std::cerr << "[AnimationManager] AttachRuntimeAnimation failed: no frames for object "
			<< objectID << std::endl;
		return;
	}

	Animator2D& anim = animators_[objectID];

	animationSets_[objectID].clear();
	animationSets_[objectID][animName] = AnimationSet{ frames, frameDuration, loop };

	anim.SetFrames(frames, frameDuration, loop);
	currentAnimations_[objectID] = animName;

	// Keep manager state consistent
	if (isPlaying) {
		anim.Play();
	}
	else {
		anim.Pause();
	}
}

void AnimationManager::RemoveAnimator(int objectID) {
	animators_.erase(objectID);
	currentAnimations_.erase(objectID);
	animationSets_.erase(objectID);
}