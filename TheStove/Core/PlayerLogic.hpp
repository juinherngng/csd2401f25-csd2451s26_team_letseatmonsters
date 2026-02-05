/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (60%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu   (40%)

 DESCRIPTION:		Declares the PlayerLogic script that handles player movement, click-to-move
					navigation, arrival callbacks, item pick-up/drop behaviour, and sprite facing
					updates. Provides the public interface used by the Scene and LogicManager.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "../Core/GameObjectLogic.hpp"

#include "Math.hpp"

#include <glm/glm.hpp>

class PlayerLogic : public GameObjectLogic {
public:
	using GameObjectLogic::GameObjectLogic; // inherit constructor

	void Start(Scene& scene) override;
	void Update(float dt, Scene& scene, InputManager& input) override;
	std::string GetName() const override {
		return "PlayerLogic";
	}

	// --- Carry state helpers ---
	bool IsHolding() const {
		return carriedItemID >= 0;
	}
	int  GetCarriedItemID() const {
		return carriedItemID;
	}

	// --- High-level interaction ---
	// Called when we want the player to interact with a particular table GameObject.
	// (For example: you can call this when the player presses a key near a table.)
	void InteractWithTable(Scene& scene, int tableObjectID);


	// Unity: Move(Vector3 dest)
	void MoveTo(Scene& scene, const glm::vec2& dest);

	// Unity: bool ReachedDestination()
	bool HasDestination() const {
		return hasMoveTarget;
	}
	bool HasArrived()   const {
		return !hasMoveTarget;
	}

	// Unity: PickUp(GameObject item)
	void PickUp(Scene& scene, int itemID);

	// Unity: Drop(Vector3 dropPos) � for now just �drop near player�
	void Drop(Scene& scene);

private:
	// Movement state
	glm::vec2 moveTarget{ 0.f, 0.f };
	bool  hasMoveTarget{ false };
	float moveSpeed{ 220.f };  // pixels/sec

	// Facing / sprite state
	enum class FacingDir {
		Front, Back, Left, Right
	};
	FacingDir facingDir{ FacingDir::Front };

	// Simple �holding� state by object ID
	int carriedItemID{ -1 };
	int pendingTableID = -1;   // table we intend to interact with after moving

	// Offset where the carried item should appear relative to the player
	glm::vec2 carryOffset{ 0.f, -32.f };

	// Store original collider size of the carried item (so we can restore on drop)
	Math::Vector2D carriedItemOriginalColliderSize{ 0.f, 0.f };
	bool hasCarriedItemOriginalColliderSize{ false };

	// Internal helpers
	void HandleClickInput(Scene& scene, InputManager& input); // Unity: input + raycast
	void UpdateMovement(float dt, Scene& scene);              // Unity: NavMeshAgent movement
	void OnArrived(Scene& scene);                             // Unity: OnArrived() hook
	void UpdateSprite(Scene& scene, GameObject* player, const glm::vec2& moveDir);

	// keep carried item following the player
	void UpdateCarriedItemTransform(Scene& scene);

	// particle footsteps
	float footstepDistanceAcc_ = 0.0f;
	bool wasMoving_ = false;
	float footstepEmitTimer_ = 0.0f;

	glm::vec3 lastTrailPos_{ 0.0f, 0.0f, 0.0f };
	bool hasLastTrailPos_ = false;
	float trailCarry_ = 0.0f;
};
