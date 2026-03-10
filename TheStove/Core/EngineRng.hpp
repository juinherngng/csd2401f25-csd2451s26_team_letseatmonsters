#pragma once

#include <cstdint>
#include <random>

namespace EngineRng {
	std::mt19937& Get();
	void SetSeed(std::uint32_t seed);
	std::uint32_t GetSeed();
	std::uint32_t CreateSeed();
}
