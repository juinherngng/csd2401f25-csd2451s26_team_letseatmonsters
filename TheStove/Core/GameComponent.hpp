/*
----------------------------------------------------------------------------------------------------
FILE NAME:			GameComponent.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Vu Phan Hung, phanhung.vu@digipen.edu

DESCRIPTION:
	Base class for all attachable components in the GameObject-Component (GOC) 
	architecture. Provides lifecycle methods (Initialize, Start, Update, 
	OnEnable, OnDisable), ownership tracking via the parent GOC, and enable/
	disable state management. 
	
	All user-defined components (e.g., Transform, RigidBody2D) should inherit 
	from GameComponent and override virtual methods to define their behavior.

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once
#include <typeindex>
#include <iostream>

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
	//Start is only called once and this check using the flag started
	virtual void Start() {};
	//Update
	virtual void Update(float dt) {};

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

	//called when the component is enabled
	virtual void OnEnable() {};
	//called when the component is disabled
	virtual void OnDisable() {};

	//Enable/disable the component
	void SetEnabled(bool state)
	{
		if (enabled == state) return;
		enabled = state;
		if (enabled) OnEnable();
		else OnDisable();
	}

	//Is this component enabled?
	bool IsEnabled() const
	{
		return enabled;
	}

	//Is this component run through Start?
	bool IsStarted() const
	{
		return started;
	}

	void SetStarted(bool state)
	{
		if (started == state)
		{
			return;
		}
		started = state;
		if (started)
		{
			Start();
		}
	}

	virtual std::string ToString() const {
		return "GameComponent (base), this doesnt do anything";
	}

	//dtor
	virtual ~GameComponent() {};

private:
	//Which Gameobject this Component belong to
	GOC* m_owner = nullptr;
	//is this Component enabled?
	bool enabled = true;
	//Start is only called once
	bool started = false;
	//Component name for testing
	std::string name;
};