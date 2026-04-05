/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GOC.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (70%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (30%)

 DESCRIPTION:		GOC (Game Object Composition) represents a single "entity" in the engine.
					Each GOC contains a map of components (inherited from GameComponent) keyed
					by std::type_index.

					Responsibilities:
					- Attach and manage components at runtime.
					- Initialize and update all attached components.
					- Provide typed access to components (via Get<T>).
					- Defer destruction requests to the Factory for safe cleanup.

					This forms the "GameObject" part of the GameObject-Component system.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include "EngineCore/GameComponent.hpp"

class GOC {
public:
	std::string name;

	/**
	 * @brief Destroys the game-object composition and its owned components.
	 */
	~GOC();

	/**
	 * @brief Retrieves a component of type `T` from this game object.
	 * @tparam T Concrete component type to look up.
	 * @return Optional raw pointer to the requested component.
	 */
	template <typename T>
	std::optional<T*> Get() const {
		auto it = m_components.find(typeid(T));

		// Return an empty optional when the requested component type is not attached.
		if (it == m_components.end())
			return std::nullopt;
		// Cast the stored component back to the requested concrete type.
		return static_cast<T*>(it->second.get());
	}

	/**
	 * @brief Checks whether this game object has a component of type `T`.
	 * @tparam T Concrete component type to test for.
	 * @return True if the component exists, otherwise false.
	 */
	template <typename T>
	bool Has() const {
		// Presence is determined by whether the type key exists in the component map.
		return m_components.find(typeid(T)) != m_components.end();
	}

	/**
	 * @brief Initializes all components attached to this game object.
	 */
	void Initialize();

	/**
	 * @brief Requests destruction of this game object.
	 */
	void Destroy();

	/**
	 * @brief Attaches a component under an explicit type key.
	 * @param id Type key used to store the component.
	 * @param c Component instance to attach.
	 * @return Raw pointer to the attached component, or nullptr if the input was null.
	 */
	GameComponent* AddComponent(std::type_index id, std::unique_ptr<GameComponent> c);

	/**
	 * @brief Constructs and attaches a component of type `T`.
	 * @tparam T Concrete component type to create.
	 * @tparam Args Constructor argument types forwarded to `T`.
	 * @param args Constructor arguments for the component.
	 * @return Raw pointer to the attached component.
	 */
	template <typename T, typename ... Args>
	T* AddComponent(Args&&... args);

	/**
	 * @brief Creates a deep copy of this game object and its components.
	 * @return Newly allocated cloned game object.
	 */
	GOC* Clone() const;

	/**
	 * @brief Removes a component of type `T` from this game object.
	 * @tparam T Concrete component type to remove.
	 */
	template <typename T>
	void RemoveComponent() {
		auto it = m_components.find(typeid(T));
		if (it != m_components.end()) {
			// Erase the owned component so its resources are released via unique_ptr.
			m_components.erase(it);
		}
	}

	/**
	 * @brief Returns this game object's unique ID.
	 * @return Unique object ID.
	 */
	unsigned int GetId() {
		// Expose the factory-assigned ID for lookups and debugging.
		return ObjectId;
	}

	/**
	 * @brief Returns the full component map for read-only inspection.
	 * @return Const reference to the component map.
	 */
	const std::unordered_map<std::type_index, std::unique_ptr<GameComponent>>& GetComponentList() const {
		// Allow systems to iterate components without transferring ownership.
		return m_components;
	}

	// Factory-assigned unique object ID.
	unsigned int ObjectId = 0;

private:
	// Store one component per type so a game object cannot accidentally own duplicate core components.
	std::unordered_map<std::type_index, std::unique_ptr<GameComponent>> m_components;
};
