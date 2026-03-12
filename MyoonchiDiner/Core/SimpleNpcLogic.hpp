/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SimpleNpcLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:		Declares the SimpleNpcLogic script used for basic NPC behaviour. Defines the
					movement states, timing values, and direction flags used to drive simple
					up-down patrolling logic. Inherits from GameObjectLogic.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "FoodTypes.hpp"
#include "GameObjectLogic.hpp"
#include "Math.hpp"

#include <glm/glm.hpp>
#include <iostream>
#include <vector>

 // Forward declarations to avoid circular includes
class SimpleNpcLogic : public GameObjectLogic {
public:
	// Constructor takes owner object ID and desired dish type for this NPC
	using GameObjectLogic::GameObjectLogic;

	// Helper to convert DishType to string for debugging
	static const char* DishTypeName(DishType t) {
		switch (t) {
		case DishType::MeatDish: return "MeatDish";
		case DishType::VegDish:  return "VegDish";
		case DishType::SoupDish: return "SoupDish";
		case DishType::PoopDish: return "PoopDish";
		case DishType::SkewerDish: return "SkewerDish";
		case DishType::CarrotSaladDish: return "CarrotSaladDish";
		default: return "Unknown";
		}
	}

	// Accessors for testing and debugging
	float GetPatienceRatioAtServe() const {
		return patienceRatioAtServe_;
	}

	// GameObjectLogic overrides
	void Awake(Scene& scene) override;
	void Update(float dt, Scene& scene, InputManager& input) override;
	std::string GetName() const override {
		return "SimpleNpcLogic";
	}

	// ===================== Customer behaviour API =====================
	// These are the functions CustomerTableLogic / NPC system can call
	// to drive the "customer" side of this NPC.

	// High-level behaviour states (separate from movement state).
	enum class BehaviourState {
		Idle,
		FindingTable,
		WalkingToTable,
		Ordering,
		WaitingForFood,
		Eating,
		Paying,
		Leaving
	};

	void UpdateNpcAnimation(Scene& scene, GameObject* npc, const glm::vec2& moveDelta);

	// Table assignment -------------------------------------------------
	void AssignCustomerTable(int tableObjectID);  // call when you pick a table
	int  GetCustomerTableID() const {
		return customerTableID_;
	}

	// Call when movement/pathfinding detects NPC has reached their table.
	void OnSeatedAtTable(Scene& scene);

	// Behaviour state access -------------------------------------------
	BehaviourState GetBehaviourState() const {
		return behaviourState_;
	}

	// Convenience queries for specific states (for table logic to decide what interactions are valid)
	bool IsOrdering() const {
		return behaviourState_ == BehaviourState::Ordering;
	}
	bool IsWaitingForFood() const {
		return behaviourState_ == BehaviourState::WaitingForFood;
	}
	bool IsPaying() const {
		return behaviourState_ == BehaviourState::Paying;
	}
	bool IsLeaving() const {
		return behaviourState_ == BehaviourState::Leaving;
	}
	bool IsEating() const {
		return behaviourState_ == BehaviourState::Eating;
	}
	bool IsAtTable(int tableID) const {
		return customerTableID_ == tableID;
	}

	void SetLeaving() {
		behaviourState_ = BehaviourState::Leaving;
		hasPaid_ = true;
	}


	// Interactions from table / player --------------------------------
	// Called when player interacts at the table while this NPC is ORDERING.
	void TakeOrder(Scene& scene);

	// Called when a completed dish is served to this NPC's table.
	void OnDishServed(Scene& scene, DishType dishType);

	// Called when player interacts at the table while this NPC is PAYING.
	void TakePayment(Scene& scene);

	// Status queries ---------------------------------------------------
	bool HasOrderBeenTaken() const {
		return orderTaken_;
	}
	bool HasDishServed()     const {
		return dishServed_;
	}
	bool HasFinishedEating() const {
		return finishedDish_;
	}
	bool HasPaid()           const {
		return hasPaid_;
	}

	DishType GetServedDishType() const {
		return servedDishType_;
	}

	// True when NPC has paid and entered Leaving state (you can despawn).
	bool IsServiceComplete() const {
		return hasPaid_ && behaviourState_ == BehaviourState::Leaving;
	}

	// Assign a customer table and the exact world position where this NPC
	// should sit. If you call this, the NPC will try to walk to that point
	// instead of doing the up/down patrol.
	void SetCustomerTableTarget(int tableObjectID, const Math::Vector2D& seatWorldPos);
	void SetLeaveTarget(const Math::Vector2D& leaveWorldPos);

	// Clear any assigned customer table � NPC will go back to normal patrol.
	void ClearCustomerTableTarget();

	// True if a customer table target is currently assigned (you can call GetCustomerTableID() and GetCustomerSeatTarget() safely).
	bool HasCustomerTableTarget() const {
		return hasCustomerTarget_;
	}

