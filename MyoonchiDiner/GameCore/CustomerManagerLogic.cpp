/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CustomerManagerlogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu   (80%)
 CO-AUTHORS:        Ng Juin Herng, juinherng.ng@digipen.edu (5%)
					Yat Chun Wee, y.chunwee@digipen.edu		(10%)
					Seah Wang Hua, wanghua.seah@digipen.edu (5%)

 DESCRIPTION:       Implements a simple scene-level system that manages all
					customers in the level. It assigns customers to available
					CustomerTableLogic tables, gives them target seating
					positions, and coordinates table to customer pairing at runtime.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "EngineCore/AudioManager.hpp"
#include "EngineCore/EngineRng.hpp"
#include "EngineCore/Logger.hpp"
#include "EngineCore/LogicManager.hpp"
#include "EngineCore/Math.hpp"
#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/CustomerManagerLogic.hpp"
#include "GameCore/CustomerOrderUILogic.hpp"
#include "GameCore/CustomerTableLogic.hpp"
#include "GameCore/SimpleNpcLogic.hpp"
#include "MyoonchiDiner/GamePaths.hpp"

namespace {
	/**
	 * @brief Chooses a random customer sprite from the authored skin set.
	 * @return Texture path for the selected customer appearance.
	 */
	std::string PickRandomCustomerTexture() {
		// Keep the available spawn skins centralized so random selection stays consistent.
		static const std::array<const char*, 3> kCustomerSkins = {
			MyoonchiPaths::Textures::CUSTOMER_GOAT,
			MyoonchiPaths::Textures::CUSTOMER_TIGER,
			MyoonchiPaths::Textures::CUSTOMER_ANTEATER
		};

		std::uniform_int_distribution<int> dist(0, static_cast<int>(kCustomerSkins.size()) - 1);
		return kCustomerSkins[dist(EngineRng::Get())];
	}

	/**
	 * @brief Plays a positional sound effect if the requested clip is available.
	 * @param scene Active scene that owns the audio manager.
	 * @param soundName Registered sound name to play.
	 * @param pos World-space position for 3D playback.
	 * @param volume Playback volume multiplier.
	 * @param minDistance Minimum attenuation distance.
	 * @param maxDistance Maximum attenuation distance.
	 */
	void PlaySpatialSfxAtPos(Scene& scene, const std::string& soundName, const glm::vec3& pos, float volume, float minDistance = 120.0f, float maxDistance = 1100.0f) {
		// Skip playback cleanly when audio is unavailable or the clip is not registered.
		AudioManager* audioMgr = scene.GetAudioManager();
		if (!audioMgr || !audioMgr->HasSound(soundName)) {
			return;
		}

		audioMgr->PlaySound3D(soundName, pos.x, pos.y, pos.z, volume, minDistance, maxDistance, false);
	}
}

namespace {
	/**
	 * @brief Tests whether a world-space point lies inside an object's visible bounds.
	 * @param point Point to test in world space.
	 * @param obj Object whose collider or visual bounds should be checked.
	 * @return True if the point falls inside the object's visual rectangle.
	 */
	bool PointInsideObjectVisualRect(const glm::vec2& point, GameObject* obj) {
		// Treat missing objects as a clean miss so callers can probe optional UI safely.
		if (!obj) return false;

		const Math::Vector2D colSize = obj->GetColliderSize();
		const Math::Vector2D colOffset = obj->GetColliderOffset();
		const glm::vec3 scale = obj->GetScaleGLM();

		// Bubble objects use collider size 0, so fall back to visual scale
		const float width = (colSize.x > 0.0f) ? colSize.x : scale.x;
		const float height = (colSize.y > 0.0f) ? colSize.y : scale.y;

		if (width <= 0.0f || height <= 0.0f) {
			return false;
		}

		const glm::vec3 pos = obj->GetPositionGLM();
		const glm::vec2 center(pos.x + colOffset.x, pos.y + colOffset.y);

		const float halfW = width * 0.5f;
		const float halfH = height * 0.5f;

		return
			point.x >= center.x - halfW && point.x <= center.x + halfW &&
			point.y >= center.y - halfH && point.y <= center.y + halfH;
	}
}

/**
 * @brief Clears runtime caches and resets spawn tuning to default values.
 */
