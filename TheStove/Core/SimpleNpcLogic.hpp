/*
----------------------------------------------------------------------------------------------------
FILE NAME:			SimpleNpcLogic.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Vu Phan Hung, phanhung.vu@digipen.edu

DESCRIPTION:		Declares the SimpleNpcLogic class implementing autonomous NPC movement
                    with idle/walk states, direction switching, and boundary collision
                    reactions for simple vertical patrol behavior.

        All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/
#pragma once
#include "GameObjectLogic.hpp"
#include "FoodTypes.hpp"
#include "Math.hpp"
#include <iostream>

class SimpleNpcLogic : public GameObjectLogic {
public:
    using GameObjectLogic::GameObjectLogic;

    void Awake(Scene& scene) override;
    void Update(float dt, Scene& scene, InputManager& input) override;
    std::string GetName() const override { return "SimpleNpcLogic"; }

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

    // Table assignment -------------------------------------------------
    void AssignCustomerTable(int tableObjectID);  // call when you pick a table
    int  GetCustomerTableID() const { return customerTableID_; }

    // Call when movement/pathfinding detects NPC has reached their table.
    void OnSeatedAtTable(Scene& scene);

    // Behaviour state access -------------------------------------------
    BehaviourState GetBehaviourState() const { return behaviourState_; }

    bool IsOrdering() const { return behaviourState_ == BehaviourState::Ordering; }
    bool IsWaitingForFood() const { return behaviourState_ == BehaviourState::WaitingForFood; }
    bool IsPaying() const { return behaviourState_ == BehaviourState::Paying; }
    bool IsLeaving() const { return behaviourState_ == BehaviourState::Leaving; }

    // Interactions from table / player --------------------------------
    // Called when player interacts at the table while this NPC is ORDERING.
    void TakeOrder(Scene& scene);

    // Called when a completed dish is served to this NPC's table.
    void OnDishServed(Scene& scene, DishType dishType);

    // Called when player interacts at the table while this NPC is PAYING.
    void TakePayment(Scene& scene);

    // Status queries ---------------------------------------------------
    bool HasOrderBeenTaken() const { return orderTaken_; }
    bool HasDishServed()     const { return dishServed_; }
    bool HasFinishedEating() const { return finishedDish_; }
    bool HasPaid()           const { return hasPaid_; }

    DishType GetServedDishType() const { return servedDishType_; }

    // True when NPC has paid and entered Leaving state (you can despawn).
    bool IsServiceComplete() const { return hasPaid_ && behaviourState_ == BehaviourState::Leaving; }

// Assign a customer table and the exact world position where this NPC
// should sit. If you call this, the NPC will try to walk to that point
// instead of doing the up/down patrol.
    void SetCustomerTableTarget(int tableObjectID, const Math::Vector2D& seatWorldPos);

    // Clear any assigned customer table – NPC will go back to normal patrol.
    void ClearCustomerTableTarget();

    bool HasCustomerTableTarget() const { return hasCustomerTarget_; }

private:
    enum class State { Idle, MoveUp, MoveDown };

    State state = State::Idle;
    float timer = 0.0f;

    float idleDuration = 0.5f;   // pause at top/bottom
    float speed = 100.0f; // pixels per second

    // Which direction we will move next after an Idle
    bool nextMoveUp = false;     // start by moving DOWN

    // ===================== New customer state data ====================
    static constexpr int kInvalidID = -1;

    bool            hasCustomerTarget_ = false;
    int             customerTableID_ = kInvalidID;
    Math::Vector2D  customerSeatTarget_{ 0.0f, 0.0f };
    float           arriveThreshold_ = 8.0f; // how close counts as "arrived"

    BehaviourState behaviourState_ = BehaviourState::Idle;

    bool orderTaken_ = false;
    bool dishServed_ = false;
    bool finishedDish_ = false;
    bool hasPaid_ = false;

    DishType servedDishType_ = DishType::PoopDish;

    // Simple "eating" timer: after dish is served, NPC spends some time
    // in Eating state before switching to Paying.
    float eatTimer_ = 0.0f;
    float eatDuration_ = 3.0f;   // seconds

    // Internal helper to advance the customer eating logic.
    void UpdateCustomerLogic(float dt);
};
