/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneObjectMetadata.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Defines the scene-owned object metadata store used to keep runtime defaults,
					tags, texture paths, and visibility information in one explicit subsystem.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

#include "EngineCore/Math.hpp"

 /**
  * @brief Stores the authored and runtime-resolved metadata tracked for a scene object.
  */
struct SceneObjectDefaults {
	glm::vec3 pos{ 0.f, 0.f, 0.f };
	glm::vec3 size{ 100.f, 100.f, 1.f };
	float rot{ 0.f };

	Math::Vector2D colSize{ 0.0f, 0.0f };
	Math::Vector2D colOff{ 0.0f, 0.0f };
	Math::Vector2D vel{ 0.0f, 0.0f };

	glm::vec2 approachOffset{ 0,0 };
	bool hasApproachOffset2{ false };
	glm::vec2 approachOffset2{ 0,0 };
	bool hasCustomerSeatOffset{ false };
	glm::vec2 customerSeatOffset{ 0,0 };
	int customerSeatCapacity{ 1 };
	bool hasCustomerSeatOffset2{ false };
	glm::vec2 customerSeatOffset2{ 0,0 };
	std::string texture;
	std::string tag;
	std::string layer;

	std::string audioOnSpawn;
	std::string audioOnInteract;
	std::string audioOnDestroy;
	std::string audioOnProcessing;
	bool audioLoop{ false };

	bool visible{ true };
};

/**
 * @brief Centralizes per-object metadata that should remain scene-owned rather than living on the GameObject itself.
 */
class SceneObjectMetadataStore {
public:
	using Defaults = SceneObjectDefaults;

	/**
	 * @brief Clears all stored defaults, tags, and texture path metadata.
	 */
	void Clear();

	/**
	 * @brief Removes every metadata entry associated with a single object identifier.
	 * @param id Identifier of the object whose metadata should be erased.
	 */
	void Erase(int id);

	/**
	 * @brief Replaces the stored defaults for an object.
	 * @param id Identifier of the object being updated.
	 * @param defaults Metadata payload to store for the object.
	 */
	void SetDefaults(int id, const Defaults& defaults);

	/**
	 * @brief Returns a copy of the stored defaults for an object, or a value-initialized payload when absent.
	 * @param id Identifier of the object being queried.
	 * @return The stored defaults copy for the object.
	 */
	Defaults GetDefaults(int id) const;

	/**
	 * @brief Finds mutable defaults storage for an object.
	 * @param id Identifier of the object being queried.
	 * @return Pointer to the stored defaults, or `nullptr` when the object has no stored defaults.
	 */
	Defaults* FindDefaults(int id);

	/**
	 * @brief Finds read-only defaults storage for an object.
	 * @param id Identifier of the object being queried.
	 * @return Pointer to the stored defaults, or `nullptr` when the object has no stored defaults.
	 */
	const Defaults* FindDefaults(int id) const;

	/**
	 * @brief Returns mutable defaults storage for an object, creating a new entry when necessary.
	 * @param id Identifier of the object being queried.
	 * @return Mutable reference to the stored defaults entry.
	 */
	Defaults& EnsureDefaults(int id);

	/**
	 * @brief Returns the full defaults table for iteration-oriented systems.
	 * @return Read-only map of object identifiers to stored defaults.
	 */
	const std::unordered_map<int, Defaults>& GetAllDefaults() const;

	/**
	 * @brief Updates the per-object visibility flag stored in defaults metadata.
	 * @param id Identifier of the object being updated.
	 * @param visible Desired visibility state.
	 */
	void SetVisible(int id, bool visible);

	/**
	 * @brief Returns the stored per-object visibility state, defaulting to visible when missing.
	 * @param id Identifier of the object being queried.
	 * @return `true` when the object should be treated as visible.
	 */
	bool IsVisible(int id) const;

	/**
	 * @brief Stores a gameplay/editor tag override for an object.
	 * @param id Identifier of the object being updated.
	 * @param tag Tag string to associate with the object.
	 */
	void SetTag(int id, const std::string& tag);

	/**
	 * @brief Returns the stored tag for an object, falling back to defaults metadata when needed.
	 * @param id Identifier of the object being queried.
	 * @return The resolved tag string for the object.
	 */
	std::string GetTag(int id) const;

	/**
	 * @brief Stores the source texture path associated with an object.
	 * @param id Identifier of the object being updated.
	 * @param path Texture path to associate with the object.
	 */
	void SetTexturePath(int id, const std::string& path);

	/**
	 * @brief Returns the stored texture path for an object, or an empty shared string when none exists.
	 * @param id Identifier of the object being queried.
	 * @return Reference to the resolved texture path string.
	 */
	const std::string& GetTexturePath(int id) const;

private:
	/**
	 * @brief Returns a shared empty string used for missing texture-path lookups.
	 * @return Reference to the store-wide empty string instance.
	 */
	static const std::string& EmptyString();

	std::unordered_map<int, Defaults> defaults_;
	std::unordered_map<int, std::string> tags_;
	std::unordered_map<int, std::string> texturePaths_;
};
