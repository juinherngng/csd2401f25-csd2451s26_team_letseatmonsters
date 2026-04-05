/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GameObjectLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (40%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (60%)

 DESCRIPTION:		Declares the base GameObjectLogic class which defines the common
					interface for all gameplay logic components attached to GameObjects,
					including lifecycle methods and owner access utilities.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <string>

class Scene;
class InputManager;
class GameObject;

class GameObjectLogic {
public:
	/**
	 * @brief Constructs a logic object bound to a specific game-object ID.
	 * @param ownerID ID of the owning game object.
	 */
	explicit GameObjectLogic(int ownerID) : ownerID(ownerID) {
		// Store the owner ID so logic can resolve its scene object on demand.
	}

	/**
	 * @brief Virtual destructor for safe polymorphic cleanup.
	 */
	virtual ~GameObjectLogic() = default;

	/**
	 * @brief Called before Start() when the logic is first attached.
	 * @param scene Active scene that owns the logic.
	 */
	virtual void Awake(Scene& scene) {
		// Base logic performs no awake behavior by default.
		(void)scene;
	}

	/**
	 * @brief Called once when the logic starts running.
	 * @param scene Active scene that owns the logic.
	 */
	virtual void Start(Scene& scene) {
		// Base logic performs no start behavior by default.
		(void)scene;
	}

	/**
	 * @brief Updates the logic once per frame.
	 * @param dt Frame delta time in seconds.
	 * @param scene Active scene that owns the logic.
	 * @param input Input manager for frame input state.
	 */
	virtual void Update(float dt, Scene& scene, InputManager& input) {
		// Base logic ignores per-frame updates until overridden by derived classes.
		(void)dt; (void)scene; (void)input;
	}

	/**
	 * @brief Called when the logic is about to be destroyed.
	 * @param scene Active scene that owns the logic.
	 */
	virtual void OnDestroy(Scene& scene) {
		// Base logic performs no destruction behavior by default.
		(void)scene;
	}

	/**
	 * @brief Returns the owning game-object ID.
	 * @return Owner object ID.
	 */
	int GetOwnerID() const {
		// Expose the cached owner ID without requiring a scene lookup.
		return ownerID;
	}

protected:
	/**
	 * @brief Resolves the owning game object from the scene.
	 * @param scene Scene used for owner lookup.
	 * @return Pointer to the owning game object, or nullptr if it no longer exists.
	 */
	GameObject* GetOwner(Scene& scene) const;

	int ownerID;

	/**
	 * @brief Returns a debug-facing logic type name.
	 * @return Logic type name string.
	 */
	virtual std::string GetName() const {
		// Provide a simple default name for generic debugging output.
		return "GameObjectLogic";
	}
};
