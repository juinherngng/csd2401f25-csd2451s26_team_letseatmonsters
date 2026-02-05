/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SimpleNpcLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu

 DESCRIPTION:		Implements the SimpleNpcLogic behaviour, including timed idle-to-move state
					transitions, vertical patrolling based on authored velocity, walk-area clamping,
					and automatic direction reversal when hitting boundaries.

		 All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <random>
#include "../Core/AudioManager.hpp"
#include "../Core/Collision.hpp"  // for WalkArea definition
#include "../Core/Physics.hpp"      // optional, if you want clamp helpers
#include "../Graphics/SceneManager.hpp"

#include "SimpleNpcLogic.hpp"

void SimpleNpcLogic::Awake(Scene& scene) {
    (void)scene;
    timer = 0.0f;
    state = State::Idle;

    // Customer FSM initialisation
    behaviourState_ = BehaviourState::Idle;
    orderTaken_ = false;
    dishServed_ = false;
    finishedDish_ = false;
    hasPaid_ = false;
    eatTimer_ = 0.0f;
    servedDishType_ = DishType::PoopDish;

    patienceRemaining_ = 0.0f;
    patienceExpired_ = false;
    payZero_ = false;
    patienceRatioAtServe_ = 0.0f;

}

void SimpleNpcLogic::Update(float dt, Scene& scene, InputManager&) {
	// IMPORTANT: do not run logic in editor mode
	if (!scene.IsSimulationActive()) {
	    return;
	}

	GameObject* npc = GetOwner(scene);
	if (!npc) return;

    glm::vec3 pos = npc->GetPositionGLM();

    // ===================== CASE 1: HAS CUSTOMER TABLE =====================
    if (hasCustomerTarget_) {
        // Move directly toward the assigned seat position.
        Math::Vector2D curPos(pos.x, pos.y);
        Math::Vector2D target = customerSeatTarget_;

        float dx = target.x - curPos.x;
        float dy = target.y - curPos.y;
        float distSq = dx * dx + dy * dy;
        float thresholdSq = arriveThreshold_ * arriveThreshold_;

        if (distSq <= thresholdSq) {
            // Considered "arrived": snap to target, stop moving.
            curPos = target;
            pos.x = curPos.x;
            pos.y = curPos.y;
            npc->SetPosition(pos);

            // If leaving, despawn instead of ordering
            if (behaviourState_ == BehaviourState::Leaving)
            {
                std::cout << "[SimpleNpcLogic] Arrived at exit. NPC will despawn.\n";
                OnReachedExit(scene);
                return;
            }

            // Notify customer behaviour FSM ONCE.
            OnSeatedAtTable(scene);

            UpdateCustomerLogic(dt, scene);

            return;
        }
        else {
            float dist = std::sqrt(distSq);
            if (dist > 0.0001f) {
                float maxStep = speed * dt;
                float step = (maxStep < dist) ? maxStep : dist;

                // Normalized direction * step
                curPos.x += dx * (step / dist);
                curPos.y += dy * (step / dist);

                pos.x = curPos.x;
                pos.y = curPos.y;
                npc->SetPosition(pos);

                // Optional: still clamp to walk area gates.
                glm::vec3 beforeClamp = pos;
                scene.ClampToWalkArea(npc);
                glm::vec3 afterClamp = npc->GetPositionGLM();
                (void)beforeClamp;
                (void)afterClamp;

                return; // skip patrol logic
            }
        }

        // Fallback: if something weird happens, don't fall through, just return.
        return;
    }

    // ===================== CASE 2: NORMAL PATROL ==========================
    timer += dt;

    // Velocity as authored in the editor / level file
    glm::vec2 vel = scene.GetNPCVelocity(npc->GetID());
    pos = npc->GetPositionGLM();

	// Step 1: compute desired movement based on state
	switch (state) {
		case State::Idle:
		if (timer >= idleDuration) {
			state = nextMoveUp?State::MoveUp:State::MoveDown;
			timer = 0.0f;
		}
		break;

		case State::MoveUp:
		pos.x += vel.x * dt;
		pos.y -= vel.y * dt;
		break;

		case State::MoveDown:
		pos.x += vel.x * dt;
		pos.y += vel.y * dt; // larger y is "down"
		break;
	}

	// Step 2: apply our desired position
	npc->SetPosition(pos);

    // Step 3: clamp to existing walk area / gates
    glm::vec3 beforeClamp = pos;
    scene.ClampToWalkArea(npc);
    glm::vec3 afterClamp = npc->GetPositionGLM();

	// Step 4: detect if we hit a vertical boundary and bounce
	const float eps = 0.1f;
	bool yChangedByClamp = std::fabs(afterClamp.y - beforeClamp.y) > eps;

	if (yChangedByClamp) {
		// We collided with top/bottom. Switch to idle and flip direction.
		if (state == State::MoveUp) {
			nextMoveUp = false; // next time, go down
		}
		else if (state == State::MoveDown) {
			nextMoveUp = true;  // next time, go up
		}

        state = State::Idle;
        timer = 0.0f;
    }

    UpdateCustomerLogic(dt, scene);
}

