/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			EngineRng.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Declares a simple wrapper around std::mt19937 to provide a globally accessible RNG instance
					with explicit seed control and lazy initialization. This allows game logic to use a shared RNG without
					needing to manage its lifetime or seeding directly.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <random>

 // A simple wrapper around std::mt19937 to provide a globally accessible RNG instance with explicit seed control and lazy initialization.
 // This allows game logic to use a shared RNG without needing to manage its lifetime or seeding directly.
namespace EngineRng {
	/**
	 * @brief Returns the shared engine RNG instance.
	 *        Lazily seeds the generator on first access.
	 */
	std::mt19937& Get();

	/**
	 * @brief Sets a deterministic seed for the shared RNG.
	 * @param seed Seed value to apply immediately.
	 */
	void SetSeed(std::uint32_t seed);

	/**
	 * @brief Returns the currently active seed.
	 *        If RNG is not initialized yet, it will be lazily seeded first.
	 */
	std::uint32_t GetSeed();

	/**
	 * @brief Creates a new entropy-derived seed using `std::random_device`.
	 * @return Newly generated 32-bit seed value.
	 */
	std::uint32_t CreateSeed();
}
