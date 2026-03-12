/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CustomerManagerLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Declares the CustomerManagerLogic system, which is
					responsible for pairing customers with tables, assigning
					seating targets, and maintaining runtime customer�table
					relationships.

		 All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
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

class Scene;

class CustomerManagerSystem {
public:
	// Construct a customer manager with default spawn and cap settings
	CustomerManagerSystem() = default;

	/**
	  * @brief Run per-frame customer management.
	  * @param dt Delta time for the current frame in seconds.
	  * @param scene Scene context used for table discovery, spawning, and cleanup.
	  */
	void Update(float dt, Scene& scene);

	/**
	 * @brief Reset all cached state when the active scene is cleared/reloaded.
	 */
	void Reset();

	/**
	 * @brief Set the maximum number of simultaneously active customers.
	 * @param n Hard cap applied by the spawn logic.
	 */
	void SetMaxCustomers(int n) {
		maxCustomers_ = n;
	}
	/**
	* @brief Set cooldown time between customer spawns.
	* @param seconds Seconds to wait before spawning the next customer.
	*/
	void SetSpawnCooldown(float seconds) {
		spawnCooldown_ = seconds;
	}
private:
    int maxCustomers_ = 4;				// hard cap on simultaneous customers; set by level design or difficulty settings
    float spawnCooldown_ = 10.0f;       // small delay between spawns
    float spawnTimer_ = 999.0f;         // big so it spawns immediately at start

	std::vector<int> activeCustomers_;  // ids of customers alive
	std::vector<int> customerTableIDs_; // ids of customer tables we discovered
	bool cachedTables_ = false;			// tracks whether table discovery has already run for the current scene

	// Cached template/prefab ID for spawning new customers
	int customerTemplateID_ = -1;

	// Tracks whether template discovery has already run for the current scene
	bool cachedTemplate_ = false;

	// Optional customer spawn entry markers discovered in the level JSON.
	std::vector<int> customerEntryIDs_;

	// Round-robin index for choosing which customer entry marker to spawn from.
	int nextEntryIndex_ = 0;
	bool cachedEntries_ = false;

	// Discover and cache all customer-table entity IDs in the scene
	void CacheTables(Scene& scene);

	// Discover and cache the customer template/prefab entity ID
	void CacheTemplate(Scene& scene);

	// Discover optional customer entry marker IDs (tag: customer_entry).
	void CacheEntries(Scene& scene);

	// Remove stale/dead customer IDs from the active customer list
	void CleanupDeadCustomers(Scene& scene);

	/**
	 * @brief Attempt to spawn exactly one customer if capacity/cooldown allows.
	 * @return True when one customer is spawned successfully, false otherwise.
	 */
	bool TrySpawnOne(Scene& scene);
};
