/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			System.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (60%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	    (40%)

 DESCRIPTION:		System interface for game engine systems.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <iostream>

namespace CoreFramework {
	class SystemInterface {
	public:
		/**
		 * @brief Updates the system once per frame.
		 * @param dt Delta time in seconds since the previous frame.
		 */
		virtual void Update(float dt) = 0;

		/**
		 * @brief Returns the system name for debugging and profiling output.
		 * @return Human-readable system name.
		 */
		virtual std::string GetName() = 0;

		/**
		 * @brief Performs any one-time setup required before updates begin.
		 */
		virtual void Initialize() {
			// Default systems have no explicit initialization work.
		};

		/**
		 * @brief Destroys the system through the polymorphic interface.
		 */
		virtual ~SystemInterface() {
			// Keep destruction polymorphic for derived engine systems.
		}

		float lastDt = 0.0f; // Tracks the most recent frame delta processed by this system.
	};
}
