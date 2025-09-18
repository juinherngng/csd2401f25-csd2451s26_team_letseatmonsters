#pragma once

namespace Framework
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
}