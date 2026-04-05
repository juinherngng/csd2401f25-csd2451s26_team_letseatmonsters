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
	/**
	 * @brief Constructs a customer-manager system with default spawn settings.
	 */
	CustomerManagerSystem() = default;

	/**
	 * @brief Advances customer spawning and seating for one frame.
	 * @param dt Delta time for the current frame.
	 * @param scene Active scene that owns the customer tables and NPCs.
	 */
	void Update(float dt, Scene& scene);

	/**
	 * @brief Clears cached runtime state and restores default spawn tuning.
	 */
	void Reset();

	/**
	 * @brief Sets the maximum number of concurrent customers this system may maintain.
	 * @param n Desired active-customer cap.
	 */
	void SetMaxCustomers(int n) {
		// Store the caller-authored cap used when computing the target customer count.
		maxCustomers_ = n;
	}

	/**
	 * @brief Limits how many customers may be spawned across the entire level.
	 * @param n Total spawn cap, or a negative value to allow unlimited spawns.
	 */
	void SetTotalSpawnLimit(int n) {
		// Record the lifetime spawn cap used by TrySpawnOne().
		totalSpawnLimit_ = n;
	}

	/**
	 * @brief Removes any previously configured lifetime spawn cap.
	 */
	void ClearTotalSpawnLimit() {
		// Use -1 as the sentinel for "no total spawn limit".
		totalSpawnLimit_ = -1;
	}

	/**
	 * @brief Toggles whether newly spawned customers should ignore patience drain.
	 * @param enabled True to give future spawns infinite patience.
	 */
	void SetSpawnedCustomersInfinitePatience(bool enabled) {
		// Persist the flag so future spawns can push it onto the spawned NPC logic.
		spawnWithInfinitePatience_ = enabled;
	}

	/**
	 * @brief Sets a fixed cooldown for all recurring spawns.
	 * @param seconds Cooldown to use between customers after clamping to non-negative values.
	 */
	void SetSpawnCooldown(float seconds) {
		// Clamp invalid negative cooldowns before storing the fixed-timing profile.
		if (seconds < 0.0f) seconds = 0.0f;

		startSpawnCooldown_ = seconds;
		spawnCooldown_ = seconds;
		minSpawnCooldown_ = seconds;
		spawnCooldownStep_ = 0.0f;
		initialSpawnDelay_ = 0.0f;
		cooldownRampStopTime_ = -1.0f;
	}

	/**
	 * @brief Configures the time-based spawn ramp used across the level.
	 * @param initialDelay Delay before the first customer appears.
	 * @param firstRepeatCooldown Cooldown used after the first successful spawn.
	 * @param minCooldown Lower bound for the recurring cooldown.
	 * @param cooldownStepPerSpawn Amount subtracted after each successful spawn during the ramp.
	 * @param rampStopTimeSeconds Time limit after which cooldown reduction stops, or negative to never stop.
	 */
	void ConfigureSpawnCurve(float initialDelay,
		float firstRepeatCooldown,
		float minCooldown,
		float cooldownStepPerSpawn,
		float rampStopTimeSeconds) {
		// Clamp authored timing values so the runtime curve stays valid.
		if (initialDelay < 0.0f) initialDelay = 0.0f;
		if (firstRepeatCooldown < 0.0f) firstRepeatCooldown = 0.0f;
		if (minCooldown < 0.0f) minCooldown = 0.0f;
		if (cooldownStepPerSpawn < 0.0f) cooldownStepPerSpawn = 0.0f;

		// Ensure the minimum cooldown never exceeds the first recurring cooldown.
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

	/**
	 * @brief Enables or disables runtime spawning without clearing cached data.
	 * @param enabled True to allow Update() to spawn new customers.
	 */
	void SetSpawningEnabled(bool enabled) {
		// Keep the flag simple so external systems can pause spawning instantly.
		spawningEnabled_ = enabled;
	}

	/**
	 * @brief Returns whether the system is currently allowed to spawn new customers.
	 * @return True when spawning is enabled.
	 */
	bool IsSpawningEnabled() const {
		// Expose the live toggle for UI and gameplay systems.
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

	/**
	 * @brief Caches all customer-table object IDs found in the scene.
	 * @param scene Active scene to scan.
	 */
	void CacheTables(Scene& scene);

	/**
	 * @brief Finds and caches the authored customer template object.
	 * @param scene Active scene to scan.
	 */
	void CacheTemplate(Scene& scene);

	/**
	 * @brief Caches all authored customer-entry spawn points.
	 * @param scene Active scene to scan.
	 */
	void CacheEntries(Scene& scene);

	/**
	 * @brief Removes despawned or already-leaving customers from the active list.
	 * @param scene Active scene that owns the tracked customers.
	 */
	void CleanupDeadCustomers(Scene& scene);

	/**
	 * @brief Attempts to spawn and seat one new customer.
	 * @param scene Active scene containing the template, entries, and tables.
	 * @return True if a customer was spawned successfully.
	 */
	bool TrySpawnOne(Scene& scene);
	bool spawningEnabled_ = true;
};
