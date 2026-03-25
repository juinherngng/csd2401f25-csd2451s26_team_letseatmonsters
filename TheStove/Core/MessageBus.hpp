/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			MessageBus.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:		Publish/Subscribe message bus for inter-component communication.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "Message.hpp"

#include <deque>
#include <functional>
#include <memory>
#include <utility>
#include <unordered_map>
#include <vector>

namespace CoreFramework {
	// Subscriber callback type: takes a const Message reference
	using MessageCallback = std::function<void(const Message&)>;

	// Unique subscriber ID for unsubscribing
	using SubscriberId = size_t;
	using SubscriberEntry = std::pair<SubscriberId, MessageCallback>;

	/************************************************************************/
	/*!
	\brief
		Central message bus implementing publish/subscribe pattern.
		Allows subscribers to register callbacks for specific message types
		and publishes messages to all interested subscribers.
	*/
	/************************************************************************/
	class MessageBus {
	public:
		MessageBus() : nextSubscriberId(1) {
		}

		/************************************************************************/
		/*!
		\brief
			Subscribes to messages of a specific type.
		\param messageType
			The MessageType to listen for.
		\param callback
			Function to call when a message of this type is published.
		\return
			Unique subscriber ID that can be used to unsubscribe.
		*/
		/************************************************************************/
		SubscriberId Subscribe(MessageType messageType, MessageCallback callback) {
			SubscriberId id = nextSubscriberId++;
			subscribers[messageType].push_back({ id, std::move(callback) });
			return id;
		}

		/************************************************************************/
		/*!
		\brief
			Unsubscribes a specific subscriber from a message type.
		\param messageType
			The message type to unsubscribe from.
		\param subscriberId
			The ID returned from Subscribe().
		\return
			True if successfully unsubscribed, false if not found.
		*/
		/************************************************************************/
		bool Unsubscribe(MessageType messageType, SubscriberId subscriberId) {
			auto it = subscribers.find(messageType);
			if (it == subscribers.end())
				return false;

			auto& callbacks = it->second;
			for (auto callbackIt = callbacks.begin(); callbackIt != callbacks.end(); ++callbackIt) {
				if (callbackIt->first == subscriberId) {
					callbacks.erase(callbackIt);
					return true;
				}
			}
			return false;
		}

		/************************************************************************/
		/*!
		\brief
			Publishes a message immediately to all subscribers of that message type.
		\param message
			The message to publish (will be passed by const reference to callbacks).
		*/
		/************************************************************************/
		void Publish(const Message& message) {
			auto it = subscribers.find(message.MessageId);
			if (it != subscribers.end()) {
				// Dispatch against a snapshot so callbacks can safely mutate
				// subscriptions without invalidating this iteration.
				std::vector<SubscriberEntry> callbacks = it->second;
				for (const auto& [id, callback] : callbacks) {
					if (IsStillSubscribed(message.MessageId, id)) {
						callback(message);
					}
				}
			}
		}

		/************************************************************************/
		/*!
		\brief
			Queues a message to be published later via ProcessQueue().
		\param args
			Arguments forwarded to construct a message of type T.
		\tparam T
			Message type deriving from Message.
		*/
		/************************************************************************/
		template<typename T, typename... Args>
		void Post(Args&&... args) {
			messageQueue.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
		}

		/************************************************************************/
		/*!
		\brief
			Processes all queued messages in FIFO order, publishing each one
			and then clearing the queue.
		*/
		/************************************************************************/
		void ProcessQueue() {
			while (!messageQueue.empty()) {
				Publish(*messageQueue.front());
				messageQueue.pop_front();
			}
		}

		/************************************************************************/
		/*!
		\brief
			Clears all queued messages without publishing them.
		*/
		/************************************************************************/
		void ClearQueue() {
			messageQueue.clear();
		}

		/************************************************************************/
		/*!
		\brief
			Removes all subscribers for all message types.
		*/
		/************************************************************************/
		void ClearAllSubscribers() {
			subscribers.clear();
		}

		/************************************************************************/
		/*!
		\brief
			Returns the number of subscribers for a specific message type.
		\param messageType
			The message type to query.
		\return
			Number of active subscribers.
		*/
		/************************************************************************/
		size_t GetSubscriberCount(MessageType messageType) const {
			auto it = subscribers.find(messageType);
			return (it != subscribers.end()) ? it->second.size() : 0;
		}

	private:
		bool IsStillSubscribed(MessageType messageType, SubscriberId subscriberId) const {
			auto it = subscribers.find(messageType);
			if (it == subscribers.end()) {
				return false;
			}

			for (const auto& [id, callback] : it->second) {
				(void)callback;
				if (id == subscriberId) {
					return true;
				}
			}

			return false;
		}

		// Map of message type -> list of (subscriberId, callback)
		std::unordered_map<MessageType, std::vector<SubscriberEntry>> subscribers;

		// Queue for deferred message processing
		std::deque<std::unique_ptr<Message>> messageQueue;

		// Counter for generating unique subscriber IDs
		SubscriberId nextSubscriberId;
	};
}

