/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			System.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:		System interface for game engine systems.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <iostream>

namespace CoreFramework {
	class SystemInterface {
	public:

		/************************************************************************/
		/*!
		\brief
			All systems are updated every game frame.
		\param dt
			Delta time in seconds since last frame.
		*/
		/************************************************************************/
		virtual void Update(float dt) = 0;

		/************************************************************************/
		/*!
		\brief
			All systems provide a string name for debugging.
		\return
			Name of the system.
		*/
		/************************************************************************/
		virtual std::string GetName() = 0;

		/************************************************************************/
		/*!
		\brief
			Initialize the system.
		*/
		/************************************************************************/
		virtual void Initialize() {
		};

		/************************************************************************/
		/*!
		\brief
			All systems need a virtual destructor to have their destructor called.
		*/
		/************************************************************************/
		virtual ~SystemInterface() {
		}

		// For performance tracking
		float lastDt = 0.0f; // Tracks the last frame delta of this system
	};
}
