/*
----------------------------------------------------------------------------------------------------
FILE NAME:			Core.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:		The core engine managing the game loop and systems.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "System.hpp"
#include "MessageBus.hpp"

#include <vector>
#include <chrono>
#include <memory>
#include <utility>

namespace CoreFramework
{
	class CoreEngine
	{
	public:
		/************************************************************************/
		/*!
		\brief
			Constructs the CoreEngine. Sets initial runtime state.
		*/
		/************************************************************************/
		CoreEngine();
		/************************************************************************/
		/*!
		\brief
			Destructs the CoreEngine. Systems are automatically destroyed.
		*/
		/************************************************************************/
		~CoreEngine();

		/************************************************************************/
		/*!
		\brief
			Runs one frame of the game loop: computes dt, updates systems,
			processes MessageBus queue, and records performance statistics.
		*/
		/************************************************************************/
		void GameLoop();

		/************************************************************************/
		/*!
		\brief
			Destroys all registered systems in reverse order of addition.
			Frees their memory.
		*/
		/************************************************************************/
		void DestroySystems();

		/************************************************************************/
		/*!
		\brief
			Adds (registers) a new system to the engine. CoreEngine takes ownership
			and will manage its lifetime via unique_ptr.
		\param system
			Unique pointer to a system.
		*/
		/************************************************************************/
		void AddSystem(std::unique_ptr<SystemInterface> system);

		/************************************************************************/
		/*!
		\brief
			Initializes all registered systems by calling their Initialize() method.
		*/
		/************************************************************************/
		void Initialize();

		/************************************************************************/
		/*!
		\brief
			Returns the last computed frames-per-second value.
		\return
			FPS as a float.
		*/
		/************************************************************************/
		float GetFPS() const { return fps; }

		/************************************************************************/
		/*!
		\brief
			Returns the last computed delta time value.
		\return
			Delta time in seconds as a float.
		*/
		/************************************************************************/
		float GetDeltaTime() const { return deltaTime; }

		/************************************************************************/
		/*!
		\brief
			Returns a read-only view of all systems (for debugging/inspection).
		\return
			Const reference to the systems vector.
		*/
		/************************************************************************/
		const std::vector<std::unique_ptr<SystemInterface>>& GetSystems() const { return Systems; }

		/************************************************************************/
		/*!
		\brief
			Fetches a pointer to the first system of type T (runtime checked).
		\return
			Pointer to system if found, else nullptr.
		\tparam T
			Concrete system type derived from SystemInterface.
		*/
		/************************************************************************/
		template<typename T>
		T* GetSystem()
		{
			for (auto& s : Systems)
				if (auto* casted = dynamic_cast<T*>(s.get()))
					return casted;
			return nullptr;
		}

		/************************************************************************/
		/*!
		\brief
			Const-qualified overload of GetSystem().
		\return
			Const pointer to system if found, else nullptr.
		*/
		/************************************************************************/
		template<typename T>
		T const* GetSystem() const
		{
			for (auto const& s : Systems)
				if (auto const* casted = dynamic_cast<T const*>(s.get()))
					return casted;
			return nullptr;
		}

		/************************************************************************/
		/*!
		\brief
			Returns whether the game is currently active/running.
		\return
			True if running, false if shutting down.
		*/
		/************************************************************************/
		bool IsGameActive() const { return gameActive; }

		/************************************************************************/
		/*!
		\brief
			Provides access to the MessageBus for pub/sub messaging.
		\return
			Reference to the internal MessageBus.
		*/
		/************************************************************************/
		MessageBus& GetMessageBus() { return messageBus; }

		/************************************************************************/
		/*!
		\brief
			Const overload for MessageBus access.
		\return
			Const reference to the internal MessageBus.
		*/
		/************************************************************************/
		const MessageBus& GetMessageBus() const { return messageBus; }

	private:
		using SystemPtr = std::unique_ptr<SystemInterface>;

		std::vector<SystemPtr>		  Systems;
		MessageBus					  messageBus;   // pub/sub message bus

		float deltaTime = 0.f;	// delta time (per frame)
		float fps = 0.f;		// fps counter
		bool gameActive;		// game running (true), game shutting down (false)
		std::chrono::high_resolution_clock::time_point lastTime; // time of last frame
	};
}