void CustomerManagerSystem::Reset() {
	// Forget all per-scene cached IDs so the next level can rebuild fresh state.
	activeCustomers_.clear();
	customerTableIDs_.clear();
	customerEntryIDs_.clear();

	// Reset tuning too, so one level's spawn curve does not leak into another.
	maxCustomers_ = 16;

	startSpawnCooldown_ = 10.0f;
	spawnCooldown_ = 10.0f;
	minSpawnCooldown_ = 10.0f;
	spawnCooldownStep_ = 0.0f;
	initialSpawnDelay_ = 0.0f;
	cooldownRampStopTime_ = -1.0f;

	totalSpawnLimit_ = -1;
	totalSpawned_ = 0;
	spawnWithInfinitePatience_ = false;

	cachedTables_ = false;

	customerTemplateID_ = -1;
	cachedTemplate_ = false;

	nextEntryIndex_ = 0;
	cachedEntries_ = false;

	// Start from zero. The first customer now waits for initialSpawnDelay_.
	spawnTimer_ = 0.0f;
	levelElapsed_ = 0.0f;
	hasSpawnedAtLeastOnce_ = false;
}

/**
 * @brief Caches all customer-table object IDs currently present in the scene.
 * @param scene Active scene to scan.
 */
void CustomerManagerSystem::CacheTables(Scene& scene) {
	// Rebuild the table cache from scratch to match the current scene contents.
	customerTableIDs_.clear();

	LogicManager& logicMgr = scene.GetLogicManager();
	for (GameObject* obj : scene.GetAllObjectsRaw()) {
		if (!obj) continue;
		const int id = obj->GetID();
		if (logicMgr.GetLogicForObject<CustomerTableLogic>(id)) {
			customerTableIDs_.push_back(id);
		}
	}

	cachedTables_ = true;
	TS_LOG_DEBUG("[CustomerManager] Cached " << customerTableIDs_.size() << " customer tables");
}

/**
 * @brief Finds the authored customer template object used for runtime spawns.
 * @param scene Active scene to scan.
 */
void CustomerManagerSystem::CacheTemplate(Scene& scene) {
	// Reset the cached template before scanning so missing templates are reported correctly.
	customerTemplateID_ = -1;

	for (GameObject* obj : scene.GetAllObjectsRaw()) {
		if (!obj) continue;

		const int id = obj->GetID();
		Scene::Defaults d = scene.GetDefaults(id);

		if (d.tag == "customer_template") {
			customerTemplateID_ = id;
			break;
		}
	}

	cachedTemplate_ = true;

	if (customerTemplateID_ < 0) {
		TS_LOG_WARN("[CustomerManager] No customer_template found in JSON.");
	}
	else {
		TS_LOG_DEBUG("[CustomerManager] Cached customer template id=" << customerTemplateID_);
	}
}

/**
 * @brief Caches all authored customer-entry spawn markers in the scene.
 * @param scene Active scene to scan.
 */
void CustomerManagerSystem::CacheEntries(Scene& scene) {
	// Refresh the entry list so spawn rotation uses the current authored markers only.
	customerEntryIDs_.clear();

	for (GameObject* obj : scene.GetAllObjectsRaw()) {
		if (!obj) continue;
		const int id = obj->GetID();
		Scene::Defaults d = scene.GetDefaults(id);
		if (d.tag == "customer_entry") {
			customerEntryIDs_.push_back(id);
		}
	}

	TS_LOG_DEBUG("[CustomerManager] Cached " << customerEntryIDs_.size() << " customer entries");
	cachedEntries_ = true;
}

/**
 * @brief Removes inactive customers from the live active-customer list.
 * @param scene Active scene that owns the tracked customers.
 */
void CustomerManagerSystem::CleanupDeadCustomers(Scene& scene) {
	// Prune despawned or already-leaving customers so the active cap reflects playable diners.
	LogicManager& logicMgr = scene.GetLogicManager();

	activeCustomers_.erase(
		std::remove_if(activeCustomers_.begin(), activeCustomers_.end(),
			[&](int id) {
				// Despawned -> remove
				if (scene.GetGameObjectByID(id) == nullptr)
					return true;

				// Leaving customers no longer count toward active cap (table already freed)
				if (auto* npc = logicMgr.GetLogicForObject<SimpleNpcLogic>(id)) {
					if (npc->IsLeaving())
						return true;
				}

				return false;
			}),
		activeCustomers_.end()
	);
}

/**
 * @brief Attempts to spawn one customer and assign them to an available seat.
 * @param scene Active scene containing the spawn template, entry markers, and tables.
 * @return True if the spawn and seating process succeeded.
 */
