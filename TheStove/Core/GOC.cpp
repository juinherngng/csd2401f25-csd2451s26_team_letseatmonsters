/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GOC.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (100%)

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

#include "GOC.hpp"
#include "Factory.hpp"
#include "Logger.hpp"

//template<typename T>
//std::optional<T*> GOC::Get() const
//{
//	auto it = m_components.find(typeid(T));
//
//	if (it == m_components.end())
//		return std::nullopt;
//	return static_cast<T*>(it->second);
//}

//template <typename T>
//bool GOC::Has() const
//{
//	return m_components.find(typeid(T)) != m_components.end();
//}

GOC::~GOC() {
	m_components.clear();
}

void GOC::Initialize() {
	//for every component in map
	for (auto& kv : m_components) {
		//call the actual game component init
		kv.second->Initialize();
	}
}

//Let the factory handle it
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


GameComponent* GOC::AddComponent(std::type_index id, std::unique_ptr<GameComponent> c) {
	if (!c) {
		return nullptr;
	}

	c->SetOwner(this);
	GameComponent* component = c.get();
	m_components[id] = std::move(c);
	return component;
}

template <typename T, typename ... Args>
T* GOC::AddComponent(Args&&... args) {
	auto component = std::make_unique<T>(std::forward<Args>(args)...);
	component->SetOwner(this);
	T* componentPtr = component.get();
	m_components[typeid(T)] = std::move(component);
	return componentPtr;
}

GOC* GOC::Clone() const {
	GOC* clone = new GOC();
	for (auto& c : m_components) {
		std::unique_ptr<GameComponent> copy(c.second->Clone());
		copy->SetOwner(clone);
		clone->m_components[c.first] = std::move(copy);
	}
	if (FACTORY) {
		FACTORY->IdGameObject(clone);
	}
	clone->name = this->name + " clone";
	return clone;
}
