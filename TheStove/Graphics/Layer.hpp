/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Layer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (40%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(60%)

 DESCRIPTION:		The Layer class maintains a collection of game object IDs that belong to this layer.
					Core functionality includes adding and removing object IDs to/from the layer, retrieving
					the current list of objects, and accessing the layer's name.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <algorithm>
#include <string>
#include <vector>

 // Forward declare GameObject to avoid circular dependency.
class Layer {
public:
	// Construct a layer with the default name "Layer".
	Layer() : name("") {
	}

	// Construct a layer with a specified name.
	Layer(const std::string& name) : name(name) {
	}

	// Add an object ID to the layer if it's not already present.
	void AddObject(int id) {
		if (std::find(objectIDs.begin(), objectIDs.end(), id) == objectIDs.end()) {
			objectIDs.push_back(id);
		}
	}
	void RemoveObject(int id) {
		objectIDs.erase(
			std::remove(objectIDs.begin(), objectIDs.end(), id),
			objectIDs.end());
	}
	const std::vector<int>& GetObjects() const {
		return objectIDs;
	}
	
	// Get the name of the layer.
	std::string GetName() const {
		return name;
	}
	void SetName(const std::string& newName) {
		name = newName;
	}

	// Check whether this layer is visible for rendering.
	bool IsVisible() const {
		return visible;
	}
	void SetVisible(bool v) {
		visible = v;
	}

	// Check whether this layer participates in collision logic.
	bool IsCollidable() const {
		return collidable;
	}
	void SetCollidable(bool c) {
		collidable = c;
	}

	// Check whether this layer is enabled (active in the scene). If disabled, it should not be visible or collidable.
	bool IsEnabled() const {
		return enabled;
	}
	void SetEnabled(bool e) {
		enabled = e;
		// Optional: if a layer is disabled, it should not be visible/collidable either
		if (!enabled) {
			visible = false;
			collidable = false;
		}
	}

private:
	// The name of the layer, used for organization and referencing in the level editor and JSON.
	std::string name;
	std::vector<int> objectIDs;

	// Layer properties that affect rendering and collision logic. These can be set per layer to control visibility and collision behavior of all objects in the layer.
	bool visible = true;
	bool collidable = true;
	bool enabled = true;
};
