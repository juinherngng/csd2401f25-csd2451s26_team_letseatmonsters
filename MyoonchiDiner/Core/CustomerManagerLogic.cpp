/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CustomerManagerlogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu   (95%)
 CO-AUTHORS:        Ng Juin Herng, juinherng.ng@digipen.edu (5%)

 DESCRIPTION:       Implements a simple scene-level system that manages all
					customers in the level. It assigns customers to available
					CustomerTableLogic tables, gives them target seating
					positions, and coordinates table to customer pairing at runtime.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "Core/AudioManager.hpp"
#include "Core/CustomerManagerLogic.hpp"
#include "Core/CustomerOrderUILogic.hpp"
#include "Core/CustomerTableLogic.hpp"
#include "Core/LogicManager.hpp"
#include "Core/Math.hpp"
#include "Core/SimpleNpcLogic.hpp"
#include "Graphics/GameObject.hpp"
#include "Graphics/SceneManager.hpp"

#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {
	bool PointInsideObjectVisualRect(const glm::vec2& point, GameObject* obj) {
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

void CustomerManagerSystem::Reset() {
	activeCustomers_.clear();
	customerTableIDs_.clear();
	customerEntryIDs_.clear();

	cachedTables_ = false;

	customerTemplateID_ = -1;
	cachedTemplate_ = false;

	nextEntryIndex_ = 0;
	cachedEntries_ = false;
	spawnTimer_ = 180.0f;
	totalSpawned_ = 0;
}

void CustomerManagerSystem::CacheTables(Scene& scene) {
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
	std::cout << "[CustomerManager] Cached " << customerTableIDs_.size() << " customer tables\n";
}

void CustomerManagerSystem::CacheTemplate(Scene& scene) {
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
		std::cout << "[CustomerManager] WARNING: No customer_template found in JSON.\n";
	}
	else {
		std::cout << "[CustomerManager] Cached customer template id=" << customerTemplateID_ << "\n";
	}
}

void CustomerManagerSystem::CacheEntries(Scene& scene) {
	customerEntryIDs_.clear();

	for (GameObject* obj : scene.GetAllObjectsRaw()) {
		if (!obj) continue;
		const int id = obj->GetID();
		Scene::Defaults d = scene.GetDefaults(id);
		if (d.tag == "customer_entry") {
			customerEntryIDs_.push_back(id);
		}
	}

	std::cout << "[CustomerManager] Cached " << customerEntryIDs_.size() << " customer entries\n";
	cachedEntries_ = true;
}

