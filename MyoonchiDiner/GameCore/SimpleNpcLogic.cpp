/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SimpleNpcLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:		Implements the SimpleNpcLogic behaviour, including timed idle-to-move state
					transitions, vertical patrolling based on authored velocity, walk-area clamping,
					and automatic direction reversal when hitting boundaries.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <random>

#include "EngineCore/AudioManager.hpp"
#include "EngineCore/Collision.hpp" // for WalkArea definition
#include "EngineCore/EngineRng.hpp"
#include "EngineCore/Logger.hpp"
#include "EngineCore/Physics.hpp" // optional, if you want clamp helpers
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/CustomerTableLogic.hpp"
#include "GameCore/ExitGateLogic.hpp"
#include "GameCore/SimpleNpcLogic.hpp"

namespace {
	void PlaySpatialSfxAtNpc(Scene& scene, int npcID, const std::string& soundName, float volume, float minDistance = 120.0f, float maxDistance = 1100.0f) {
		AudioManager* audioMgr = scene.GetAudioManager();
		if (!audioMgr || !audioMgr->HasSound(soundName)) {
			return;
		}

		if (GameObject* npc = scene.GetGameObjectByID(npcID)) {
			const glm::vec3 pos = npc->GetPositionGLM();
			audioMgr->PlaySound3D(soundName, pos.x, pos.y, 0.0f, volume, minDistance, maxDistance, false);
			return;
		}

		audioMgr->PlaySound(soundName, volume, false);
	}
}

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

	moveMode_ = MoveMode::None;
	moveTarget_ = glm::vec2(0.0f, 0.0f);
	hasMoveTarget_ = false;
	pathPoints_.clear();
	pathIndex_ = 0;
	finalTarget_ = glm::vec2(0.0f, 0.0f);
	directPathCheckTimer_ = 0.0f;

	leaveTargetWorldPos_ = Math::Vector2D(0.0f, 0.0f);
	hasLeaveTarget_ = false;
}

void SimpleNpcLogic::ClearNavigationMove() {
	moveMode_ = MoveMode::None;
	moveTarget_ = glm::vec2(0.0f, 0.0f);
	hasMoveTarget_ = false;
	pathPoints_.clear();
	pathIndex_ = 0;
	finalTarget_ = glm::vec2(0.0f, 0.0f);
	directPathCheckTimer_ = 0.0f;
}

void SimpleNpcLogic::BeginMoveDirect(const glm::vec2& dest) {
	finalTarget_ = dest;
	moveTarget_ = dest;
	pathPoints_.clear();
	pathIndex_ = 0;
	hasMoveTarget_ = true;
	moveMode_ = MoveMode::Direct;
	directPathCheckTimer_ = 0.0f;
}

void SimpleNpcLogic::BeginMoveTo(Scene& scene, const glm::vec2& dest) {
	GameObject* npc = GetOwner(scene);
	if (!npc) {
		ClearNavigationMove();
		return;
	}

	finalTarget_ = dest;
	pathPoints_.clear();
	pathIndex_ = 0;
	hasMoveTarget_ = false;
	moveMode_ = MoveMode::Pathfinding;
	directPathCheckTimer_ = 0.0f;

	const glm::vec3 pos3 = npc->GetPositionGLM();
	const glm::vec2 startPos(pos3.x, pos3.y);

	if (!scene.FindPathForObject(npc->GetID(), startPos, finalTarget_, pathPoints_)) {
		ClearNavigationMove();
		return;
	}

	const float kSkipWaypointRadius = 18.0f;
	while (!pathPoints_.empty()) {
		glm::vec2 d = pathPoints_.front() - startPos;
		if ((d.x * d.x + d.y * d.y) <= kSkipWaypointRadius * kSkipWaypointRadius) {
			pathPoints_.erase(pathPoints_.begin());
		}
		else {
			break;
		}
	}

	if (pathPoints_.empty()) {
		ClearNavigationMove();
		return;
	}

	hasMoveTarget_ = true;
	moveTarget_ = pathPoints_[0];
}

