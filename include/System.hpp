/*
----------------------------------------------------------------------------------------------------
FILE NAME:			System.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:		System interface for game engine systems.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "Message.hpp"

#include <iostream>

/*
	How to create a new system class:
	1) Create a new class in your header that inherits from SystemInterface.
	2) Implement the required methods: Update(float dt), SendMessage(Message*), GetName().
	3) (Optional) Override Initialize() for setup logic.
	4) Add the new system to the CoreEngine using AddSystem().

	Example:

	class MySystem : public Framework::SystemInterface 
	{
    public:
       void Initialize() override {  setup code  }
       void Update(float dt) override {  per-frame logic  }
       void SendMessage(Framework::Message* msg) override {  handle messages }
       std::string GetName() override { return "MySystem"; }
    };

	In main():

		engine.AddSystem(new MySystem());

	Actual example in Main.cpp as well.
*/

namespace CoreFramework
{
	class SystemInterface
	{
	public:

		///Systems can receive all message send to the Core. 
		///See Message.h for details.
		virtual void SendMessage(Message* message) { (message); };

		///All systems are updated every game frame.
		virtual void Update(float timeSlice) = 0;

		///All systems provide a string name for debugging.
		virtual std::string GetName() = 0;

		///Initialize the system.
		virtual void Initialize() {};

		///All systems need a virtual destructor to have their destructor called 
		virtual ~SystemInterface() {}

		// For performance tracking
		float lastDt = 0.0f; // Tracks the last frame delta of this system
	};
}