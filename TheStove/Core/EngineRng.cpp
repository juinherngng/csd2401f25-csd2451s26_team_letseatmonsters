/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			EngineRng.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Implements the engine-wide random number utility declared in EngineRng.hpp.

	Behavior:
	  - Keeps one shared RNG (`std::mt19937`) for all systems.
	  - Performs lazy one-time seeding on first use.
	  - Supports explicit deterministic reseeding for replay consistency.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "EngineRng.hpp"

#include <atomic>
#include <random>

namespace {
	// Shared RNG state for the entire engine process.
	std::mt19937 sRng;
	std::uint32_t sSeed = 0;
	std::atomic<bool> sSeeded = false;

	// Ensures RNG is seeded exactly once (lazy init path).
	void EnsureSeeded() {
		if (sSeeded.load()) {
			return;
		}

		std::random_device rd;
		const std::uint32_t seed = (rd() << 1) ^ rd(); // lightweight mix of two entropy pulls
		sSeed = seed;
		sRng.seed(seed);
		sSeeded.store(true);
	}
}

namespace EngineRng {
	std::mt19937& Get() {
		EnsureSeeded();
		return sRng;
	}

	void SetSeed(std::uint32_t seed) {
		// Deterministic path used by replay/bootstrap flows.
		sSeed = seed;
		sRng.seed(seed);
		sSeeded.store(true);
	}

	std::uint32_t GetSeed() {
		EnsureSeeded();
		return sSeed;
	}

	std::uint32_t CreateSeed() {
		// Non-deterministic seed generation helper.
		std::random_device rd;
		return (rd() << 1) ^ rd();
	}
}