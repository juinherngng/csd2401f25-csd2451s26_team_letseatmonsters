/*
----------------------------------------------------------------------------------------------------
 FILE NAME:          Precompiled.hpp
 PROJECT NAME:       Project GAM200
 AUTHOR:             Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:        Precompiled header for commonly used system and third-party includes.
					 Includes OpenGL, GLFW, GLM, and ImGui headers for faster compilation.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

// Standard library headers (commonly used across the project)
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

// Third-party libraries - OpenGL/GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Third-party libraries - GLM (math)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Third-party libraries - ImGui (UI)
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>