bool CustomerManagerSystem::TrySpawnOne(Scene& scene) {
	// Refuse to spawn until the template exists and any total cap still allows another customer.
	if (customerTemplateID_ < 0)
		return false;

	if (totalSpawnLimit_ >= 0 && totalSpawned_ >= totalSpawnLimit_) {
		return false;
	}

	LogicManager& logicMgr = scene.GetLogicManager();

	// Compute a horizontal split so entries prefer seating customers on the same half of the room.
	float tableSplitX = 0.0f;
	int tableCount = 0;

	for (int tableID : customerTableIDs_) {
		if (GameObject* tableObj = scene.GetGameObjectByID(tableID)) {
			tableSplitX += tableObj->GetPositionGLM().x;
			++tableCount;
		}
	}
	if (tableCount > 0) {
		tableSplitX /= static_cast<float>(tableCount);
	}

	// Resolve the most suitable free table for a spawn position, preferring the matching side first.
	auto findAvailableTableForSpawnX = [&](float spawnX, CustomerTableLogic*& outTable, int& outTableID) -> bool {
		const bool wantsLeftSide = (spawnX < tableSplitX);

		// First pass: prefer same-side table
		for (int tableID : customerTableIDs_) {
			auto* table = logicMgr.GetLogicForObject<CustomerTableLogic>(tableID);
			if (!table || !table->IsAvailableForSeating()) continue;

			GameObject* tableObj = scene.GetGameObjectByID(tableID);
			if (!tableObj) continue;

			const bool tableIsLeftSide = (tableObj->GetPositionGLM().x < tableSplitX);
			if (tableIsLeftSide == wantsLeftSide) {
				outTable = table;
				outTableID = tableID;
				return true;
			}
		}

		// Second pass: if no same-side table exists, use any free table
		for (int tableID : customerTableIDs_) {
			auto* table = logicMgr.GetLogicForObject<CustomerTableLogic>(tableID);
			if (!table || !table->IsAvailableForSeating()) continue;

			outTable = table;
			outTableID = tableID;
			return true;
		}

		return false;
		};

	// Choose the next usable entry marker and pair it with an available table.
	CustomerTableLogic* chosenTable = nullptr;
	int chosenTableID = -1;
	Math::Vector2D spawn2 = scene.GetExitGateWorldPos();

	if (!customerEntryIDs_.empty()) {
		const int entryCount = static_cast<int>(customerEntryIDs_.size());
		for (int attempt = 0; attempt < entryCount; ++attempt) {
			const int idx = (nextEntryIndex_ + attempt) % entryCount;
			GameObject* entryObj = scene.GetGameObjectByID(customerEntryIDs_[idx]);
			if (!entryObj) continue;
			const glm::vec3 p = entryObj->GetPositionGLM();
			if (findAvailableTableForSpawnX(p.x, chosenTable, chosenTableID)) {
				spawn2 = { p.x, p.y };
				nextEntryIndex_ = (idx + 1) % entryCount;
				break;
			}
		}
	}
	else {
		findAvailableTableForSpawnX(spawn2.x, chosenTable, chosenTableID);
	}

	if (!chosenTable) return false;

	// Copy spawn profile data from the template object before creating the NPC.
	Scene::Defaults prof = scene.GetDefaults(customerTemplateID_);

	glm::vec3 spawnPos{ spawn2.x, spawn2.y, 0.0f };

	// Spawn as an animated sprite so the customer animation system can attach clips immediately.
	std::vector<glm::vec4> dummyFrames = { glm::vec4(0.f, 0.f, 1.f, 1.f) };

	const std::string chosenTexture = PickRandomCustomerTexture();

	GameObject* npc = scene.SpawnAnimatedSprite(
		chosenTexture,
		spawnPos,
		prof.size,
		dummyFrames,
		0.1f,
		true,
		prof.layer
	);
	if (!npc) return false;

	const int npcID = npc->GetID();

	// Mirror template shadow settings so runtime-spawned customers match authored ones visually.
	if (GameObject* templateObj = scene.GetGameObjectByID(customerTemplateID_)) {
		npc->EnableShadow(templateObj->HasShadow());
		npc->SetShadowSize(templateObj->GetShadowSize());
		npc->SetShadowOffset(templateObj->GetShadowOffset());
		npc->SetShadowOpacity(templateObj->GetShadowOpacity());
	}

	// Attach the same tag, logic, and animations that authored customer objects receive.
	scene.SetObjectTag(npcID, "customer_template");
	scene.AttachLogicForTag(npcID, "customer_template");
	scene.SetObjectTexturePath(npcID, chosenTexture);
	scene.AttachCustomersAnimations(npcID, chosenTexture);
	scene.SetAnimation(npcID, "IDLE_FRONT");

	// Apply authored movement and collider settings copied from the template profile.
	npc->SetColliderSize(Math::Vector2D(prof.colSize.x, prof.colSize.y));
	npc->SetColliderOffset(Math::Vector2D(prof.colOff.x, prof.colOff.y));
	scene.SetNPCVelocity(npcID, prof.vel.x, prof.vel.y);

	// Attach the thought-bubble and patience-bar logic immediately after the NPC is created.
	if (auto* ui = logicMgr.AddLogic<CustomerOrderUILogic>(npcID)) {
		ui->Start(scene);
	}

	// Reserve a table seat before activating the NPC's movement target toward that seat.
	Math::Vector2D seatWorld;
	if (!chosenTable->SeatCustomer(scene, npcID, &seatWorld)) {
		scene.RequestDespawn(npcID);
		return false;
	}

	if (auto* npcLogic = logicMgr.GetLogicForObject<SimpleNpcLogic>(npcID)) {
		if (spawnWithInfinitePatience_) {
			npcLogic->SetInfinitePatience(true);
		}
		npcLogic->SetCustomerTableTarget(chosenTableID, seatWorld);
		npcLogic->SetLeaveTarget(spawn2);
	}

	scene.ClampToWalkArea(npc);

	// Track the spawned customer so future cap checks and cleanup can see them.
	activeCustomers_.push_back(npcID);
	++totalSpawned_;
	hasSpawnedAtLeastOnce_ = true;

	// Keep the first repeat cooldown at its starting value (e.g. 15s),
	// then begin accelerating from the second successful spawn onward:
	// 15 -> 14 -> 13 -> ...
	const bool canStillRampByTime =
		(cooldownRampStopTime_ < 0.0f) || (levelElapsed_ < cooldownRampStopTime_);

	if (canStillRampByTime &&
		totalSpawned_ >= 2 &&
		spawnCooldownStep_ > 0.0f &&
		spawnCooldown_ > minSpawnCooldown_) {
		// Gradually shorten future spawn spacing until the configured minimum is reached.
		spawnCooldown_ = std::max(minSpawnCooldown_, spawnCooldown_ - spawnCooldownStep_);
	}

	// Keep the authored arrival SFX behind runtime parity mode to match release behavior.
	if (scene.ShouldUseRuntimeParityMode()) {
		if (AudioManager* audioMgr = scene.GetAudioManager()) {
			PlaySpatialSfxAtPos(scene, "sfx_customer_entering", npc->GetPositionGLM(), audioMgr->GetVfxVolume() * 0.3f);
		}
	}

	TS_LOG_DEBUG("[CustomerManager] Spawned customer " << npcID
		<< " -> table " << chosenTableID
		<< " | next cooldown=" << spawnCooldown_);

	return true;
}

