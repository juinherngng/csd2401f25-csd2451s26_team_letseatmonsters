#include "GOC.hpp"
#include "Factory.hpp"

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
	for (auto& kv : m_components) {
		delete kv.second; // will call ~Transform, ~RigidBody2D, etc.
	}
	m_components.clear();
}

void GOC::Initialize()
{
	//for every component in map
	for (auto& kv : m_components)
	{
		//call the actual game component init
		kv.second->Initialize();
	}
}

//Let the factory handle it
void GOC::Destroy()
{
	//FACTORY->AddDestroy(this);
	std::cout << "Deleting " << name << " game Object" << std::endl;
	delete this;
}


void GOC::AddComponent(std::type_index id, GameComponent* c)
{
	c->SetOwner(this);
	m_components[id] = c;
}

template <typename T, typename ... Args>
T* GOC::AddComponent(Args&&... args)
{
	auto* component = new T(std::forward<Args>(args)...);
	component->SetOwner(this);
	m_components[typeid(T)] = component;
	return component;
}