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
	FACTORY->AddDestroy(this);
}


void GOC::AddComponent(std::type_index id, GameComponent* c)
{
	c->SetOwner(this);
	m_components[id] = c;
}
