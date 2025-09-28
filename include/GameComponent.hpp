#pragma once
#include <typeindex>

//class ISerializer;
class GOC;

//The GameComponent that you can attach to a GameObject
//Inherit this class when creating Game Component
class GameComponent
{
public:
	GameComponent() = default;
	//Init the Game Component
	virtual void Initialize() {};

	//virtual void Serialize(ISerializer& s) = 0;	//nah Im good for now

	//Return the owner of this GameComponent
	GOC* GetOwner() const
	{
		return m_owner;
	}

	//key to the type
	std::type_index typeId{ typeid(void) };

	//Attach this GameComponent to a GameObject
	void SetOwner(GOC* owner)
	{
		m_owner = owner;
	}

protected:
	//dtor
	virtual ~GameComponent() {};

private:
	//Which Gameobject this Component belong to
	GOC* m_owner = nullptr;
};