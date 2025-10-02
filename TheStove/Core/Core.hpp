/*
----------------------------------------------------------------------------------------------------
FILE NAME:			Core.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:		The core engine managing the game loop and systems.

		All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "System.hpp"

#include <vector>
#include <chrono>
#include <deque>
#include <memory>
#include <utility>

namespace CoreFramework
{
	// how to access global dt:
	// float dt = CoreFramework::gDt;
	extern float gDt;			// global delta time

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
			Destructs the CoreEngine. Systems should already be destroyed via
			DestroySystems() before this runs.
		*/
		/************************************************************************/
		~CoreEngine();

		/************************************************************************/
		/*!
		\brief
			Runs one frame of the game loop: computes dt, updates systems,
			flushes queued messages, and records performance statistics.
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
			Broadcasts a message immediately to every registered system.
		\param msg
			Pointer to an existing message object (not owned / not deleted).
		*/
		/************************************************************************/
		void BroadcastMessage(Message* msg);

		/************************************************************************/
		/*!
		\brief
			Adds (registers) a new system to the engine. CoreEngine takes ownership
			and will delete it in DestroySystems().
		\param system
			Raw pointer to a heap-allocated system.
		*/
		/************************************************************************/
		void AddSystem(SystemInterface* system);

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

		// Accessor to Read-Only values of Systems
		const std::vector<SystemInterface*>& GetSystems() const { return Systems; }

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
			for (auto* s : Systems)
				if (auto* casted = dynamic_cast<T*>(s))
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
		T* const GetSystem() const
		{
			for (auto* s : Systems)
				if (auto* casted = dynamic_cast<T*>(s))
					return casted;
			return nullptr;
		}

		/************************************************************************/
		/*!
		\brief
			Queues a message for delivery at the next flush point.
		\details
			Message is constructed in-place and owned by the queue until flushed.
		\param args
			Arguments forwarded to the message constructor.
		\tparam T
			Message type deriving from Message.
		*/
		/************************************************************************/
		template<typename T, typename... Args>
		void Post(Args&&... args)
		{
			messageQueue.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
		}

		/************************************************************************/
		/*!
		\brief
			Delivers all queued messages (FIFO order) via BroadcastMessage(),
			then clears the queue.
		*/
		/************************************************************************/
		void FlushMessages()
		{
			while (!messageQueue.empty())
			{
				BroadcastMessage(messageQueue.front().get());
				messageQueue.pop_front();
			}
		}

	private:
		using MessagePtr = std::unique_ptr<Message>;

		std::vector<SystemInterface*> Systems;
		std::deque<MessagePtr>		  messageQueue; // messages to be processed at the start of the next frame

		float fps = 0.f;		// fps counter
		bool gameActive;		// game running (true), game shutting down (false)
		std::chrono::high_resolution_clock::time_point lastTime; // time of last frame
	};

	extern CoreEngine* CORE;
}