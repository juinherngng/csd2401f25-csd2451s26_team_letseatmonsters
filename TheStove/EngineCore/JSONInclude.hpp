#pragma once

#if defined(_MSC_VER)
// stop almost all warnings from the next include
#pragma warning(push, 0)
#pragma warning(disable: 26495) // uninitialized member (type.6)
#pragma warning(disable: 26812) // unscoped enum
#endif

#include "EngineCore/JSON.hpp" // your nlohmann single-header

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
