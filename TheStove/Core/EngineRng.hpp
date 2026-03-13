/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			EngineRng.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Declares a simple wrapper around std::mt19937 to provide a globally accessible RNG instance
					with explicit seed control and lazy initialization. This allows game logic to use a shared RNG without
					needing to manage its lifetime or seeding directly.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <random>

 // A simple wrapper around std::mt19937 to provide a globally accessible RNG instance with explicit seed control and lazy initialization.
 // This allows game logic to use a shared RNG without needing to manage its lifetime or seeding directly.
namespace EngineRng {
	std::mt19937& Get();
	void SetSeed(std::uint32_t seed);
	std::uint32_t GetSeed();
	std::uint32_t CreateSeed();
}
