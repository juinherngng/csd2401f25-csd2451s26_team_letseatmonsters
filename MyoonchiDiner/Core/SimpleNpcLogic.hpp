/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SimpleNpcLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (60%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu	  (40%)

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

	/**
	 * @brief Performs dish type name.
	 * @param t Parameter for t.
	 * @return Result produced by this operation.
	 */
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

	/**
	 * @brief Returns patience ratio at serve.
	 * @return Requested value.
	 */
	float GetPatienceRatioAtServe() const {
		return patienceRatioAtServe_;
	}

	/**
	 * @brief Performs awake.
	 * @param scene Scene being processed.
	 */
	void Awake(Scene& scene) override;

	/**
	 * @brief Updates this object.
	 * @param dt Frame delta time in seconds.
	 * @param scene Scene being processed.
	 * @param input Input manager for the current frame.
	 */
	void Update(float dt, Scene& scene, InputManager& input) override;

	/**
	 * @brief Returns the stable name for this object.
	 * @return Requested value.
	 */
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

	/**
	 * @brief Updates npc animation.
	 * @param scene Scene being processed.
	 * @param npc Parameter for npc.
	 * @param moveDelta Parameter for move delta.
	 */
	void UpdateNpcAnimation(Scene& scene, GameObject* npc, const glm::vec2& moveDelta);

	/**
	 * @brief Performs assign customer table.
	 * @param tableObjectID Parameter for table object id.
	 */
	void AssignCustomerTable(int tableObjectID);  // call when you pick a table

	/**
	 * @brief Returns customer table id.
	 * @return Requested value.
	 */
	int  GetCustomerTableID() const {
		return customerTableID_;
	}

	/**
	 * @brief Performs on seated at table.
	 * @param scene Scene being processed.
	 */
	void OnSeatedAtTable(Scene& scene);

	/**
	 * @brief Returns behaviour state.
	 * @return Requested value.
	 */
	BehaviourState GetBehaviourState() const {
		return behaviourState_;
	}

	/**
	 * @brief Returns whether ordering.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsOrdering() const {
		return behaviourState_ == BehaviourState::Ordering;
	}

	/**
	 * @brief Returns whether waiting for food.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsWaitingForFood() const {
		return behaviourState_ == BehaviourState::WaitingForFood;
	}

	/**
	 * @brief Returns whether paying.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsPaying() const {
		return behaviourState_ == BehaviourState::Paying;
	}

	/**
	 * @brief Returns whether leaving.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsLeaving() const {
		return behaviourState_ == BehaviourState::Leaving;
	}

	/**
	 * @brief Returns whether eating.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsEating() const {
		return behaviourState_ == BehaviourState::Eating;
	}

	/**
	 * @brief Returns whether at table.
	 * @param tableID Parameter for table id.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsAtTable(int tableID) const {
		return customerTableID_ == tableID;
	}

	/**
	 * @brief Sets leaving.
	 */
	void SetLeaving() {
		behaviourState_ = BehaviourState::Leaving;
		hasPaid_ = true;
	}


	// Interactions from table / player --------------------------------
	/**
	 * @brief Performs take order.
	 * @param scene Scene being processed.
	 */
	void TakeOrder(Scene& scene);

	/**
	 * @brief Performs on dish served.
	 * @param scene Scene being processed.
	 * @param dishType Parameter for dish type.
	 */
	void OnDishServed(Scene& scene, DishType dishType);

	/**
	 * @brief Performs take payment.
	 * @param scene Scene being processed.
	 */
	void TakePayment(Scene& scene);

	/**
	 * @brief Returns whether order been taken.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool HasOrderBeenTaken() const {
		return orderTaken_;
	}

	/**
	 * @brief Returns whether dish served.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool HasDishServed()     const {
		return dishServed_;
	}

	/**
	 * @brief Returns whether finished eating.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool HasFinishedEating() const {
		return finishedDish_;
	}

	/**
	 * @brief Returns whether paid.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool HasPaid()           const {
		return hasPaid_;
	}

	/**
	 * @brief Returns served dish type.
	 * @return Requested value.
	 */
	DishType GetServedDishType() const {
		return servedDishType_;
	}

	/**
	 * @brief Returns whether service complete.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsServiceComplete() const {
		return hasPaid_ && behaviourState_ == BehaviourState::Leaving;
	}

	// Assign a customer table and the exact world position where this NPC
	// should sit. If you call this, the NPC will try to walk to that point
	/**
	 * @brief Sets customer table target.
	 * @param tableObjectID Parameter for table object id.
	 * @param seatWorldPos Parameter for seat world pos.
	 */
	void SetCustomerTableTarget(int tableObjectID, const Math::Vector2D& seatWorldPos);

	/**
	 * @brief Sets leave target.
	 * @param leaveWorldPos Parameter for leave world pos.
	 */
	void SetLeaveTarget(const Math::Vector2D& leaveWorldPos);

	/**
	 * @brief Clears customer table target.
	 */
	void ClearCustomerTableTarget();

	/**
	 * @brief Returns whether customer table target.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool HasCustomerTableTarget() const {
		return hasCustomerTarget_;
	}

	// Exit gate target
	bool           hasExitGatePos_ = false;
	Math::Vector2D exitGateWorldPos_{ 0.0f, 0.0f };
	float          exitArriveThreshold_ = 8.0f;

	/**
	 * @brief Performs cache exit gate pos.
	 * @param scene Scene being processed.
	 */
	void CacheExitGatePos(Scene& scene);

	/**
	 * @brief Performs on reached exit.
	 * @param scene Scene being processed.
	 */
	void OnReachedExit(Scene& scene);

	/**
	 * @brief Returns desired dish type.
	 * @return Requested value.
	 */
	DishType GetDesiredDishType() const {
		return desiredDishType_;
	}

	/**
	 * @brief Returns patience remaining.
	 * @return Requested value.
	 */
	float GetPatienceRemaining() const {
		return patienceRemaining_;
	}

	/**
	 * @brief Returns patience max.
	 * @return Requested value.
	 */
	float GetPatienceMax() const {
		return patienceMax_;
	}

	/**
	 * @brief Returns patience ratio01.
	 * @return Requested value.
	 */
	float GetPatienceRatio01() const {
		if (patienceMax_ <= 0.f) return 0.f;
		float r = patienceRemaining_ / patienceMax_;
		if (r < 0.f) r = 0.f;
		if (r > 1.f) r = 1.f;
		return r;
	}

	/**
	 * @brief Returns whether patience expired.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool HasPatienceExpired() const {
		return patienceExpired_;
	}

	/**
	 * @brief Returns whether willpayzero.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool WillPayZero() const {
		return payZero_;
	}

	// --- Animation facing direction ---
	enum class FacingDir {
		Front, Back, Left, Right
	};
	FacingDir facingDir_ = FacingDir::Front;

	/**
	 * @brief Sets infinite patience.
	 * @param enabled Parameter for enabled.
	 */
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

	/**
	 * @brief Updates customer logic.
	 * @param dt Frame delta time in seconds.
	 * @param scene Scene being processed.
	 */
	void UpdateCustomerLogic(float dt, Scene& scene);

	bool exitProcessed_ = false;
	bool dishRolled_ = false;

	/**
	 * @brief Performs roll random dish.
	 * @param scene Scene being processed.
	 * @return Result produced by this operation.
	 */
	DishType RollRandomDish(Scene& scene);

	// ===== Customer patience =====
	float patienceMax_ = 45.0f;
	float patienceRemaining_ = 0.0f;
	bool  patienceExpired_ = false;

	// If true, payment should be $0 (wrong dish OR patience timeout)
	bool  payZero_ = false;

	/**
	 * @brief Performs on patience expired.
	 * @param scene Scene being processed.
	 */
	void OnPatienceExpired(Scene& scene);
	float patienceRatioAtServe_ = 0.0f; // 0..1 snapshot when correct dish is served

	/**
	 * @brief Attempts to get delta to table.
	 * @param scene Scene being processed.
	 * @param outDelta Output value for out delta.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool TryGetDeltaToTable(Scene& scene, glm::vec2& outDelta) const;

	/**
	 * @brief Begins leave to exit.
	 * @param scene Scene being processed.
	 * @param freeTableImmediately Parameter for free table immediately.
	 */
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

	/**
	 * @brief Clears navigation move.
	 */
	void ClearNavigationMove();

	/**
	 * @brief Begins move direct.
	 * @param dest Parameter for dest.
	 */
	void BeginMoveDirect(const glm::vec2& dest);

	/**
	 * @brief Begins move to.
	 * @param scene Scene being processed.
	 * @param dest Parameter for dest.
	 */
	void BeginMoveTo(Scene& scene, const glm::vec2& dest);

	/**
	 * @brief Performs ensure navigation plan.
	 * @param scene Scene being processed.
	 * @param npc Parameter for npc.
	 * @param desiredTarget Parameter for desired target.
	 */
	void EnsureNavigationPlan(Scene& scene, GameObject* npc, const glm::vec2& desiredTarget);

	/**
	 * @brief Updates navigation move.
	 * @param dt Frame delta time in seconds.
	 * @param scene Scene being processed.
	 * @param npc Parameter for npc.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool UpdateNavigationMove(float dt, Scene& scene, GameObject* npc);
};
