/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GameComponent.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (60%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (40%)

 DESCRIPTION:		Base class for all attachable components in the GameObject-Component (GOC)
					architecture. Provides lifecycle methods (Initialize, Start, Update,
					OnEnable, OnDisable), ownership tracking via the parent GOC, and enable/
					disable state management.

					All user-defined components (e.g., Transform, RigidBody2D) should inherit
					from GameComponent and override virtual methods to define their behavior.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <iostream>
#include <typeindex>

class GOC;

// The GameComponent that you can attach to a GameObject.
// Inherit this class when creating a concrete game component type.
class GameComponent {
public:
	GameComponent() = default;

	/**
	 * @brief Performs one-time setup before the component begins runtime use.
	 */
	virtual void Initialize() {
		// Base GameComponent has no initialization work.
	}

	/**
	 * @brief Performs the component's first runtime start step.
	 */
	virtual void Start() {
		// Base GameComponent has no startup behavior.
	}

	/**
	 * @brief Updates the component once per frame while it is active.
	 * @param dt Frame delta time in seconds.
	 */
	virtual void Update(float dt) {
		// Base GameComponent ignores per-frame updates by default.
		(void)dt;
	}

	//virtual void Serialize(ISerializer& s) = 0;	//nah Im good for now

	/**
	 * @brief Returns the owning GOC that this component is attached to.
	 * @return Pointer to the owning GOC, or nullptr if none has been assigned.
	 */
	GOC* GetOwner() const {
		return m_owner;
	}

	// Key to the runtime component type.
	std::type_index typeId{ typeid(void) };

	/**
	 * @brief Assigns the owning GOC for this component.
	 * @param owner Pointer to the owning GOC.
	 */
	void SetOwner(GOC* owner) {
		// Store the back-reference so the component can query its parent object later.
		m_owner = owner;
	}

	/**
	 * @brief Called when the component transitions into the enabled state.
	 */
	virtual void OnEnable() {
		// Base GameComponent has no enable hook behavior.
	}

	/**
	 * @brief Called when the component transitions into the disabled state.
	 */
	virtual void OnDisable() {
		// Base GameComponent has no disable hook behavior.
	}

	/**
	 * @brief Enables or disables the component and triggers the matching lifecycle hook.
	 * @param state Desired enabled state.
	 */
	void SetEnabled(bool state) {
		if (enabled == state) return;
		// Only fire lifecycle hooks when the state actually changes.
		enabled = state;
		if (enabled) OnEnable();
		else OnDisable();
	}

	/**
	 * @brief Checks whether the component is currently enabled.
	 * @return True when the component is enabled.
	 */
	bool IsEnabled() const {
		return enabled;
	}

	/**
	 * @brief Checks whether the component has already executed Start().
	 * @return True when Start() has been run through SetStarted().
	 */
	bool IsStarted() const {
		return started;
	}

	/**
	 * @brief Updates the started flag and runs Start() on the first transition to true.
	 * @param state Desired started state.
	 */
	void SetStarted(bool state) {
		if (started == state) {
			return;
		}
		started = state;
		if (started) {
			// Trigger Start() exactly once when the component first becomes started.
			Start();
		}
	}

	/**
	 * @brief Returns a debug-facing string description of the component.
	 * @return Human-readable description string.
	 */
	virtual std::string ToString() const {
		return "GameComponent (base), this doesnt do anything";
	}

	/**
	 * @brief Virtual destructor for safe polymorphic cleanup.
	 */
	virtual ~GameComponent() {
		// Base GameComponent does not own any cleanup-sensitive resources.
	}

	/**
	 * @brief Creates a heap-allocated copy of the concrete component.
	 * @return Newly allocated clone of the component.
	 */
	virtual GameComponent* Clone() const = 0;

	// Which GameObject this component belongs to.
	GOC* m_owner = nullptr;
private:
	// Tracks whether this component is currently enabled.
	bool enabled = true;
	// Tracks whether Start() has already been issued for this component.
	bool started = false;
	// Component name for testing.
	std::string name;
};
