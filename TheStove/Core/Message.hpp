/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Message.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:		Message system for inter-component communication.
					Supports publish/subscribe patterns.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <utility>

namespace CoreFramework {
	/************************************************************************/
	/*!
	\brief
		Message type identifiers using enum class for type safety.
		Each message type has a unique identifier used for routing.
	*/
	/************************************************************************/
	enum class MessageType : uint32_t {
		NONE = 0,
		QUIT,
		COLLIDE,
		TOGGLE_DEBUG_INFO,
		CHARACTER_KEY,
		MOUSE_BUTTON,
		MOUSE_MOVE,
		PLAY_AUDIO,
		STOP_AUDIO
	};

	// Legacy support - can be removed after full migration
	namespace MsgId {
		using MsgIdType = MessageType;

		constexpr MessageType NONE = MessageType::NONE;
		constexpr MessageType QUIT = MessageType::QUIT;
		constexpr MessageType COLLIDE = MessageType::COLLIDE;
		constexpr MessageType TOGGLE_DEBUG_INFO = MessageType::TOGGLE_DEBUG_INFO;
		constexpr MessageType CHARACTER_KEY = MessageType::CHARACTER_KEY;
		constexpr MessageType MOUSE_BUTTON = MessageType::MOUSE_BUTTON;
		constexpr MessageType MOUSE_MOVE = MessageType::MOUSE_MOVE;
		constexpr MessageType PLAY_AUDIO = MessageType::PLAY_AUDIO;
		constexpr MessageType STOP_AUDIO = MessageType::STOP_AUDIO;
	}

	/************************************************************************/
	/*!
	\brief
		Base polymorphic message class.
		All specific messages inherit from this class.
	*/
	/************************************************************************/
	class Message {
	public:
		explicit Message(MessageType id) noexcept : MessageId(id) {
		}
		virtual ~Message() = default;

		// Delete copy operations to prevent slicing
		Message(const Message&) = delete;
		Message& operator=(const Message&) = delete;

		// Allow move operations
		Message(Message&&) noexcept = default;
		Message& operator=(Message&&) noexcept = default;

		MessageType GetType() const noexcept {
			return MessageId;
		}

		MessageType MessageId;
	};

	using EntityId = uint32_t;

	/************************************************************************/
	/*!
	\brief
		Message to signal application shutdown.
	*/
	/************************************************************************/
	struct QuitMessage final : public Message {
		QuitMessage() noexcept : Message(MessageType::QUIT) {
		}
	};

	/************************************************************************/
	/*!
	\brief
		Message to toggle debug information display.
	*/
	/************************************************************************/
	struct ToggleDebugInfoMessage final : public Message {
		explicit ToggleDebugInfoMessage(bool forceState = false, bool hasForcedState = false) noexcept
			: Message(MessageType::TOGGLE_DEBUG_INFO)
			, ForceState(forceState)
			, HasForcedState(hasForcedState) {
		}

		bool ForceState;		// if HasForcedState is true, set debug info to this state
		bool HasForcedState;	// if true, ForceState is used to set the debug info state
	};

	/************************************************************************/
	/*!
	\brief
		Message indicating a collision between two entities.
	*/
	/************************************************************************/
	struct CollideMessage final : public Message {
		CollideMessage(EntityId entityA, EntityId entityB) noexcept
			: Message(MessageType::COLLIDE)
			, EntityA(entityA)
			, EntityB(entityB) {
		}

		EntityId EntityA;
		EntityId EntityB;
	};

	/************************************************************************/
	/*!
	\brief
		Message for character/text input events.
		Character represents the ASCII character code of the key.
	*/
	/************************************************************************/
	struct CharacterKeyMessage final : public Message {
		CharacterKeyMessage(char character, bool isPressed) noexcept
			: Message(MessageType::CHARACTER_KEY)
			, keyCharacter(static_cast<unsigned int>(character))
			, keyIsPressed(isPressed) {
		}