void SimpleNpcLogic::UpdateCustomerLogic(float dt, Scene& scene)
{
    if (behaviourState_ == BehaviourState::WaitingForFood && orderTaken_ && !dishServed_)
    {
        if (!patienceExpired_) {
            patienceRemaining_ -= dt;
            if (patienceRemaining_ <= 0.f) {
                OnPatienceExpired(scene);
            }
        }
    }

    if (behaviourState_ == BehaviourState::Eating)
    {
        eatTimer_ += dt;
        //std::cout << "SimpleNpcLogic] Eating... timer= " << eatTimer_ << "/" << eatDuration_ << "\n";

        if (eatTimer_ >= eatDuration_) {
            eatTimer_ = eatDuration_;

            if (!finishedDish_)  // <-- make sure it only runs once
            {
                finishedDish_ = true;

                // Clear the food from the table now
                if (customerTableID_ != kInvalidID)
                {
                    LogicManager& logicMgr = scene.GetLogicManager();
                    if (auto* table = logicMgr.GetLogicForObject<CustomerTableLogic>(customerTableID_))
                    {
                        table->ClearServedFood(scene);
                    }
                }

                std::cout << "[SimpleNpcLogic] Finished eating, switching to Paying\n";

                // Once done eating, NPC is ready to pay.
                behaviourState_ = BehaviourState::Paying;

                //TakePayment();
            }
        }
    }

    // Other behaviour transitions (e.g. auto-leave after paying)
    // can be added here later if you want.
}

void SimpleNpcLogic::AssignCustomerTable(int tableObjectID)
{
    customerTableID_ = tableObjectID;

    std::cout << "[SimpleNpcLogic] AssignCustomerTable tableID=" << tableObjectID << "\n";

    // If currently idle as a customer, start looking for the table.
    if (behaviourState_ == BehaviourState::Idle) {
        behaviourState_ = BehaviourState::FindingTable;
    }
}

void SimpleNpcLogic::OnSeatedAtTable(Scene& scene)
{
    int npcID = -1;
    if (GameObject* owner = GetOwner(scene)) {
        npcID = owner->GetID();
    }

    if (!dishRolled_) {
        desiredDishType_ = RollRandomDish();
        dishRolled_ = true;

        std::cout << "[SimpleNpcLogic] Rolled desired dish = "
            << DishTypeName(desiredDishType_)
            << " (" << (int)desiredDishType_ << ")\n";
    }

    //std::cout << "[SimpleNpcLogic] OnSeatedAtTable, npcID="
    //    << npcID << " state="
    //    << static_cast<int>(behaviourState_) << "\n";

    // When NPC reaches its assigned table, it should start ordering.
    if (behaviourState_ == BehaviourState::FindingTable ||
        behaviourState_ == BehaviourState::WalkingToTable)
    {
        // Start ordering this dish (2 processed veg salad).
        behaviourState_ = BehaviourState::Ordering;

        // Auto-take the order so we move into WaitingForFood right away.
        TakeOrder(scene); // This sets behaviourState_ = WaitingForFood
    }
}


