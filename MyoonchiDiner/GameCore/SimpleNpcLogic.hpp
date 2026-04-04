/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SimpleNpcLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (60%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (40%)

 DESCRIPTION:		Declares the SimpleNpcLogic script used for basic NPC behaviour. Defines the
					movement states, timing values, and direction flags used to drive simple
					up-down patrolling logic. Inherits from GameObjectLogic.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <glm/glm.hpp>
#include <iostream>
#include <vector>

#include "EngineCore/GameObjectLogic.hpp"
#include "EngineCore/Math.hpp"
#include "GameCore/FoodTypes.hpp"

 // Forward declarations to avoid circular includes
class SimpleNpcLogic : public GameObjectLogic {
public:
	using GameObjectLogic::GameObjectLogic;

	/**
	 * @brief Returns a readable string name for a dish type.
	 * @param t Dish type to convert into text.
	 * @return Static string name for the supplied dish type.
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
	 * @brief Returns the patience ratio captured when the correct dish was served.
	 * @return Snapshot of the customer's patience ratio at serve time.
	 */
	float GetPatienceRatioAtServe() const {
		// Expose the serve-time patience snapshot for payment calculation and UI feedback.
		return patienceRatioAtServe_;
	}

	/**
	 * @brief Initializes the NPC after the owning scene is ready.
	 * @param scene Active scene containing the NPC object.
	 */
	void Awake(Scene& scene) override;

	/**
	 * @brief Updates NPC movement, customer behavior, and animation for one frame.
	 * @param dt Delta time for the frame.
	 * @param scene Active scene containing the NPC and gameplay systems.
	 * @param input Input manager forwarded by the logic system.
	 */
	void Update(float dt, Scene& scene, InputManager& input) override;

	/**
	 * @brief Returns the stable runtime logic name used by the engine.
	 * @return Name string for this logic component.
	 */
	std::string GetName() const override {
		// Keep the logic name stable for debugging and runtime registration.
		return "SimpleNpcLogic";
	}

	// High-level customer behaviour states used by service, movement, and UI systems.
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
	 * @brief Updates the NPC's locomotion or eating animation from its movement delta.
	 * @param scene Active scene containing the NPC animation data.
	 * @param npc NPC object whose animation should be updated.
	 * @param moveDelta Movement delta used to infer facing and locomotion state.
	 */
	void UpdateNpcAnimation(Scene& scene, GameObject* npc, const glm::vec2& moveDelta);

	/**
	 * @brief Assigns a customer table to this NPC without a seat target.
	 * @param tableObjectID Runtime ID of the assigned customer table.
	 */
	void AssignCustomerTable(int tableObjectID);

	/**
	 * @brief Returns the currently assigned customer table ID.
	 * @return Assigned customer table ID, or an invalid ID when none is assigned.
	 */
	int  GetCustomerTableID() const {
		// Expose the currently assigned table so table-side systems can verify ownership.
		return customerTableID_;
	}

	/**
	 * @brief Transitions the NPC into its seated-at-table state.
	 * @param scene Active scene containing the NPC object.
	 */
	void OnSeatedAtTable(Scene& scene);

	/**
	 * @brief Returns the NPC's current high-level customer behaviour state.
	 * @return Current behaviour state.
	 */
	BehaviourState GetBehaviourState() const {
		// Surface the live customer state for UI, tables, and manager systems.
		return behaviourState_;
	}

	/**
	 * @brief Returns whether the NPC is currently in the ordering state.
	 * @return True when the NPC is waiting for the player to take its order.
	 */
	bool IsOrdering() const {
		// Ordering state means the customer is seated and ready to place an order.
		return behaviourState_ == BehaviourState::Ordering;
	}

	/**
	 * @brief Returns whether the NPC is currently waiting for food.
	 * @return True when the NPC has ordered and is still awaiting a dish.
	 */
	bool IsWaitingForFood() const {
		// Waiting-for-food is the main state where patience drains over time.
		return behaviourState_ == BehaviourState::WaitingForFood;
	}

	/**
	 * @brief Returns whether the NPC is currently ready to pay.
	 * @return True when the NPC is waiting for payment collection.
	 */
	bool IsPaying() const {
		// Paying state starts after eating finishes or after the service flow reaches payment.
		return behaviourState_ == BehaviourState::Paying;
	}

	/**
	 * @brief Returns whether the NPC is currently leaving the scene.
	 * @return True when the NPC is walking toward its leave target or exit gate.
	 */
	bool IsLeaving() const {
		// Leaving state means the service loop is over and the NPC is exiting.
		return behaviourState_ == BehaviourState::Leaving;
	}

	/**
	 * @brief Returns whether the NPC is currently eating.
	 * @return True when the NPC is consuming its served dish.
	 */
	bool IsEating() const {
		// Eating state plays after a valid dish is served and before payment begins.
		return behaviourState_ == BehaviourState::Eating;
	}

	/**
	 * @brief Returns whether this NPC belongs to a specific customer table.
	 * @param tableID Runtime ID of the table being checked.
	 * @return True when the supplied table matches the NPC's assigned customer table.
	 */
	bool IsAtTable(int tableID) const {
		// Compare against the assigned customer table so callers can verify ownership quickly.
		return customerTableID_ == tableID;
	}

	/**
	 * @brief Forces the NPC into the leaving state and marks payment as complete.
	 */
	void SetLeaving() {
		// Use this helper when an external system wants to terminate service and send the NPC away.
		behaviourState_ = BehaviourState::Leaving;
		hasPaid_ = true;
	}


	/**
	 * @brief Marks the NPC's order as taken and advances to the waiting-for-food state.
	 * @param scene Active scene containing the NPC object.
	 */
	void TakeOrder(Scene& scene);

	/**
	 * @brief Processes a served dish and advances the NPC into the correct follow-up state.
	 * @param scene Active scene containing the NPC object.
	 * @param dishType Dish type that was served to the NPC.
	 */
	void OnDishServed(Scene& scene, DishType dishType);

	/**
	 * @brief Marks payment as collected and advances the NPC toward leaving.
	 * @param scene Active scene containing the NPC object.
	 */
	void TakePayment(Scene& scene);

	/**
	 * @brief Returns whether the NPC's order has already been taken.
	 * @return True when the player has taken this NPC's order.
	 */
	bool HasOrderBeenTaken() const {
		// Expose order progress so service logic can avoid retaking the same order.
		return orderTaken_;
	}

	/**
	 * @brief Returns whether a dish has already been served to the NPC.
	 * @return True when the service flow has recorded a served dish.
	 */
	bool HasDishServed()     const {
		// Expose dish-service progress so tables and UI can react accordingly.
		return dishServed_;
	}

	/**
	 * @brief Returns whether the NPC has finished eating its dish.
	 * @return True when the eating timer has completed.
	 */
	bool HasFinishedEating() const {
		// Finished-eating state controls the transition from eating to payment.
		return finishedDish_;
	}

	/**
	 * @brief Returns whether payment has already been collected from the NPC.
	 * @return True when the NPC has completed its payment step.
	 */
	bool HasPaid()           const {
		// Payment completion is used to determine whether service is fully resolved.
		return hasPaid_;
	}

	/**
	 * @brief Returns the dish type that was served to the NPC.
	 * @return Served dish type recorded during service.
	 */
	DishType GetServedDishType() const {
		// Expose the served dish so payment and scoring can compare it against the desired order.
		return servedDishType_;
	}

	/**
	 * @brief Returns whether the NPC has fully completed its service loop.
	 * @return True when payment is complete and the NPC is in the leaving state.
	 */
	bool IsServiceComplete() const {
		// Treat payment plus the leaving transition as the end of the customer service flow.
		return hasPaid_ && behaviourState_ == BehaviourState::Leaving;
	}

	/**
	 * @brief Assigns both a customer table and the exact seat position this NPC should reach.
	 * @param tableObjectID Runtime ID of the assigned customer table.
	 * @param seatWorldPos World-space seat position the NPC should walk toward.
	 */
	void SetCustomerTableTarget(int tableObjectID, const Math::Vector2D& seatWorldPos);

	/**
	 * @brief Sets the world-space point this NPC should walk toward when leaving.
	 * @param leaveWorldPos World-space leave target for the NPC.
	 */
	void SetLeaveTarget(const Math::Vector2D& leaveWorldPos);

	/**
	 * @brief Clears the current customer-table target and seat assignment.
	 */
	void ClearCustomerTableTarget();

	/**
	 * @brief Returns whether the NPC currently has a valid customer-table target.
	 * @return True when a table/seat target has been assigned.
	 */
	bool HasCustomerTableTarget() const {
		// Expose whether the customer has an active seat target for movement and manager logic.
		return hasCustomerTarget_;
	}

	// Exit gate target
	bool           hasExitGatePos_ = false;
	Math::Vector2D exitGateWorldPos_{ 0.0f, 0.0f };
	float          exitArriveThreshold_ = 8.0f;

	/**
	 * @brief Resolves and caches the current exit-gate world position.
	 * @param scene Active scene containing the exit gate.
	 */
	void CacheExitGatePos(Scene& scene);

	/**
	 * @brief Handles NPC cleanup after it reaches the exit gate.
	 * @param scene Active scene containing the NPC object.
	 */
	void OnReachedExit(Scene& scene);

	/**
	 * @brief Returns the dish type currently desired by this NPC.
	 * @return Desired dish type for the order.
	 */
	DishType GetDesiredDishType() const {
		// Expose the desired dish so UI and service logic can display and validate the order.
		return desiredDishType_;
	}

	/**
	 * @brief Returns the remaining patience time for this NPC.
	 * @return Remaining patience in seconds.
	 */
	float GetPatienceRemaining() const {
		// Surface the live patience timer for UI and service-resolution systems.
		return patienceRemaining_;
	}

	/**
	 * @brief Returns the maximum patience time configured for this NPC.
	 * @return Maximum patience in seconds.
	 */
	float GetPatienceMax() const {
		// Expose the configured patience cap so callers can normalize patience state.
		return patienceMax_;
	}

	/**
	 * @brief Returns the current patience ratio normalized to the range `[0, 1]`.
	 * @return Normalized patience ratio.
	 */
	float GetPatienceRatio01() const {
		// Clamp to the normalized range so callers always receive a stable ratio.
		if (patienceMax_ <= 0.f) return 0.f;
		float r = patienceRemaining_ / patienceMax_;
		if (r < 0.f) r = 0.f;
		if (r > 1.f) r = 1.f;
		return r;
	}

	/**
	 * @brief Returns whether this NPC's patience has run out.
	 * @return True when patience has expired.
	 */
	bool HasPatienceExpired() const {
		// Expose the expired flag so service logic can react without recomputing patience state.
		return patienceExpired_;
	}

	/**
	 * @brief Returns whether this NPC should pay zero because service failed.
	 * @return True when the NPC will not award any payment.
	 */
	bool WillPayZero() const {
		// Zero-payment status is determined by wrong dishes or patience timeout.
		return payZero_;
	}

	// Current facing direction used to choose the NPC's animation set.
	enum class FacingDir {
		Front, Back, Left, Right
	};
	FacingDir facingDir_ = FacingDir::Front;

	/**
	 * @brief Enables or disables effectively infinite patience for this NPC.
	 * @param enabled True to give the NPC effectively unlimited patience.
	 */
	void SetInfinitePatience(bool enabled = true) {
		// Promote patience to a very large value so scripted scenes can opt out of timeout pressure.
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
	 * @brief Advances the NPC's customer-specific behaviour state for one frame.
	 * @param dt Delta time for the frame.
	 * @param scene Active scene containing the NPC object.
	 */
	void UpdateCustomerLogic(float dt, Scene& scene);

	bool exitProcessed_ = false;
	bool dishRolled_ = false;

	/**
	 * @brief Chooses a random desired dish for this NPC.
	 * @param scene Active scene containing the NPC and RNG dependencies.
	 * @return Randomly selected desired dish type.
	 */
	DishType RollRandomDish(Scene& scene);

	// ===== Customer patience =====
	float patienceMax_ = 45.0f;
	float patienceRemaining_ = 0.0f;
	bool  patienceExpired_ = false;

	// If true, payment should be $0 (wrong dish OR patience timeout)
	bool  payZero_ = false;

	/**
	 * @brief Handles the transition that occurs when this NPC's patience expires.
	 * @param scene Active scene containing the NPC object.
	 */
	void OnPatienceExpired(Scene& scene);
	float patienceRatioAtServe_ = 0.0f; // 0..1 snapshot when correct dish is served

	/**
	 * @brief Computes the current world-space offset from the NPC to its assigned table.
	 * @param scene Active scene containing the NPC and table.
	 * @param outDelta Output delta from the NPC toward the table.
	 * @return True when the NPC has a valid table and the delta could be computed.
	 */
	bool TryGetDeltaToTable(Scene& scene, glm::vec2& outDelta) const;

	/**
	 * @brief Starts the leave-to-exit flow for this NPC.
	 * @param scene Active scene containing the NPC and table.
	 * @param freeTableImmediately True to free the table assignment immediately.
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
	 * @brief Clears the current navigation move and any pending path state.
	 */
	void ClearNavigationMove();

	/**
	 * @brief Starts direct movement toward a world-space destination.
	 * @param dest Destination to move toward without pathfinding.
	 */
	void BeginMoveDirect(const glm::vec2& dest);

	/**
	 * @brief Starts movement toward a world-space destination, using navigation when needed.
	 * @param scene Active scene containing navigation data.
	 * @param dest Final world-space destination.
	 */
	void BeginMoveTo(Scene& scene, const glm::vec2& dest);

	/**
	 * @brief Ensures a navigation plan exists from the NPC to the desired destination.
	 * @param scene Active scene containing navigation data.
	 * @param npc NPC object whose navigation should be planned.
	 * @param desiredTarget Desired final world-space target.
	 */
	void EnsureNavigationPlan(Scene& scene, GameObject* npc, const glm::vec2& desiredTarget);

	/**
	 * @brief Advances the current navigation move for one frame.
	 * @param dt Delta time for the frame.
	 * @param scene Active scene containing navigation data.
	 * @param npc NPC object being moved.
	 * @return True while navigation is still active after this update.
	 */
	bool UpdateNavigationMove(float dt, Scene& scene, GameObject* npc);
};