void CustomerManagerSystem::CleanupDeadCustomers(Scene& scene) {
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

bool CustomerManagerSystem::TrySpawnOne(Scene& scene) {
	if (customerTemplateID_ < 0)
		return false;

	if (totalSpawnLimit_ >= 0 && totalSpawned_ >= totalSpawnLimit_) {
		return false;
	}

	LogicManager& logicMgr = scene.GetLogicManager();

	// Compute a horizontal split for table-side matching.
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

	// Read profile from template
	auto findAvailableTableForSpawnX = [&](float spawnX, CustomerTableLogic*& outTable, int& outTableID) -> bool {
		const bool wantsLeftSide = (spawnX < tableSplitX);

		// First pass: strictly match table side to spawner side.
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

		return false;
		};

	// Find a spawn entry and a matching-side empty customer table.
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

	// Read profile from template
	Scene::Defaults prof = scene.GetDefaults(customerTemplateID_);

	glm::vec3 spawnPos{ spawn2.x, spawn2.y, 0.0f };

	// Spawn an ANIMATED sprite so UVRect animation actually works
	std::vector<glm::vec4> dummyFrames = { glm::vec4(0.f, 0.f, 1.f, 1.f) };

	GameObject* npc = scene.SpawnAnimatedSprite(
		prof.texture,
		spawnPos,
		prof.size,
		dummyFrames,
		0.1f,   // doesn't matter much; AnimationManager will drive frames
		true,
		prof.layer
	);
	if (!npc) return false;

	const int npcID = npc->GetID();

	// Copy shadow settings from the template object (if available)
	if (GameObject* templateObj = scene.GetGameObjectByID(customerTemplateID_)) {
		npc->EnableShadow(templateObj->HasShadow());
		npc->SetShadowSize(templateObj->GetShadowSize());
		npc->SetShadowOffset(templateObj->GetShadowOffset());
		npc->SetShadowOpacity(templateObj->GetShadowOpacity());
	}

	// Tag + attach logic/animations the same way JSON spawning does
	scene.SetObjectTag(npcID, "customer_template");
	scene.AttachLogicForTag(npcID, "customer_template");
	scene.AttachCustomersAnimations(npcID);
	scene.SetAnimation(npcID, "IDLE_FRONT");

	// Apply collider/profile settings
	npc->SetColliderSize(Math::Vector2D(prof.colSize.x, prof.colSize.y));
	npc->SetColliderOffset(Math::Vector2D(prof.colOff.x, prof.colOff.y));
	scene.SetNPCVelocity(npcID, prof.vel.x, prof.vel.y);
	scene.SetObjectTexturePath(npcID, prof.texture);

	//attach UI logic to the customer
	if (auto* ui = logicMgr.AddLogic<CustomerOrderUILogic>(npcID)) {
		ui->Start(scene);
	}

	// Seat + assign target
	chosenTable->SeatCustomer(npcID);
	Math::Vector2D seatWorld = chosenTable->GetCustomerSeatWorld(scene);

	if (auto* npcLogic = logicMgr.GetLogicForObject<SimpleNpcLogic>(npcID)) {
		if (spawnWithInfinitePatience_) {
			npcLogic->SetInfinitePatience(true);
		}
		npcLogic->SetCustomerTableTarget(chosenTableID, seatWorld);
		npcLogic->SetLeaveTarget(spawn2);
	}

	scene.ClampToWalkArea(npc);

	activeCustomers_.push_back(npcID);
	++totalSpawned_;

	// Play customer entering sound effect (release mode only)
#ifndef _DEBUG
	if (AudioManager* audioMgr = scene.GetAudioManager()) {
		audioMgr->PlaySound("sfx_customer_entering", audioMgr->GetVfxVolume() * 0.3f, false);
	}
#endif

	std::cout << "[CustomerManager] Spawned customer " << npcID
		<< " -> table " << chosenTableID << "\n";

	return true;
}


void CustomerManagerSystem::Update(float dt, Scene& scene) {
	if (!scene.IsSimulationActive())
		return;

	if (!cachedTables_) CacheTables(scene);
	if (!cachedTemplate_) CacheTemplate(scene);
	if (!cachedEntries_) CacheEntries(scene);

	CleanupDeadCustomers(scene);

	// Hard cap by number of tables
	const int tableCap = static_cast<int>(customerTableIDs_.size());
	const int targetCount = std::min(maxCustomers_, tableCap);

	spawnTimer_ += dt;

	while ((int)activeCustomers_.size() < targetCount &&
		(totalSpawnLimit_ < 0 || totalSpawned_ < totalSpawnLimit_) &&
		spawnTimer_ >= spawnCooldown_) {
		spawnTimer_ = 0.0f;

		if (!TrySpawnOne(scene)) {
			break; // no empty table/template or total spawn cap reached
		}
	}
}

bool CustomerOrderUILogic::HitTestBubble(Scene& scene, const glm::vec2& worldPos) const {
	if (PointInsideObjectVisualRect(worldPos, scene.GetGameObjectByID(bubbleBG_ID_))) {
		return true;
	}

	if (PointInsideObjectVisualRect(worldPos, scene.GetGameObjectByID(bubbleDish_ID_))) {
		return true;
	}

	return false;
}