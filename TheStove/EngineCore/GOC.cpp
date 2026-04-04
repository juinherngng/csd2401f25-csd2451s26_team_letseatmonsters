/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GOC.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (40%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (60%)

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

#include "EngineCore/Factory.hpp"
#include "EngineCore/GOC.hpp"
#include "EngineCore/Logger.hpp"

/**
 * @brief Destroys the game-object composition and releases all owned components.
 */
GOC::~GOC() {
	// Clearing the component map releases every owned component via unique_ptr.
	m_components.clear();
}

/**
 * @brief Initializes every component currently attached to the game object.
 */
void GOC::Initialize() {
	// Forward initialization to each attached component exactly once during object startup.
	for (auto& kv : m_components) {
		// Call the component-specific initialization hook.
		kv.second->Initialize();
	}
}

/**
 * @brief Requests destruction of this game object.
 */
void GOC::Destroy() {
	// Prefer deferred destruction through the factory so external id maps do not keep dangling pointers.
	if (FACTORY) {
		TS_LOG_INFO("[GOC] Queueing game object for destruction: " << name);
		FACTORY->AddDestroy(this);
		return;
	}

	// Fall back to direct deletion only when no factory exists to own lifetime.
	TS_LOG_WARN("[GOC] Destroy called without Factory; deleting directly: " << name);
	delete this;
}

/**
 * @brief Attaches a component to the game object under an explicit type key.
 * @param id Type key used to store the component.
 * @param c Component instance to attach.
 * @return Raw pointer to the attached component, or nullptr if the input was null.
 */
GameComponent* GOC::AddComponent(std::type_index id, std::unique_ptr<GameComponent> c) {
	if (!c) {
		return nullptr;
	}

	// Set the back-reference before transferring ownership into the component map.
	c->SetOwner(this);
	GameComponent* component = c.get();
	m_components[id] = std::move(c);
	return component;
}

template <typename T, typename ... Args>
/**
 * @brief Constructs and attaches a component of type `T` to the game object.
 * @tparam T Concrete component type to create.
 * @tparam Args Constructor argument types forwarded into `T`.
 * @param args Constructor arguments used to build the component.
 * @return Raw pointer to the attached component.
 */
T* GOC::AddComponent(Args&&... args) {
	auto component = std::make_unique<T>(std::forward<Args>(args)...);
	// Set ownership before exposing the component so it can safely query its parent object.
	component->SetOwner(this);
	T* componentPtr = component.get();
	m_components[typeid(T)] = std::move(component);
	return componentPtr;
}

/**
 * @brief Creates a deep copy of this game object and all of its components.
 * @return Newly allocated cloned game object.
 */
GOC* GOC::Clone() const {
	GOC* clone = new GOC();
	for (auto& c : m_components) {
		// Ask each component to clone itself so concrete component state is preserved.
		std::unique_ptr<GameComponent> copy(c.second->Clone());
		copy->SetOwner(clone);
		clone->m_components[c.first] = std::move(copy);
	}
	if (FACTORY) {
		// Register the clone with the factory so it gets a valid object ID and lifetime ownership.
		FACTORY->IdGameObject(clone);
	}
	// Distinguish the clone in debug/editor views without losing the original name.
	clone->name = this->name + " clone";
	return clone;
}