void SimpleNpcLogic::EnsureNavigationPlan(Scene& scene, GameObject* npc, const glm::vec2& desiredTarget) {
	if (!npc) return;

	glm::vec2 snappedTarget = desiredTarget;
	scene.GetNearestNavigationCellCenterForObject(npc->GetID(), desiredTarget, snappedTarget);

	constexpr float kRetargetEpsSq = 16.0f * 16.0f;

	if (hasMoveTarget_ || moveMode_ != MoveMode::None) {
		glm::vec2 d = snappedTarget - finalTarget_;
		if ((d.x * d.x + d.y * d.y) <= kRetargetEpsSq) {
			return;
		}
	}

	const glm::vec3 pos3 = npc->GetPositionGLM();
	const glm::vec2 start(pos3.x, pos3.y);

	if (scene.HasDirectPathForObject(npc->GetID(), start, snappedTarget)) {
		BeginMoveDirect(snappedTarget);
	}
	else {
		BeginMoveTo(scene, snappedTarget);
	}
}

bool SimpleNpcLogic::UpdateNavigationMove(float dt, Scene& scene, GameObject* npc) {
	if (!npc || !hasMoveTarget_) {
		return false;
	}

	glm::vec3 pos3 = npc->GetPositionGLM();
	glm::vec2 pos(pos3.x, pos3.y);

	const float arriveRadius = 6.0f;
	const float arriveRadiusSq = arriveRadius * arriveRadius;

	// Live shortcut while pathfinding
	if (moveMode_ == MoveMode::Pathfinding) {
		directPathCheckTimer_ -= dt;

		if (directPathCheckTimer_ <= 0.0f) {
			directPathCheckTimer_ = kDirectPathCheckInterval;

			if (scene.HasDirectPathForObject(npc->GetID(), pos, finalTarget_)) {
				moveMode_ = MoveMode::Direct;
				moveTarget_ = finalTarget_;
				pathPoints_.clear();
				pathIndex_ = 0;
				hasMoveTarget_ = true;
			}
		}
	}

	// ---------------------------
	// DIRECT MODE
	// ---------------------------
	if (moveMode_ == MoveMode::Direct) {
		moveTarget_ = finalTarget_;

		glm::vec2 dir = moveTarget_ - pos;
		float distSq = dir.x * dir.x + dir.y * dir.y;

		if (distSq <= arriveRadiusSq) {
			ClearNavigationMove();
			return true;
		}

		float dist = std::sqrt(distSq);
		if (dist > 0.0001f) {
			dir /= dist;
		}

		float step = speed * dt;
		if (step > dist) step = dist;

		glm::vec2 desiredDelta = dir * step;
		glm::vec2 allowedDelta = scene.ResolveWorldStep(npc, desiredDelta);

		float allowedLenSq =
			allowedDelta.x * allowedDelta.x +
			allowedDelta.y * allowedDelta.y;

		if (allowedLenSq < 0.0001f) {
			std::vector<glm::vec2> newPath;
			if (scene.FindPathForObject(npc->GetID(), pos, finalTarget_, newPath)) {
				pathPoints_ = newPath;
				pathIndex_ = 0;
				moveMode_ = MoveMode::Pathfinding;
				hasMoveTarget_ = !pathPoints_.empty();

				while (!pathPoints_.empty()) {
					glm::vec2 d = pathPoints_.front() - pos;
					if ((d.x * d.x + d.y * d.y) <= arriveRadiusSq) {
						pathPoints_.erase(pathPoints_.begin());
					}
					else {
						break;
					}
				}

				if (!pathPoints_.empty()) {
					moveTarget_ = pathPoints_.front();
					return false;
				}
			}

			ClearNavigationMove();
			return false;
		}

		pos += allowedDelta;
		npc->SetPosition(glm::vec3(pos.x, pos.y, pos3.z));
		scene.ClampToWalkArea(npc);
		return false;
	}

	// ---------------------------
	// PATHFINDING MODE
	// ---------------------------
	if (pathPoints_.empty()) {
		ClearNavigationMove();
		return false;
	}

	while (pathIndex_ < pathPoints_.size()) {
		glm::vec2 toWaypoint = pathPoints_[pathIndex_] - pos;
		float distSq = toWaypoint.x * toWaypoint.x + toWaypoint.y * toWaypoint.y;

		if (distSq <= arriveRadiusSq) {
			++pathIndex_;
		}
		else {
			break;
		}
	}

	if (pathIndex_ >= pathPoints_.size()) {
		ClearNavigationMove();
		return true;
	}

	moveTarget_ = pathPoints_[pathIndex_];

	glm::vec2 dir = moveTarget_ - pos;
	float distSq = dir.x * dir.x + dir.y * dir.y;
	float dist = std::sqrt(distSq);

	if (dist > 0.0001f) {
		dir /= dist;
	}

	float step = speed * dt;
	if (step > dist) step = dist;

	glm::vec2 desiredDelta = dir * step;
	glm::vec2 allowedDelta = scene.ResolveWorldStep(npc, desiredDelta);

	float allowedLenSq =
		allowedDelta.x * allowedDelta.x +
		allowedDelta.y * allowedDelta.y;

	if (allowedLenSq < 0.0001f) {
		std::vector<glm::vec2> newPath;

		if (scene.FindPathForObject(npc->GetID(), pos, finalTarget_, newPath)) {
			pathPoints_ = newPath;
			pathIndex_ = 0;

			while (!pathPoints_.empty()) {
				glm::vec2 d = pathPoints_.front() - pos;
				if ((d.x * d.x + d.y * d.y) <= arriveRadiusSq) {
					pathPoints_.erase(pathPoints_.begin());
				}
				else {
					break;
				}
			}

			if (!pathPoints_.empty()) {
				moveTarget_ = pathPoints_.front();
				return false;
			}
		}

		ClearNavigationMove();
		return false;
	}

	pos += allowedDelta;
	npc->SetPosition(glm::vec3(pos.x, pos.y, pos3.z));
	scene.ClampToWalkArea(npc);

	return false;
}