		unsigned int keyCharacter;
		bool keyIsPressed;
	};

	/************************************************************************/
	/*!
	\brief
		Message for mouse button events.
		Button values: 0 = left, 1 = right, 2 = middle
	*/
	/************************************************************************/
	struct MouseButtonMessage final : public Message {
		MouseButtonMessage(int button, bool isPressed, double x, double y) noexcept
			: Message(MessageType::MOUSE_BUTTON)
			, mouseButton(button)
			, mouseIsPressed(isPressed)
			, cursorX(x)
			, cursorY(y) {
		}

		int mouseButton;
		bool mouseIsPressed;
		double cursorX, cursorY; // mouse position when button event occurred
	};

	/************************************************************************/
	/*!
	\brief
		Message for mouse movement events.
		Contains both absolute position and delta movement.
	*/
	/************************************************************************/
	struct MouseMoveMessage final : public Message {
		MouseMoveMessage(double x, double y, double deltaX, double deltaY) noexcept
			: Message(MessageType::MOUSE_MOVE)
			, cursorX(x)
			, cursorY(y)
			, deltaX(deltaX)
			, deltaY(deltaY) {
		}

		double cursorX, cursorY;	// current mouse position
		double deltaX, deltaY;		// change in mouse position since last event
	};

	/************************************************************************/
	/*!
	\brief
		Message for requesting audio playback.
		Contains the sound name and playback parameters.
	*/
	/************************************************************************/
	struct PlayAudioMessage final : public Message {
		PlayAudioMessage(std::string soundName, float volume = 1.0f, bool paused = false) noexcept
			: Message(MessageType::PLAY_AUDIO)
			, soundName(std::move(soundName))
			, volume(volume)
			, paused(paused) {
		}

		std::string soundName;	// name of the sound to play
		float volume;			// playback volume (0.0 to 1.0)
		bool paused;			// whether to start paused
	};

	/************************************************************************/
	/*!
	\brief
		Message for requesting audio stop.
		Contains the sound name to stop, or empty string to stop all.
	*/
	/************************************************************************/
	struct StopAudioMessage final : public Message {
		explicit StopAudioMessage(std::string soundName = "") noexcept
			: Message(MessageType::STOP_AUDIO)
			, soundName(std::move(soundName)) {
		}

		std::string soundName;	// name of the sound to stop (empty = stop all)
	};

	/************************************************************************/
	/*!
	\brief
		Converts a MessageType to a human-readable string.
	\param type
		The message type to convert.
	\return
		String representation of the message type.
	*/
	/************************************************************************/
	inline const char* MessageTypeToString(MessageType type) noexcept {
		switch (type) {
		case MessageType::NONE:					return "NONE";
		case MessageType::QUIT:					return "QUIT";
		case MessageType::COLLIDE:				return "COLLIDE";
		case MessageType::TOGGLE_DEBUG_INFO:	return "TOGGLE_DEBUG_INFO";
		case MessageType::CHARACTER_KEY:		return "CHARACTER_KEY";
		case MessageType::MOUSE_BUTTON:			return "MOUSE_BUTTON";
		case MessageType::MOUSE_MOVE:			return "MOUSE_MOVE";
		case MessageType::PLAY_AUDIO:			return "PLAY_AUDIO";
		case MessageType::STOP_AUDIO:			return "STOP_AUDIO";
		default:								return "UNKNOWN";
		}
	}

	// Legacy support - can be removed after full migration
	inline const char* MsgIdToString(MessageType id) noexcept {
		return MessageTypeToString(id);
	}
}

/************************************************************************/
/*!
\brief
	Hash specialization for MessageType to support unordered_map.
	Required for using MessageType as a key in hash-based containers.
*/
/************************************************************************/
namespace std {
	template<>
	struct hash<CoreFramework::MessageType> {
		size_t operator()(CoreFramework::MessageType type) const noexcept {
			return static_cast<size_t>(type);
		}
	};
}