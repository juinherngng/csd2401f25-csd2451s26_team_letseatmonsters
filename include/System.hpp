#pragma once

#include "Message.hpp"

#include <iostream>

namespace Framework
{
	class SystemInterface
	{
	public:

		///Systems can receive all message send to the Core. 
		///See Message.h for details.
		virtual void SendMessage(Message* message) { (message); };

		///All systems are updated every game frame.
		virtual void Update(float timeslice) = 0;

		///All systems provide a string name for debugging.
		virtual std::string GetName() = 0;

		///Initialize the system.
		virtual void Initialize() {};

		///All systems need a virtual destructor to have their destructor called 
		virtual ~SystemInterface() {}
	};
}