/**
 * @brief Updates customer spawning and seat-cap enforcement for the current frame.
 * @param dt Delta time for the frame.
 * @param scene Active scene that owns the tables and customer NPCs.
 */
void CustomerManagerSystem::Update(float dt, Scene& scene) {
	// Spawning only advances while gameplay simulation is live.
	if (!scene.IsSimulationActive())
		return;

	// Lazily build scene caches the first time Update runs for this level.
	if (!cachedTables_) CacheTables(scene);
	if (!cachedTemplate_) CacheTemplate(scene);
	if (!cachedEntries_) CacheEntries(scene);
	if (!spawningEnabled_) {
		return;
	}

	CleanupDeadCustomers(scene);

	// Advance the level clock and the time accumulated toward the next spawn.
	levelElapsed_ += dt;
	spawnTimer_ += dt;

	// Derive the seat-based cap from all currently registered customer tables.
	int totalSeatCap = 0;
	LogicManager& logicMgr = scene.GetLogicManager();

	for (int tableID : customerTableIDs_) {
		if (auto* table = logicMgr.GetLogicForObject<CustomerTableLogic>(tableID)) {
			totalSeatCap += table->GetSeatCapacity();
		}
	}

	const int targetCount = std::min(maxCustomers_, totalSeatCap);

	// Spawn repeatedly only while the active roster is still below the allowed target.
	while ((int)activeCustomers_.size() < targetCount &&
		(totalSpawnLimit_ < 0 || totalSpawned_ < totalSpawnLimit_)) {

		const float requiredDelay = hasSpawnedAtLeastOnce_
			? spawnCooldown_
			: initialSpawnDelay_;

		if (requiredDelay > 0.0f && spawnTimer_ < requiredDelay) {
			break;
		}

		if (!TrySpawnOne(scene)) {
			break;
		}

		// Consume only the cooldown that the successful spawn actually used.
		if (requiredDelay > 0.0f) {
			spawnTimer_ -= requiredDelay;
		}
		else {
			spawnTimer_ = 0.0f;
		}
	}
}

/**
 * @brief Tests whether a world-space point overlaps the customer's order bubble.
 * @param scene Active scene containing the bubble objects.
 * @param worldPos World-space point to test.
 * @return True if the point hits the bubble background or icon.
 */
bool CustomerOrderUILogic::HitTestBubble(Scene& scene, const glm::vec2& worldPos) const {
	// Check both bubble sprites so hover logic can treat the whole bubble as one target.
	if (PointInsideObjectVisualRect(worldPos, scene.GetGameObjectByID(bubbleBG_ID_))) {
		return true;
	}

	if (PointInsideObjectVisualRect(worldPos, scene.GetGameObjectByID(bubbleDish_ID_))) {
		return true;
	}

	return false;
}
