#pragma once
class ISerializer;
class GOC;

//The GameComponent that you can attach to a GameObject
//It could be anything like transform, renderer, etc
class GameComponent
{
public:
	virtual ~GameComponent() = default;

	virtual void Serialize(ISerializer& s) = 0;
	virtual void Initialize() {};

	//Who the owner of this GameComponent
	GOC* GetOwner() const
	{
		return m_owner;
	}

	//Attach this GameComponent to a GameObject
	void SetOwner(GOC* owner)
	{
		m_owner = owner;
	}

private:
	//Which Gameobject this Component belong to
	GOC* m_owner = nullptr;
};