void SimpleNpcLogic::Update(float dt, Scene& scene, InputManager&) {
	// IMPORTANT: do not run logic in editor mode
	if (!scene.IsSimulationActive()) {
		return;
	}

	GameObject* npc = GetOwner(scene);
	if (!npc) return;

	glm::vec3 pos = npc->GetPositionGLM();

	glm::vec3 prevPos = npc->GetPositionGLM();

	// ===================== CASE 1: HAS CUSTOMER TARGET =====================
	if (hasCustomerTarget_) {
		const bool usePathfindingMovement =
			(behaviourState_ == BehaviourState::WalkingToTable) ||
			(behaviourState_ == BehaviourState::Leaving);

		if (usePathfindingMovement) {
			EnsureNavigationPlan(scene, npc, glm::vec2(customerSeatTarget_.x, customerSeatTarget_.y));
			const bool arrived = UpdateNavigationMove(dt, scene, npc);

			glm::vec3 newPos = npc->GetPositionGLM();
			glm::vec2 moveDelta(newPos.x - prevPos.x, newPos.y - prevPos.y);
			UpdateNpcAnimation(scene, npc, moveDelta);

			if (arrived) {
				if (behaviourState_ == BehaviourState::Leaving) {
					TS_LOG_DEBUG("[SimpleNpcLogic] Arrived at exit. NPC will despawn.");
					OnReachedExit(scene);
					return;
				}

				if (behaviourState_ == BehaviourState::WalkingToTable) {
					// Snap exactly to the authored seat position for cleaner visuals
					glm::vec3 seatedPos = npc->GetPositionGLM();
					seatedPos.x = customerSeatTarget_.x;
					seatedPos.y = customerSeatTarget_.y;
					npc->SetPosition(seatedPos);

					OnSeatedAtTable(scene);
					UpdateCustomerLogic(dt, scene);
					UpdateNpcAnimation(scene, npc, glm::vec2(0.0f, 0.0f));
					return;
				}
			}

			UpdateCustomerLogic(dt, scene);
			return;
		}

		// Has target, but not in a movement state anymore:
		// stay seated / waiting / eating / paying.
		UpdateCustomerLogic(dt, scene);
		UpdateNpcAnimation(scene, npc, glm::vec2(0.0f, 0.0f));
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
			state = nextMoveUp ? State::MoveUp : State::MoveDown;
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

	glm::vec3 newPos = npc->GetPositionGLM();
	glm::vec2 moveDelta(newPos.x - prevPos.x, newPos.y - prevPos.y);
	UpdateNpcAnimation(scene, npc, moveDelta);

	UpdateCustomerLogic(dt, scene);
}

void SimpleNpcLogic::UpdateCustomerLogic(float dt, Scene& scene) {
	if (behaviourState_ == BehaviourState::WaitingForFood && orderTaken_ && !dishServed_) {
		if (!patienceExpired_) {
			patienceRemaining_ -= dt;
			if (patienceRemaining_ <= 0.f) {
				OnPatienceExpired(scene);
			}
		}
	}

	if (behaviourState_ == BehaviourState::Eating) {
		eatTimer_ += dt;

		if (eatTimer_ >= eatDuration_) {
			eatTimer_ = eatDuration_;

			if (!finishedDish_)  // <-- make sure it only runs once
			{
				finishedDish_ = true;

				// Clear the food from the table now
				if (customerTableID_ != kInvalidID) {
					LogicManager& logicMgr = scene.GetLogicManager();
					if (auto* table = logicMgr.GetLogicForObject<CustomerTableLogic>(customerTableID_)) {
						table->ClearServedFood(scene);
					}
				}

				TS_LOG_DEBUG("[SimpleNpcLogic] Finished eating, switching to Paying");

				// Once done eating, NPC is ready to pay.
				behaviourState_ = BehaviourState::Paying;

				//TakePayment();
			}
		}
	}

	// Other behaviour transitions (e.g. auto-leave after paying)
	// can be added here later if you want.
}

static SimpleNpcLogic::FacingDir FacingFromDelta(const glm::vec2& d) {
	// Prefer left/right if horizontal dominates, else front/back
	if (std::abs(d.x) > std::abs(d.y))
		return (d.x > 0.0f) ? SimpleNpcLogic::FacingDir::Right : SimpleNpcLogic::FacingDir::Left;

	// +Y is down in your game -> "Front"
	return (d.y > 0.0f) ? SimpleNpcLogic::FacingDir::Front : SimpleNpcLogic::FacingDir::Back;
}

bool SimpleNpcLogic::TryGetDeltaToTable(Scene& scene, glm::vec2& outDelta) const {
	if (customerTableID_ == kInvalidID) return false;

	GameObject* me = GetOwner(scene);
	GameObject* tableObj = scene.GetGameObjectByID(customerTableID_);
	if (!me || !tableObj) return false;

	glm::vec3 mp = me->GetPositionGLM();
	glm::vec3 tp = tableObj->GetPositionGLM();

	outDelta = glm::vec2(tp.x - mp.x, tp.y - mp.y);
	return true;
}

void SimpleNpcLogic::AssignCustomerTable(int tableObjectID) {
	customerTableID_ = tableObjectID;

	TS_LOG_DEBUG("[SimpleNpcLogic] AssignCustomerTable tableID=" << tableObjectID);

	// If currently idle as a customer, start looking for the table.
	if (behaviourState_ == BehaviourState::Idle) {
		behaviourState_ = BehaviourState::FindingTable;
	}
}

void SimpleNpcLogic::OnSeatedAtTable(Scene& scene) {
	int npcID = -1;
	if (GameObject* owner = GetOwner(scene)) {
		npcID = owner->GetID();
	}

	if (!dishRolled_) {
		desiredDishType_ = RollRandomDish(scene);
		dishRolled_ = true;

		TS_LOG_DEBUG("[SimpleNpcLogic] Rolled desired dish = "
			<< DishTypeName(desiredDishType_)
			<< " (" << static_cast<int>(desiredDishType_) << ")");
	}

	// When NPC reaches its assigned table, it should start ordering.
	if (behaviourState_ == BehaviourState::FindingTable ||
		behaviourState_ == BehaviourState::WalkingToTable) {
		// Start ordering this dish (2 processed veg salad).
		behaviourState_ = BehaviourState::Ordering;

		// Auto-take the order so we move into WaitingForFood right away.
		TakeOrder(scene); // This sets behaviourState_ = WaitingForFood
	}
}


void SimpleNpcLogic::TakeOrder(Scene& scene) {
	TS_LOG_DEBUG("[SimpleNpcLogic] TakeOrder, state="
		<< static_cast<int>(behaviourState_));

	// Only meaningful if in ORDERING state.
	if (behaviourState_ != BehaviourState::Ordering)
		return;

	orderTaken_ = true;
	patienceRatioAtServe_ = 1.0f; // starts full; will be snapshotted on serve

	// Play new order sound effect
	if (AudioManager* audioMgr = scene.GetAudioManager()) {
		audioMgr->PlaySound("sfx_new_order_v2", audioMgr->GetVfxVolume() * 0.8f, false);
	}

	// Start patience timer now that we are waiting for food
	patienceRemaining_ = patienceMax_;
	patienceExpired_ = false;
	payZero_ = false;

	behaviourState_ = BehaviourState::WaitingForFood;
#ifdef _DEBUG
	(void)scene;
#endif
}

void SimpleNpcLogic::OnDishServed(Scene& scene, DishType dishType) {
	TS_LOG_DEBUG("[SimpleNpcLogic] OnDishServed, dishType=" << static_cast<int>(dishType));

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

	if (servedDishType_ != desiredDishType_) {
		// Clear served food so table isn't blocked
		if (customerTableID_ != kInvalidID) {
			LogicManager& logicMgr = scene.GetLogicManager();
			if (auto* table = logicMgr.GetLogicForObject<CustomerTableLogic>(customerTableID_)) {
				table->ClearServedFood(scene);
			}
		}

		payZero_ = true;
		hasPaid_ = false; // doesn't matter much, BeginLeaveToExit sets hasPaid_=true
		patienceRatioAtServe_ = 0.0f;

		TS_LOG_DEBUG("[SimpleNpcLogic] Wrong dish served. Leaving immediately (pay $0)");

		BeginLeaveToExit(scene, true); // free table NOW
		return;
	}

	payZero_ = false;
	behaviourState_ = BehaviourState::Eating;
	eatTimer_ = 0.0f;
}

void SimpleNpcLogic::TakePayment(Scene& scene) {
	if (behaviourState_ != BehaviourState::Paying)
		return;

	// Play payment or wrong order sound effect (release mode only)
#ifndef _DEBUG
	if (AudioManager* audioMgr = scene.GetAudioManager()) {
		if (payZero_) {
			// Wrong order or patience expired - play wrong order sound
			PlaySpatialSfxAtNpc(scene, GetOwnerID(), "sfx_wrong_order", audioMgr->GetVfxVolume() * 0.3f);
		}
		else {
			// Successful order - play payment sound
			PlaySpatialSfxAtNpc(scene, GetOwnerID(), "sfx_payment", audioMgr->GetVfxVolume() * 0.3f);
		}
	}
#endif

	hasPaid_ = true;
	behaviourState_ = BehaviourState::Leaving;

	// Play a random happy customer voice line when leaving after successful payment
	if (!payZero_) {
		if (AudioManager* audioMgr = scene.GetAudioManager()) {
			std::uniform_int_distribution<int> dist(1, 9);
			int variant = dist(EngineRng::Get());
			std::string sfxName = "vo_customer_happy_0" + std::to_string(variant);
			PlaySpatialSfxAtNpc(scene, GetOwnerID(), sfxName, audioMgr->GetVfxVolume());
		}
	}

	if (hasLeaveTarget_) {
		hasCustomerTarget_ = true;
		customerSeatTarget_ = leaveTargetWorldPos_;
	}
	else {
		Math::Vector2D gate = scene.GetExitGateWorldPos();
		hasCustomerTarget_ = true;
		customerSeatTarget_ = gate;
	}
	ClearNavigationMove();

	// NOTE: do NOT change customerTableID_ here, you still want to know which table to free.
}

void SimpleNpcLogic::SetCustomerTableTarget(int tableObjectID, const Math::Vector2D& seatWorldPos) {
	customerTableID_ = tableObjectID;
	customerSeatTarget_ = seatWorldPos;
	hasCustomerTarget_ = true;
	behaviourState_ = BehaviourState::WalkingToTable;
	ClearNavigationMove();

}

void SimpleNpcLogic::SetLeaveTarget(const Math::Vector2D& leaveWorldPos) {
	leaveTargetWorldPos_ = leaveWorldPos;
	hasLeaveTarget_ = true;
}

void SimpleNpcLogic::ClearCustomerTableTarget() {
	hasCustomerTarget_ = false;
	customerTableID_ = kInvalidID;
	customerSeatTarget_ = Math::Vector2D(0.0f, 0.0f);
	ClearNavigationMove();
}

void SimpleNpcLogic::CacheExitGatePos(Scene& scene) {
	if (hasExitGatePos_) return;

	LogicManager& logicMgr = scene.GetLogicManager();

	GameObject* me = GetOwner(scene);
	glm::vec3 myPos = me ? me->GetPositionGLM() : glm::vec3(0.0f);

	bool found = false;
	float bestDistSq = std::numeric_limits<float>::max();

	for (GameObject* obj : scene.GetAllObjectsRaw()) {
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
		TS_LOG_WARN("[SimpleNpcLogic] No ExitGateLogic found. Using fallback.");
	}
	else {
		TS_LOG_DEBUG("[SimpleNpcLogic] Cached exit gate target at ("
			<< exitGateWorldPos_.x << ", " << exitGateWorldPos_.y << ")");
	}

	hasExitGatePos_ = true;
}


void SimpleNpcLogic::OnReachedExit(Scene& scene) {
	if (exitProcessed_) return;
	exitProcessed_ = true;
	TS_LOG_DEBUG("[SimpleNpcLogic] Reached exit gate. Despawning.");

	// Play customer leaving sound effect (release mode only)
#ifndef _DEBUG
	if (AudioManager* audioMgr = scene.GetAudioManager()) {
		audioMgr->PlaySound("sfx_customer_leaving", audioMgr->GetVfxVolume() * 0.3f, false);
	}
#endif

	// Free the customer table
	if (customerTableID_ != kInvalidID) {
		LogicManager& logicMgr = scene.GetLogicManager();
		if (CustomerTableLogic* table = logicMgr.GetLogicForObject<CustomerTableLogic>(customerTableID_)) {
			if (GameObject* npc = GetOwner(scene)) {
				table->ClearCustomer(npc->GetID());
			}
		}
	}

	// Despawn this NPC
	if (GameObject* npc = GetOwner(scene)) {
		scene.RequestDespawn(npc->GetID());
	}
}

DishType SimpleNpcLogic::RollRandomDish(Scene& scene) {
	static const DishType kPoolBase[] = {
		DishType::VegDish,
		DishType::MeatDish,
		DishType::SoupDish
		//DishType::PoopDish
	};

	static const DishType kPoolLevel2[] = {
	DishType::VegDish,
	DishType::MeatDish,
	DishType::SoupDish,
	DishType::SkewerDish,
	DishType::CarrotSaladDish
	};

	const bool isLevel2 = scene.GetCurrentLevelPath().find("kitchen02") != std::string::npos;
	const DishType* pool = isLevel2 ? kPoolLevel2 : kPoolBase;
	const int poolSize = isLevel2
		? (int)(sizeof(kPoolLevel2) / sizeof(kPoolLevel2[0]))
		: (int)(sizeof(kPoolBase) / sizeof(kPoolBase[0]));

	static std::mt19937 rng{ std::random_device{}() };
	std::uniform_int_distribution<int> dist(0, poolSize - 1);
	return pool[dist(EngineRng::Get())];
}

void SimpleNpcLogic::OnPatienceExpired(Scene& scene) {
	if (patienceExpired_) return;

	patienceExpired_ = true;
	patienceRemaining_ = 0.f;
	patienceRatioAtServe_ = 0.0f;

	payZero_ = true;
	hasPaid_ = false; // BeginLeaveToExit will set it true
	behaviourState_ = BehaviourState::Paying; // (optional, just for clarity)

	BeginLeaveToExit(scene, true);

	// Optional safety: clear served food if anything got stuck
	if (customerTableID_ != kInvalidID) {
		LogicManager& logicMgr = scene.GetLogicManager();
		if (auto* table = logicMgr.GetLogicForObject<CustomerTableLogic>(customerTableID_)) {
			table->ClearServedFood(scene);
		}
	}

}

void SimpleNpcLogic::UpdateNpcAnimation(Scene& scene, GameObject* npc, const glm::vec2& moveDelta) {
	if (!npc) return;

	const float epsX = 0.01f;
	const float epsY = 0.01f;

	const float absX = std::abs(moveDelta.x);
	const float absY = std::abs(moveDelta.y);
	const bool moving = (absX > epsX) || (absY > epsY);

	// --- Decide whether to face by movement or by table ---
	const bool movementFacing =
		(behaviourState_ == BehaviourState::WalkingToTable) ||
		(behaviourState_ == BehaviourState::Leaving);

	glm::vec2 tableDelta{};
	const bool hasTable = TryGetDeltaToTable(scene, tableDelta);

	// 1) Decide facingDir_
	if (movementFacing) {
		// walking/leaving: use movement-based facing (your current behavior)
		if (moving) {
			if (absX > epsX)
				facingDir_ = (moveDelta.x > 0.0f) ? FacingDir::Right : FacingDir::Left;
			else
				facingDir_ = (moveDelta.y > 0.0f) ? FacingDir::Front : FacingDir::Back;
		}
	}
	else {
		// all other states: face toward the table (if we have one)
		if (hasTable) {
			facingDir_ = FacingFromDelta(tableDelta);
		}
		else if (moving) {
			// fallback if table missing
			if (absX > epsX)
				facingDir_ = (moveDelta.x > 0.0f) ? FacingDir::Right : FacingDir::Left;
			else
				facingDir_ = (moveDelta.y > 0.0f) ? FacingDir::Front : FacingDir::Back;
		}
	}

	std::string desired;

	// 2) Choose animation clip
	if (behaviourState_ == BehaviourState::Eating) {
		// only have left/right eat, so pick based on table X when possible
		if (facingDir_ == FacingDir::Left) desired = "EAT_LEFT";
		else if (facingDir_ == FacingDir::Right) desired = "EAT_RIGHT";
		else if (hasTable && std::abs(tableDelta.x) > 0.001f)
			desired = (tableDelta.x < 0.0f) ? "EAT_LEFT" : "EAT_RIGHT";
		else
			desired = "EAT_RIGHT";
	}
	else if (moving) {
		switch (facingDir_) {
		case FacingDir::Front: desired = "WALK_FRONT"; break;
		case FacingDir::Back:  desired = "WALK_BACK";  break;
		case FacingDir::Left:  desired = "WALK_LEFT";  break;
		case FacingDir::Right: desired = "WALK_RIGHT"; break;
		}
	}
	else {
		switch (facingDir_) {
		case FacingDir::Left:  desired = "IDLE_LEFT";  break;
		case FacingDir::Right: desired = "IDLE_RIGHT"; break;
		case FacingDir::Front: desired = "IDLE_FRONT"; break;
		case FacingDir::Back:  desired = "IDLE_BACK";  break;
		}
	}

	const std::string current = scene.GetCurrentAnimationName(npc->GetID());
	if (current != desired)
		scene.SetAnimation(npc->GetID(), desired);
}

void SimpleNpcLogic::BeginLeaveToExit(Scene& scene, bool freeTableImmediately) {
	// If already leaving, don't re-trigger
	if (behaviourState_ == BehaviourState::Leaving)
		return;

	// Play "wrong order" sound immediately when leaving unhappy
	if (payZero_) {
		if (AudioManager* audioMgr = scene.GetAudioManager()) {
			PlaySpatialSfxAtNpc(scene, GetOwnerID(), "sfx_wrong_order", audioMgr->GetVfxVolume() * 0.3f);
		}
	}

	hasPaid_ = true; // "payment processed" (even if $0)
	behaviourState_ = BehaviourState::Leaving;

	// Play a random angry customer voice line when leaving unhappy
	if (payZero_) {
		if (AudioManager* audioMgr = scene.GetAudioManager()) {
			std::uniform_int_distribution<int> dist(1, 8);
			int variant = dist(EngineRng::Get());
			std::string sfxName = "vo_customer_angry_0" + std::to_string(variant);
			PlaySpatialSfxAtNpc(scene, GetOwnerID(), sfxName, audioMgr->GetVfxVolume());
		}
	}

	// Walk to the authored leave target (defaults to exit gate when unavailable)
	if (hasLeaveTarget_) {
		hasCustomerTarget_ = true;
		customerSeatTarget_ = leaveTargetWorldPos_;
	}
	else {
		Math::Vector2D gate = scene.GetExitGateWorldPos();
		hasCustomerTarget_ = true;
		customerSeatTarget_ = gate;
	}
	ClearNavigationMove();

	// Free the table RIGHT NOW so another customer can take it
	if (freeTableImmediately && customerTableID_ != kInvalidID) {
		LogicManager& logicMgr = scene.GetLogicManager();
		if (auto* table = logicMgr.GetLogicForObject<CustomerTableLogic>(customerTableID_)) {
			if (GameObject* npc = GetOwner(scene)) {
				table->ClearCustomer(npc->GetID());
			}
		}

		// IMPORTANT:
		// Prevent OnReachedExit() from clearing a NEW customer seated later.
		customerTableID_ = kInvalidID;
	}
}
