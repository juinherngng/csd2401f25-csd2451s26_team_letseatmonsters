/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CustomerManagerLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu	(85%)
 CO-AUTHORS:        Seah Wang Hua, wanghua.seah@digipen.edu (10%)
					Yat Chun Wee, y.chunwee@digipen.edu		(5%)

 DESCRIPTION:       Declares the CustomerManagerLogic system, which is
					responsible for pairing customers with tables, assigning
					seating targets, and maintaining runtime customer and table
					relationships.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

class Scene;

/**
 * @brief Scene-level system that seats NPC customers at customer tables.
 *
 * On the first Update, it:
 *  - Finds all GameObjects with CustomerTableLogic
 *  - Finds all GameObjects with SimpleNpcLogic (treated as customers)
 *  - Pairs them 1:1 in order, calls SeatCustomer() on the table,
 *    and SetCustomerTableTarget() on the NPC so they walk to the seat.
 */

class CustomerManagerSystem {
public:
	CustomerManagerSystem() = default;

	void Update(float dt, Scene& scene);
	void Reset();

	void SetMaxCustomers(int n) {
		maxCustomers_ = n;
	}

	void SetTotalSpawnLimit(int n) {
		totalSpawnLimit_ = n;
	}

	void ClearTotalSpawnLimit() {
		totalSpawnLimit_ = -1;
	}

	void SetSpawnedCustomersInfinitePatience(bool enabled) {
		spawnWithInfinitePatience_ = enabled;
	}

	void SetSpawnCooldown(float seconds) {
		if (seconds < 0.0f) seconds = 0.0f;

		startSpawnCooldown_ = seconds;
		spawnCooldown_ = seconds;
		minSpawnCooldown_ = seconds;
		spawnCooldownStep_ = 0.0f;
		initialSpawnDelay_ = 0.0f;
		cooldownRampStopTime_ = -1.0f;
	}

	void ConfigureSpawnCurve(float initialDelay,
		float firstRepeatCooldown,
		float minCooldown,
		float cooldownStepPerSpawn,
		float rampStopTimeSeconds) {
		if (initialDelay < 0.0f) initialDelay = 0.0f;
		if (firstRepeatCooldown < 0.0f) firstRepeatCooldown = 0.0f;
		if (minCooldown < 0.0f) minCooldown = 0.0f;
		if (cooldownStepPerSpawn < 0.0f) cooldownStepPerSpawn = 0.0f;

		if (minCooldown > firstRepeatCooldown) {
			minCooldown = firstRepeatCooldown;
		}

		initialSpawnDelay_ = initialDelay;
		startSpawnCooldown_ = firstRepeatCooldown;
		spawnCooldown_ = firstRepeatCooldown;
		minSpawnCooldown_ = minCooldown;
		spawnCooldownStep_ = cooldownStepPerSpawn;
		cooldownRampStopTime_ = rampStopTimeSeconds;
	}

	void SetSpawningEnabled(bool enabled) {
		spawningEnabled_ = enabled;
	}

	bool IsSpawningEnabled() const {
		return spawningEnabled_;
	}

private:
	int maxCustomers_ = 16;

	// Current live cooldown used for the NEXT spawn after the first customer.
	float spawnCooldown_ = 10.0f;

	// Initial recurring cooldown after the first customer has already appeared.
	float startSpawnCooldown_ = 10.0f;

	// Lower bound once rush hour is reached.
	float minSpawnCooldown_ = 10.0f;

	// Amount removed from cooldown after each successful spawn during the ramp.
	float spawnCooldownStep_ = 0.0f;

	// Delay before the very first customer appears.
	float initialSpawnDelay_ = 0.0f;

	// Stop reducing cooldown after this many seconds of level time.
	// Use negative value for "never stop by time".
	float cooldownRampStopTime_ = -1.0f;

	// Time accumulated toward the next spawn.
	float spawnTimer_ = 0.0f;

	// Elapsed active gameplay time in this level.
	float levelElapsed_ = 0.0f;

	// Whether the first customer has already spawned.
	bool hasSpawnedAtLeastOnce_ = false;

	int totalSpawnLimit_ = -1;
	int totalSpawned_ = 0;
	bool spawnWithInfinitePatience_ = false;

	std::vector<int> activeCustomers_;
	std::vector<int> customerTableIDs_;
	bool cachedTables_ = false;

	int customerTemplateID_ = -1;
	bool cachedTemplate_ = false;

	std::vector<int> customerEntryIDs_;
	int nextEntryIndex_ = 0;
	bool cachedEntries_ = false;

	void CacheTables(Scene& scene);
	void CacheTemplate(Scene& scene);
	void CacheEntries(Scene& scene);
	void CleanupDeadCustomers(Scene& scene);
	bool TrySpawnOne(Scene& scene);
	bool spawningEnabled_ = true;
};
