/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu

 DESCRIPTION:		Implements the SimpleNpcLogic behaviour, including timed idle-to-move state
					transitions, vertical patrolling based on authored velocity, walk-area clamping,
					and automatic direction reversal when hitting boundaries.

		 All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

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
    // eatDuration_ already set to 3.0f in the header
    desiredDishType_ = DishType::VegDish;
    servedDishType_ = DishType::PoopDish;
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

            // Once arrived, we can keep them there. If later you want them to
            // go back to patrol, you can call ClearCustomerTableTarget().

            // Notify customer behaviour FSM ONCE.
            OnSeatedAtTable(scene);

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

    UpdateCustomerLogic(dt);
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

    //std::cout << "[SimpleNpcLogic] OnSeatedAtTable, npcID="
    //    << npcID << " state="
    //    << static_cast<int>(behaviourState_) << "\n";

    // When NPC reaches its assigned table, it should start ordering.
    if (behaviourState_ == BehaviourState::FindingTable ||
        behaviourState_ == BehaviourState::WalkingToTable)
    {
        // Start ordering this dish (2 processed veg salad).
        behaviourState_ = BehaviourState::Ordering;
        desiredDishType_ = DishType::VegDish;

        std::cout << "[SimpleNpcLogic] Now ordering dish type = "
            << static_cast<int>(desiredDishType_) << "\n";

        // Auto-take the order so we move into WaitingForFood right away.
        TakeOrder(scene); // This sets behaviourState_ = WaitingForFood
    }
}


void SimpleNpcLogic::TakeOrder(Scene& /*scene*/)
{
    std::cout << "[SimpleNpcLogic] TakeOrder, state="
        << static_cast<int>(behaviourState_) << "\n";

    // Only meaningful if in ORDERING state.
    if (behaviourState_ != BehaviourState::Ordering)
        return;

    orderTaken_ = true;
    behaviourState_ = BehaviourState::WaitingForFood;
}

void SimpleNpcLogic::OnDishServed(Scene& /*scene*/, DishType dishType)
{
    std::cout << "[SimpleNpcLogic] OnDishServed, dishType="
        << static_cast<int>(dishType) << "\n";

    // Only meaningful if actually waiting for food.
    if (behaviourState_ != BehaviourState::WaitingForFood)
        return;

    dishServed_ = true;
    servedDishType_ = dishType;

    behaviourState_ = BehaviourState::Eating;
    eatTimer_ = 0.0f;
}

void SimpleNpcLogic::TakePayment(Scene& /*scene*/)
{
    std::cout << "[SimpleNpcLogic] TakePayment, state="
        << static_cast<int>(behaviourState_) << "\n";

    // Only meaningful if currently paying.
    if (behaviourState_ != BehaviourState::Paying)
        return;

    hasPaid_ = true;
    behaviourState_ = BehaviourState::Leaving;
}

void SimpleNpcLogic::UpdateCustomerLogic(float dt)
{
    if (behaviourState_ == BehaviourState::Eating)
    {
        eatTimer_ += dt;
        std::cout << "SimpleNpcLogic] Eating... timer= " << eatTimer_ << "/" << eatDuration_ << "\n";
        if (eatTimer_ >= eatDuration_) {
            eatTimer_ = eatDuration_;
            finishedDish_ = true;

            std::cout << "[SimpleNpcLogic] Finished eating, switching to Paying\n";

            // Once done eating, NPC is ready to pay.
            behaviourState_ = BehaviourState::Paying;
        }
    }

    // Other behaviour transitions (e.g. auto-leave after paying)
    // can be added here later if you want.
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