void SimpleNpcLogic::TakeOrder(Scene& scene)
{
    std::cout << "[SimpleNpcLogic] TakeOrder, state="
        << static_cast<int>(behaviourState_) << "\n";

    // Only meaningful if in ORDERING state.
    if (behaviourState_ != BehaviourState::Ordering)
        return;

    orderTaken_ = true;
    patienceRatioAtServe_ = 1.0f; // starts full; will be snapshotted on serve

    // Play new order sound effect (release mode only)
#ifndef _DEBUG
    if (AudioManager* audioMgr = scene.GetAudioManager()) {
        audioMgr->PlaySound("sfx_new_order", audioMgr->GetVfxVolume() * 0.3f, false);
    }
#endif

    // Start patience timer now that we are waiting for food
    patienceRemaining_ = patienceMax_;
    patienceExpired_ = false;
    payZero_ = false;

    behaviourState_ = BehaviourState::WaitingForFood;
}

void SimpleNpcLogic::OnDishServed(Scene& scene, DishType dishType)
{
    std::cout << "[SimpleNpcLogic] OnDishServed, dishType=" << static_cast<int>(dishType) << "\n";

    // Only meaningful if actually waiting for food.
    if (behaviourState_ != BehaviourState::WaitingForFood)
        return;

    dishServed_ = true;
    servedDishType_ = dishType;

    // Snapshot patience % at the moment the dish is served (remaining/max)
    if (patienceMax_ > 0.0f) {
        patienceRatioAtServe_ = std::clamp(patienceRemaining_ / patienceMax_, 0.0f, 1.0f);
    }
    else {
        patienceRatioAtServe_ = 0.0f;
    }

    if (servedDishType_ != desiredDishType_)
    {
        // Clear the served food so the table doesn't stay blocked
        if (customerTableID_ != kInvalidID)
        {
            LogicManager& logicMgr = scene.GetLogicManager();
            if (auto* table = logicMgr.GetLogicForObject<CustomerTableLogic>(customerTableID_))
            {
                table->ClearServedFood(scene);
            }
        }

        hasPaid_ = false;
        payZero_ = true;

        // IMPORTANT: do NOT leave yet — wait in Paying so player can "take payment" (which will be $0)
        behaviourState_ = BehaviourState::Paying;

        std::cout << "[SimpleNpcLogic] Wrong dish served. Switching to Paying (will pay $0)\n";
        return;
    }

    payZero_ = false;
    behaviourState_ = BehaviourState::Eating;
    eatTimer_ = 0.0f;
}

void SimpleNpcLogic::TakePayment(Scene& scene)
{
    std::cout << "[SimpleNpcLogic] TakePayment, state="
        << static_cast<int>(behaviourState_) << "\n";

    if (behaviourState_ != BehaviourState::Paying)
        return;

    // Play payment or wrong order sound effect (release mode only)
#ifndef _DEBUG
    if (AudioManager* audioMgr = scene.GetAudioManager()) {
        if (payZero_) {
            // Wrong order or patience expired - play wrong order sound
            audioMgr->PlaySound("sfx_wrong_order", audioMgr->GetVfxVolume() * 0.3f, false);
        } else {
            // Successful order - play payment sound
            audioMgr->PlaySound("sfx_payment", audioMgr->GetVfxVolume() * 0.3f, false);
        }
    }
#endif

    hasPaid_ = true;
    behaviourState_ = BehaviourState::Leaving;

    Math::Vector2D gate = scene.GetExitGateWorldPos();
    hasCustomerTarget_ = true;
    customerSeatTarget_ = gate;

    std::cout << "[SimpleNpcLogic] NPC leaving: heading to exit at ("
        << gate.x << "," << gate.y << ")\n";

    hasCustomerTarget_ = true;
    //customerSeatTarget_ = exitGateWorldPos_;

    // NOTE: do NOT change customerTableID_ here — you still want to know which table to free.
}

void SimpleNpcLogic::SetCustomerTableTarget(int tableObjectID, const Math::Vector2D& seatWorldPos)
{
    customerTableID_ = tableObjectID;
    customerSeatTarget_ = seatWorldPos;
    hasCustomerTarget_ = true;
    behaviourState_ = BehaviourState::WalkingToTable;

    std::cout << "[SimpleNpcLogic] SetCustomerTableTarget tableID=" << tableObjectID
        << " seat=(" << seatWorldPos.x << ", " << seatWorldPos.y << ")\n";

}

