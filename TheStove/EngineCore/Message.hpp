/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Message.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (60%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	    (40%)

 DESCRIPTION:		Message system for inter-component communication.
					Supports publish/subscribe patterns.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <utility>

namespace CoreFramework {
	/**
	 * @brief Identifies each routed message type in a type-safe way.
	 */
	enum class MessageType : uint32_t {
		NONE = 0,
		QUIT,
		COLLIDE,
		TOGGLE_DEBUG_INFO,
		CHARACTER_KEY,
		MOUSE_BUTTON,
		MOUSE_MOVE,
		PLAY_AUDIO,
		STOP_AUDIO,
		PLAY_AUDIO_3D,
		SCENE_FLOW_STATE_CHANGED,
		LEVEL_LOAD_QUEUED,
		LEVEL_LOADED,
		PAUSE_OVERLAY_CHANGED,
		CUTSCENE_SKIPPED
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
		constexpr MessageType PLAY_AUDIO_3D = MessageType::PLAY_AUDIO_3D;
		constexpr MessageType SCENE_FLOW_STATE_CHANGED = MessageType::SCENE_FLOW_STATE_CHANGED;
		constexpr MessageType LEVEL_LOAD_QUEUED = MessageType::LEVEL_LOAD_QUEUED;
		constexpr MessageType LEVEL_LOADED = MessageType::LEVEL_LOADED;
		constexpr MessageType PAUSE_OVERLAY_CHANGED = MessageType::PAUSE_OVERLAY_CHANGED;
		constexpr MessageType CUTSCENE_SKIPPED = MessageType::CUTSCENE_SKIPPED;
	}

	/**
	 * @brief Base polymorphic message type used by the message bus.
	 */
	class Message {
	public:
		/**
		 * @brief Initializes the base message with a concrete message type.
		 * @param id Type identifier stored for downstream routing.
		 */
		explicit Message(MessageType id) noexcept : MessageId(id) {}

		/**
		 * @brief Destroys the message through a polymorphic base pointer.
		 */
		virtual ~Message() = default;

		// Delete copy operations to prevent slicing
		Message(const Message&) = delete;
		Message& operator=(const Message&) = delete;

		// Allow move operations
		Message(Message&&) noexcept = default;
		Message& operator=(Message&&) noexcept = default;

		/**
		 * @brief Returns the stored runtime message type.
		 * @return Message type used for dispatch.
		 */
		MessageType GetType() const noexcept {
			// Expose the cached type identifier without altering the message payload.
			return MessageId;
		}

		MessageType MessageId;
	};

	using EntityId = uint32_t;

	/**
	 * @brief Signals that the application should shut down.
	 */
	struct QuitMessage final : public Message {
		/**
		 * @brief Creates an application-quit message.
		 */
		QuitMessage() noexcept : Message(MessageType::QUIT) {}
	};

	/**
	 * @brief Requests that debug information visibility be toggled or forced.
	 */
	struct ToggleDebugInfoMessage final : public Message {
		/**
		 * @brief Creates a debug-visibility toggle request.
		 * @param forceState Requested forced debug state when `hasForcedState` is true.
		 * @param hasForcedState Indicates whether the toggle should act as a forced set.
		 */
		explicit ToggleDebugInfoMessage(bool forceState = false, bool hasForcedState = false) noexcept
			: Message(MessageType::TOGGLE_DEBUG_INFO)
			, ForceState(forceState)
			, HasForcedState(hasForcedState) {}

		bool ForceState;		// if HasForcedState is true, set debug info to this state
		bool HasForcedState;	// if true, ForceState is used to set the debug info state
	};

	/**
	 * @brief Reports a collision between two entities.
	 */
	struct CollideMessage final : public Message {
		/**
		 * @brief Creates a collision event between two entities.
		 * @param entityA First entity participating in the collision.
		 * @param entityB Second entity participating in the collision.
		 */
		CollideMessage(EntityId entityA, EntityId entityB) noexcept
			: Message(MessageType::COLLIDE)
			, EntityA(entityA)
			, EntityB(entityB) {}

		EntityId EntityA;
		EntityId EntityB;
	};

	/**
	 * @brief Carries character-input events for text-style input handling.
	 */
	struct CharacterKeyMessage final : public Message {
		/**
		 * @brief Creates a character-input event.
		 * @param character Character code associated with the input.
		 * @param isPressed Indicates whether the key transitioned to pressed state.
		 */
		CharacterKeyMessage(char character, bool isPressed) noexcept
			: Message(MessageType::CHARACTER_KEY)
			, keyCharacter(static_cast<unsigned int>(character))
			, keyIsPressed(isPressed) {}

		unsigned int keyCharacter;
		bool keyIsPressed;
	};

	/**
	 * @brief Carries a mouse-button transition and cursor snapshot.
	 */
	struct MouseButtonMessage final : public Message {
		/**
		 * @brief Creates a mouse-button event snapshot.
		 * @param button Button index associated with the input event.
		 * @param isPressed Indicates whether the button transitioned to pressed state.
		 * @param x Cursor x position at the time of the event.
		 * @param y Cursor y position at the time of the event.
		 */
		MouseButtonMessage(int button, bool isPressed, double x, double y) noexcept
			: Message(MessageType::MOUSE_BUTTON)
			, mouseButton(button)
			, mouseIsPressed(isPressed)
			, cursorX(x)
			, cursorY(y) {}

		int mouseButton;
		bool mouseIsPressed;
		double cursorX, cursorY; // mouse position when button event occurred
	};

	/**
	 * @brief Carries cursor position and delta movement for one mouse-move event.
	 */
	struct MouseMoveMessage final : public Message {
		/**
		 * @brief Creates a mouse-movement event snapshot.
		 * @param x Current cursor x position.
		 * @param y Current cursor y position.
		 * @param deltaX Cursor movement along the x axis since the last event.
		 * @param deltaY Cursor movement along the y axis since the last event.
		 */
		MouseMoveMessage(double x, double y, double deltaX, double deltaY) noexcept
			: Message(MessageType::MOUSE_MOVE)
			, cursorX(x)
			, cursorY(y)
			, deltaX(deltaX)
			, deltaY(deltaY) {}

		double cursorX, cursorY;	// current mouse position
		double deltaX, deltaY;		// change in mouse position since last event
	};

	/**
	 * @brief Requests playback of a non-spatialized sound.
	 */
	struct PlayAudioMessage final : public Message {
		/**
		 * @brief Creates a request to start audio playback.
		 * @param soundName Logical sound identifier to play.
		 * @param volume Playback volume in the inclusive range `[0, 1]`.
		 * @param paused Indicates whether playback should begin paused.
		 */
		PlayAudioMessage(std::string soundName, float volume = 1.0f, bool paused = false) noexcept
			: Message(MessageType::PLAY_AUDIO)
			, soundName(std::move(soundName))
			, volume(volume)
			, paused(paused) {}

		std::string soundName;	// name of the sound to play
		float volume;			// playback volume (0.0 to 1.0)
		bool paused;			// whether to start paused
	};

	/**
	 * @brief Requests that one sound or all tracked sounds be stopped.
	 */
	struct StopAudioMessage final : public Message {
		/**
		 * @brief Creates a request to stop one sound or all sounds.
		 * @param soundName Logical sound identifier to stop, or empty to stop all tracked sounds.
		 */
		explicit StopAudioMessage(std::string soundName = "") noexcept
			: Message(MessageType::STOP_AUDIO)
			, soundName(std::move(soundName)) {}

		std::string soundName;	// name of the sound to stop (empty = stop all)
	};

	/**
	 * @brief Requests playback of a spatialized sound source.
	 */
	struct PlayAudio3DMessage final : public Message {
		/**
		 * @brief Creates a request to play a spatialized sound.
		 * @param soundName Logical sound identifier to play.
		 * @param posX World x position of the sound source.
		 * @param posY World y position of the sound source.
		 * @param posZ World z position of the sound source.
		 * @param volume Playback volume in the inclusive range `[0, 1]`.
		 * @param minDistance Distance at which attenuation begins.
		 * @param maxDistance Distance at which attenuation reaches its maximum.
		 * @param paused Indicates whether playback should begin paused.
		 */
		PlayAudio3DMessage(std::string soundName, float posX, float posY, float posZ = 0.0f,
			float volume = 1.0f, float minDistance = 1.0f, float maxDistance = 50.0f, bool paused = false) noexcept
			: Message(MessageType::PLAY_AUDIO_3D)
			, soundName(std::move(soundName))
			, posX(posX)
			, posY(posY)
			, posZ(posZ)
			, volume(volume)
			, minDistance(minDistance)
			, maxDistance(maxDistance)
			, paused(paused) {}

		std::string soundName;	// name of the sound to play
		float posX, posY, posZ;	// 3D world position of the sound source
		float volume;			// playback volume (0.0 to 1.0)
		float minDistance;		// distance at which sound starts to attenuate
		float maxDistance;		// distance at which sound is fully attenuated
		bool paused;			// whether to start paused
	};

	/**
	 * @brief Announces a scene-flow state change.
	 */
	struct SceneFlowStateChangedMessage final : public Message {
		/**
		 * @brief Creates a scene-flow state transition event.
		 * @param stateName Human-readable scene-flow state name.
		 * @param simulationActive Indicates whether gameplay simulation should currently run.
		 */
		SceneFlowStateChangedMessage(std::string stateName, bool simulationActive) noexcept
			: Message(MessageType::SCENE_FLOW_STATE_CHANGED)
			, stateName(std::move(stateName))
			, simulationActive(simulationActive) {}

		std::string stateName;
		bool simulationActive;
	};

	/**
	 * @brief Announces that a level load has been queued.
	 */
	struct LevelLoadQueuedMessage final : public Message {
		/**
		 * @brief Creates a deferred level-load request event.
		 * @param levelPath Path of the level that was queued for loading.
		 * @param activateSimulation Indicates whether simulation should activate after loading.
		 */
		LevelLoadQueuedMessage(std::string levelPath, bool activateSimulation) noexcept
			: Message(MessageType::LEVEL_LOAD_QUEUED)
			, levelPath(std::move(levelPath))
			, activateSimulation(activateSimulation) {}

		std::string levelPath;
		bool activateSimulation;
	};

	/**
	 * @brief Announces that a level has finished loading.
	 */
	struct LevelLoadedMessage final : public Message {
		/**
		 * @brief Creates a level-loaded notification event.
		 * @param levelPath Path of the level that finished loading.
		 * @param simulationActive Indicates whether simulation is active after the load.
		 */
		LevelLoadedMessage(std::string levelPath, bool simulationActive) noexcept
			: Message(MessageType::LEVEL_LOADED)
			, levelPath(std::move(levelPath))
			, simulationActive(simulationActive) {}

		std::string levelPath;
		bool simulationActive;
	};

	/**
	 * @brief Announces that the pause overlay was opened or closed.
	 */
	struct PauseOverlayChangedMessage final : public Message {
		/**
		 * @brief Creates a pause-overlay visibility event.
		 * @param isOpen Indicates whether the pause overlay is currently visible.
		 */
		explicit PauseOverlayChangedMessage(bool isOpen) noexcept
			: Message(MessageType::PAUSE_OVERLAY_CHANGED)
			, isOpen(isOpen) {}

		bool isOpen;
	};

	/**
	 * @brief Announces that a cutscene was skipped.
	 */
	struct CutsceneSkippedMessage final : public Message {
		/**
		 * @brief Creates a cutscene-skip event.
		 * @param transitionedCutscene Indicates whether the skip also triggered a cutscene transition.
		 * @param targetLevelPath Level path targeted after the skip completes.
		 */
		CutsceneSkippedMessage(bool transitionedCutscene, std::string targetLevelPath) noexcept
			: Message(MessageType::CUTSCENE_SKIPPED)
			, transitionedCutscene(transitionedCutscene)
			, targetLevelPath(std::move(targetLevelPath)) {}

		bool transitionedCutscene;
		std::string targetLevelPath;
	};

	/**
	 * @brief Converts a message type into a human-readable label.
	 * @param type Message type to convert.
	 * @return String representation of the supplied message type.
	 */
	inline const char* MessageTypeToString(MessageType type) noexcept {
		// Keep the string mapping centralized so logs and debug UI stay consistent.
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
		case MessageType::PLAY_AUDIO_3D:		return "PLAY_AUDIO_3D";
		case MessageType::SCENE_FLOW_STATE_CHANGED: return "SCENE_FLOW_STATE_CHANGED";
		case MessageType::LEVEL_LOAD_QUEUED:    return "LEVEL_LOAD_QUEUED";
		case MessageType::LEVEL_LOADED:         return "LEVEL_LOADED";
		case MessageType::PAUSE_OVERLAY_CHANGED: return "PAUSE_OVERLAY_CHANGED";
		case MessageType::CUTSCENE_SKIPPED:     return "CUTSCENE_SKIPPED";
		default:								return "UNKNOWN";
		}
	}

	// Legacy support - can be removed after full migration
	/**
	 * @brief Converts a legacy message identifier alias into a human-readable label.
	 * @param id Message identifier to convert.
	 * @return String representation of the supplied message type.
	 */
	inline const char* MsgIdToString(MessageType id) noexcept {
		// Forward the legacy helper to the canonical conversion function.
		return MessageTypeToString(id);
	}
}

namespace std {
	template<>
	struct hash<CoreFramework::MessageType> {
		/**
		 * @brief Hashes a message type for use in unordered containers.
		 * @param type Message type to hash.
		 * @return Hash value derived from the underlying enum value.
		 */
		size_t operator()(CoreFramework::MessageType type) const noexcept {
			// Reuse the enum's integral value as a stable lightweight hash.
			return static_cast<size_t>(type);
		}
	};
}
