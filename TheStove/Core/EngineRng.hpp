/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			EngineRng.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Declares the engine-wide random number utility used by gameplay systems.

	Responsibilities:
	  - Expose a shared `std::mt19937` generator instance.
	  - Allow deterministic reseeding for replay/debug flows.
	  - Provide seed query and entropy-based seed creation helpers.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <random>

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