void SimpleNpcLogic::ClearCustomerTableTarget()
{
    std::cout << "[SimpleNpcLogic] ClearCustomerTableTarget (was "
        << customerTableID_ << ")\n";

    hasCustomerTarget_ = false;
    customerTableID_ = kInvalidID;
    customerSeatTarget_ = Math::Vector2D(0.0f, 0.0f);
}

void SimpleNpcLogic::CacheExitGatePos(Scene& scene)
{
    if (hasExitGatePos_) return;

    LogicManager& logicMgr = scene.GetLogicManager();

    GameObject* me = GetOwner(scene);
    glm::vec3 myPos = me ? me->GetPositionGLM() : glm::vec3(0.0f);

    bool found = false;
    float bestDistSq = std::numeric_limits<float>::max();

    for (GameObject* obj : scene.GetAllObjectsRaw())
    {
        if (!obj) continue;

        const int id = obj->GetID();

        //identify by logic type
        ExitGateLogic* gate = logicMgr.GetLogicForObject<ExitGateLogic>(id);
        if (!gate) continue;

        Math::Vector2D target = gate->GetExitTargetWorld(scene);

        float dx = target.x - myPos.x;
        float dy = target.y - myPos.y;
        float distSq = dx * dx + dy * dy;

        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            exitGateWorldPos_ = target;
            found = true;
        }
    }

    if (!found) {
        // fallback
        exitGateWorldPos_ = Math::Vector2D(50.0f, 50.0f);
        std::cout << "[SimpleNpcLogic] WARNING: No ExitGateLogic found. Using fallback.\n";
    }
    else {
        std::cout << "[SimpleNpcLogic] Cached exit gate target at ("
            << exitGateWorldPos_.x << ", " << exitGateWorldPos_.y << ")\n";
    }

    hasExitGatePos_ = true;
}


void SimpleNpcLogic::OnReachedExit(Scene& scene)
{
    if (exitProcessed_) return;
    exitProcessed_ = true;
    std::cout << "[SimpleNpcLogic] Reached exit gate. Despawning.\n";

    // Play customer leaving sound effect (release mode only)
#ifndef _DEBUG
    if (AudioManager* audioMgr = scene.GetAudioManager()) {
        audioMgr->PlaySound("sfx_customer_leaving", audioMgr->GetVfxVolume() * 0.3f, false);
    }
#endif

    // Free the customer table
    if (customerTableID_ != kInvalidID)
    {
        LogicManager& logicMgr = scene.GetLogicManager();
        if (CustomerTableLogic* table = logicMgr.GetLogicForObject<CustomerTableLogic>(customerTableID_))
        {
            table->ClearCustomer();
        }
    }

    // Despawn this NPC
    if (GameObject* npc = GetOwner(scene))
    {
        scene.RequestDespawn(npc->GetID());
    }
}

DishType SimpleNpcLogic::RollRandomDish()
{
    // Replace this pool with the dish types you actually support.
    // (Using PoopDish just because it exists in your enum right now.)
    static const DishType kPool[] = {
        DishType::VegDish,
        DishType::MeatDish,
        DishType::SoupDish
        //DishType::PoopDish
    };

    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<int> dist(0, (int)(sizeof(kPool) / sizeof(kPool[0])) - 1);
    return kPool[dist(rng)];
}

void SimpleNpcLogic::OnPatienceExpired(Scene& scene)
{
    if (patienceExpired_) return;

    patienceExpired_ = true;
    patienceRemaining_ = 0.f;
    patienceRatioAtServe_ = 0.0f;

    // Same behavior as wrong dish: go to Paying and pay $0
    payZero_ = true;
    hasPaid_ = false;
    behaviourState_ = BehaviourState::Paying;

    // Optional safety: clear served food if anything got stuck
    if (customerTableID_ != kInvalidID)
    {
        LogicManager& logicMgr = scene.GetLogicManager();
        if (auto* table = logicMgr.GetLogicForObject<CustomerTableLogic>(customerTableID_))
        {
            table->ClearServedFood(scene);
        }
    }

    std::cout << "[SimpleNpcLogic] Patience expired. Switching to Paying (will pay $0)\n";
}

