/*
----------------------------------------------------------------------------------------------------
FILE NAME:			Message.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:		Message system for inter-component communication.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <cstdint>
#include <utility>

namespace CoreFramework
{
	namespace MsgId
	{
		enum MsgIdType
		{
			NONE,
			QUIT,
			COLLIDE,
			TOGGLE_DEBUG_INFO,
			CHARACTER_KEY,
			MOUSE_BUTTON,
			MOUSE_MOVE
		};
	}

	// base polymorphic message class
	class Message
	{
	public:
		explicit Message(MsgId::MsgIdType id) : MessageId(id) {}
		virtual ~Message() {}

		MsgId::MsgIdType MessageId;
	};

	using EntityId = uint32_t;

	// specific message types
	struct QuitMessage final : public Message
	{
		QuitMessage() : Message(MsgId::QUIT) {}
	};

	struct ToggleDebugInfoMessage final : public Message
	{
		explicit ToggleDebugInfoMessage(bool forceState = false, bool hasForcedState = false) 
			: Message(MsgId::TOGGLE_DEBUG_INFO), ForceState(forceState), HasForcedState(hasForcedState) {}

		bool ForceState;		// if HasForcedState is true, set debug info to this state
		bool HasForcedState;	// if true, ForceState is used to set the debug info state
	};

	// entityA and entityB are the two entities that collided
	struct CollideMessage final : public Message
	{
		CollideMessage(EntityId entityA, EntityId entityB)
			: Message(MsgId::COLLIDE), EntityA(entityA), EntityB(entityB) {}
		EntityId EntityA;
		EntityId EntityB;
	};

	// character: ASCII character code of the key
	struct CharacterKeyMessage final : public Message
	{
		CharacterKeyMessage(char character, bool isPressed)
			: Message(MsgId::CHARACTER_KEY), keyCharacter(character), keyIsPressed(isPressed) {}
		char keyCharacter;
		bool keyIsPressed;
	};

	// button: 0 = left, 1 = right, 2 = middle
	struct MouseButtonMessage final : public Message
	{
		MouseButtonMessage(int button, bool isPressed, double x, double y)
			: Message(MsgId::MOUSE_BUTTON), mouseButton(button), mouseIsPressed(isPressed), cursorX(x), cursorY(y) {}
		int mouseButton;
		bool mouseIsPressed;
		double cursorX, cursorY; // mouse position when button event occurred
	};

	// deltaX and deltaY are the change in mouse position since the last mouse move event
	struct MouseMoveMessage final : public Message
	{
		MouseMoveMessage(double x, double y, double deltaX, double deltaY)
			: Message(MsgId::MOUSE_MOVE), cursorX(x), cursorY(y), deltaX(deltaX), deltaY(deltaY) {
		}
		double cursorX, cursorY;		 // current mouse position
		double deltaX, deltaY; // change in mouse position since last event
	};

	inline char const* MsgIdToString(MsgId::MsgIdType id)
	{
		switch (id)
		{
			case MsgId::NONE:			   return "NONE";
			case MsgId::QUIT:			   return "QUIT";
			case MsgId::COLLIDE:		   return "COLLIDE";
			case MsgId::TOGGLE_DEBUG_INFO: return "TOGGLE_DEBUG_INFO";
			case MsgId::CHARACTER_KEY:	   return "CHARACTER_KEY";
			case MsgId::MOUSE_BUTTON:	   return "MOUSE_BUTTON";
			case MsgId::MOUSE_MOVE:		   return "MOUSE_MOVE";
			default:					   return "UNKNOWN";
		}
	}
}

// note to self:
// make Message a base class and use inheritance for message data.