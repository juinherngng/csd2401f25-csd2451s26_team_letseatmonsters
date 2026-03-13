/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			EngineRng.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Implements the EngineRng namespace, which provides a globally accessible random number generator
					and seed management for the game. This allows for consistent random behavior across different systems
					and the ability to set a specific seed for reproducibility.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "EngineRng.hpp"

#include <atomic>
#include <random>

namespace {
	std::mt19937 sRng;
	std::uint32_t sSeed = 0;
	std::atomic<bool> sSeeded = false;

	void EnsureSeeded() {
		if (sSeeded.load()) {
			return;
		}

		std::random_device rd;
		const std::uint32_t seed = (rd() << 1) ^ rd();
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
		sSeed = seed;
		sRng.seed(seed);
		sSeeded.store(true);
	}

	std::uint32_t GetSeed() {
		EnsureSeeded();
		return sSeed;
	}

	std::uint32_t CreateSeed() {
		std::random_device rd;
		return (rd() << 1) ^ rd();
	}
}
