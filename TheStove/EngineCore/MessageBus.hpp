/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			MessageBus.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (60%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	    (40%)

 DESCRIPTION:		Publish/Subscribe message bus for inter-component communication.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "EngineCore/Message.hpp"

namespace CoreFramework {
	// Subscriber callback type: takes a const Message reference
	using MessageCallback = std::function<void(const Message&)>;

	// Unique subscriber ID for unsubscribing
	using SubscriberId = size_t;
	using SubscriberEntry = std::pair<SubscriberId, MessageCallback>;

	/**
	 * @brief Central publish/subscribe message bus for engine and game events.
	 */
	class MessageBus {
	public:
		/**
		 * @brief Initializes the message bus with the first subscriber identifier.
		 */
		MessageBus() : nextSubscriberId(1) {}

		/**
		 * @brief Registers a callback for one message type.
		 * @param messageType Message type to listen for.
		 * @param callback Function invoked whenever that message type is published.
		 * @return Unique subscriber identifier that can later be used to unsubscribe.
		 */
		SubscriberId Subscribe(MessageType messageType, MessageCallback callback) {
			// Reserve the next stable subscriber identifier before storing the callback.
			SubscriberId id = nextSubscriberId++;
			subscribers[messageType].push_back({ id, std::move(callback) });
			return id;
		}

		/**
		 * @brief Removes one subscriber from a specific message type.
		 * @param messageType Message type to unsubscribe from.
		 * @param subscriberId Identifier previously returned by `Subscribe`.
		 * @return `true` when the subscriber entry was found and removed.
		 */
		bool Unsubscribe(MessageType messageType, SubscriberId subscriberId) {
			auto it = subscribers.find(messageType);
			if (it == subscribers.end()) {
				// Bail out quickly when the message type has no subscriber list.
				return false;
			}

			auto& callbacks = it->second;
			for (auto callbackIt = callbacks.begin(); callbackIt != callbacks.end(); ++callbackIt) {
				if (callbackIt->first == subscriberId) {
					// Remove the matching subscriber entry and report success immediately.
					callbacks.erase(callbackIt);
					return true;
				}
			}

			// Report failure when the requested subscriber ID does not exist for this type.
			return false;
		}

		/**
		 * @brief Immediately dispatches a message to all current subscribers of its type.
		 * @param message Message instance to publish.
		 */
		void Publish(const Message& message) {
			auto it = subscribers.find(message.MessageId);
			if (it != subscribers.end()) {
				// Dispatch against a snapshot so callbacks can safely mutate subscriptions mid-broadcast.
				std::vector<SubscriberEntry> callbacks = it->second;
				for (const auto& [id, callback] : callbacks) {
					if (IsStillSubscribed(message.MessageId, id)) {
						// Skip callbacks that unsubscribed themselves before their turn.
						callback(message);
					}
				}
			}
		}

		/**
		 * @brief Queues a message to be published later in FIFO order.
		 * @tparam T Concrete message type deriving from `Message`.
		 * @param args Constructor arguments forwarded into the queued message.
		 */
		template<typename T, typename... Args>
		void Post(Args&&... args) {
			// Construct the deferred message in-place inside the queue.
			messageQueue.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
		}

		/**
		 * @brief Publishes every queued message in FIFO order.
		 */
		void ProcessQueue() {
			while (!messageQueue.empty()) {
				// Publish the front message before discarding it from the deferred queue.
				Publish(*messageQueue.front());
				messageQueue.pop_front();
			}
		}

		/**
		 * @brief Discards every queued message without publishing it.
		 */
		void ClearQueue() {
			// Drop deferred work when the caller wants a clean message queue.
			messageQueue.clear();
		}

		/**
		 * @brief Removes every subscriber from the bus.
		 */
		void ClearAllSubscribers() {
			// Reset the routing table when systems need to tear down or rebuild subscriptions.
			subscribers.clear();
		}

		/**
		 * @brief Returns the number of active subscribers for one message type.
		 * @param messageType Message type to query.
		 * @return Number of currently registered subscribers for that type.
		 */
		size_t GetSubscriberCount(MessageType messageType) const {
			auto it = subscribers.find(messageType);
			// Return zero when the message type has never been registered.
			return (it != subscribers.end()) ? it->second.size() : 0;
		}

	private:
		/**
		 * @brief Checks whether a subscriber is still registered for a message type.
		 * @param messageType Message type whose subscriber list should be searched.
		 * @param subscriberId Identifier to look up.
		 * @return `true` when the subscriber is still present.
		 */
		bool IsStillSubscribed(MessageType messageType, SubscriberId subscriberId) const {
			auto it = subscribers.find(messageType);
			if (it == subscribers.end()) {
				// Treat a missing list as an unsubscribed subscriber.
				return false;
			}

			for (const auto& [id, callback] : it->second) {
				(void)callback;
				if (id == subscriberId) {
					// Report success as soon as the subscriber ID is found.
					return true;
				}
			}

			// No subscriber with the requested identifier remains registered.
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