	// Exit gate target
	bool           hasExitGatePos_ = false;
	Math::Vector2D exitGateWorldPos_{ 0.0f, 0.0f };
	float          exitArriveThreshold_ = 8.0f;

	// Call when NPC should start trying to exit (after paying). Caches the exit gate position for pathfinding.
	void CacheExitGatePos(Scene& scene);
	void OnReachedExit(Scene& scene);
	DishType GetDesiredDishType() const {
		return desiredDishType_;
	}

	// ===== Patience =====
	float GetPatienceRemaining() const {
		return patienceRemaining_;
	}
	float GetPatienceMax() const {
		return patienceMax_;
	}

	// Returns 0..1 ratio of patience remaining, clamped to that range. Useful for UI.
	float GetPatienceRatio01() const {
		if (patienceMax_ <= 0.f) return 0.f;
		float r = patienceRemaining_ / patienceMax_;
		if (r < 0.f) r = 0.f;
		if (r > 1.f) r = 1.f;
		return r;
	}

	// True if patience has fully expired (you can check this in your update loop to trigger any consequences like auto-leaving or payment reduction).
	bool HasPatienceExpired() const {
		return patienceExpired_;
	}

	// Unified "will pay $0" for wrong dish OR patience timeout
	bool WillPayZero() const {
		return payZero_;
	}

	// --- Animation facing direction ---
	enum class FacingDir {
		Front, Back, Left, Right
	};
	FacingDir facingDir_ = FacingDir::Front;

	void SetInfinitePatience(bool enabled = true) {
		if (enabled) {
			patienceMax_ = 1000000.0f;
			patienceRemaining_ = patienceMax_;
			patienceExpired_ = false;
			payZero_ = false;
		}
	}

private:
	// Internal movement state for simple up/down patrol when not doing customer behaviour.
	enum class State {
		Idle, MoveUp, MoveDown
	};

	State state = State::Idle;
	float timer = 0.0f;

	float idleDuration = 0.5f;   // pause at top/bottom
	float speed = 200.0f; // pixels per second

	// Which direction we will move next after an Idle
	bool nextMoveUp = false;     // start by moving DOWN

	// ===================== New customer state data ====================
	static constexpr int kInvalidID = -1;

	bool            hasCustomerTarget_ = false;
	int             customerTableID_ = kInvalidID;
	Math::Vector2D  customerSeatTarget_{ 0.0f, 0.0f };
	Math::Vector2D  leaveTargetWorldPos_{ 0.0f, 0.0f };
	bool            hasLeaveTarget_ = false;
	float           arriveThreshold_ = 8.0f; // how close counts as "arrived"

	BehaviourState behaviourState_ = BehaviourState::Idle;

	bool orderTaken_ = false;
	bool dishServed_ = false;
	bool finishedDish_ = false;
	bool hasPaid_ = false;

	DishType desiredDishType_ = DishType::VegDish; // default want salad
	DishType servedDishType_ = DishType::PoopDish;

	// Simple "eating" timer: after dish is served, NPC spends some time
	// in Eating state before switching to Paying.
	float eatTimer_ = 0.0f;
	float eatDuration_ = 10.0f;   // seconds

	// Internal helper to advance the customer eating logic.
	void UpdateCustomerLogic(float dt, Scene& scene);

	bool exitProcessed_ = false;
	bool dishRolled_ = false;
	DishType RollRandomDish(Scene& scene);

	// ===== Customer patience =====
	float patienceMax_ = 45.0f;
	float patienceRemaining_ = 0.0f;
	bool  patienceExpired_ = false;

	// If true, payment should be $0 (wrong dish OR patience timeout)
	bool  payZero_ = false;

	// helper
	void OnPatienceExpired(Scene& scene);
	float patienceRatioAtServe_ = 0.0f; // 0..1 snapshot when correct dish is served
	bool TryGetDeltaToTable(Scene& scene, glm::vec2& outDelta) const;

	void BeginLeaveToExit(Scene& scene, bool freeTableImmediately);

	enum class MoveMode {
		None,
		Direct,
		Pathfinding
	};

	MoveMode moveMode_ = MoveMode::None;

	glm::vec2 moveTarget_{ 0.0f, 0.0f };
	bool hasMoveTarget_ = false;

	std::vector<glm::vec2> pathPoints_;
	std::size_t pathIndex_ = 0;
	glm::vec2 finalTarget_{ 0.0f, 0.0f };

	float directPathCheckTimer_ = 0.0f;
	static constexpr float kDirectPathCheckInterval = 0.05f;

	void ClearNavigationMove();
	void BeginMoveDirect(const glm::vec2& dest);
	void BeginMoveTo(Scene& scene, const glm::vec2& dest);
	void EnsureNavigationPlan(Scene& scene, GameObject* npc, const glm::vec2& desiredTarget);
	bool UpdateNavigationMove(float dt, Scene& scene, GameObject* npc);
};
