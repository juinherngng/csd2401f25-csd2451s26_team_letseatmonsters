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

	class Message
	{
	public:
		Message(MsgId::MsgIdType id) : MessageId(id) {}
		MsgId::MsgIdType MessageId;
		virtual ~Message() {}
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