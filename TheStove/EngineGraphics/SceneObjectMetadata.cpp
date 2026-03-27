/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneObjectMetadata.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements the scene-owned object metadata store used to keep runtime defaults,
					tags, texture paths, and visibility information in one explicit subsystem.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "EngineGraphics/SceneObjectMetadata.hpp"

 /**
  * @brief Clears all stored defaults, tags, and texture path metadata.
  */
void SceneObjectMetadataStore::Clear() {
	defaults_.clear();
	tags_.clear();
	texturePaths_.clear();
}

/**
 * @brief Removes every metadata entry associated with a single object identifier.
 * @param id Identifier of the object whose metadata should be erased.
 */
void SceneObjectMetadataStore::Erase(int id) {
	defaults_.erase(id);
	tags_.erase(id);
	texturePaths_.erase(id);
}

/**
 * @brief Replaces the stored defaults for an object.
 * @param id Identifier of the object being updated.
 * @param defaults Metadata payload to store for the object.
 */
void SceneObjectMetadataStore::SetDefaults(int id, const Defaults& defaults) {
	defaults_[id] = defaults;
}

/**
 * @brief Returns a copy of the stored defaults for an object, or a value-initialized payload when absent.
 * @param id Identifier of the object being queried.
 * @return The stored defaults copy for the object.
 */
SceneObjectMetadataStore::Defaults SceneObjectMetadataStore::GetDefaults(int id) const {
	auto it = defaults_.find(id);
	return (it != defaults_.end()) ? it->second : Defaults{};
}

/**
 * @brief Finds mutable defaults storage for an object.
 * @param id Identifier of the object being queried.
 * @return Pointer to the stored defaults, or `nullptr` when the object has no stored defaults.
 */
SceneObjectMetadataStore::Defaults* SceneObjectMetadataStore::FindDefaults(int id) {
	auto it = defaults_.find(id);
	return (it != defaults_.end()) ? &it->second : nullptr;
}

/**
 * @brief Finds read-only defaults storage for an object.
 * @param id Identifier of the object being queried.
 * @return Pointer to the stored defaults, or `nullptr` when the object has no stored defaults.
 */
const SceneObjectMetadataStore::Defaults* SceneObjectMetadataStore::FindDefaults(int id) const {
	auto it = defaults_.find(id);
	return (it != defaults_.end()) ? &it->second : nullptr;
}

/**
 * @brief Returns mutable defaults storage for an object, creating a new entry when necessary.
 * @param id Identifier of the object being queried.
 * @return Mutable reference to the stored defaults entry.
 */
SceneObjectMetadataStore::Defaults& SceneObjectMetadataStore::EnsureDefaults(int id) {
	return defaults_[id];
}

/**
 * @brief Returns the full defaults table for iteration-oriented systems.
 * @return Read-only map of object identifiers to stored defaults.
 */
const std::unordered_map<int, SceneObjectMetadataStore::Defaults>& SceneObjectMetadataStore::GetAllDefaults() const {
	return defaults_;
}

/**
 * @brief Updates the per-object visibility flag stored in defaults metadata.
 * @param id Identifier of the object being updated.
 * @param visible Desired visibility state.
 */
void SceneObjectMetadataStore::SetVisible(int id, bool visible) {
	defaults_[id].visible = visible;
}

/**
 * @brief Returns the stored per-object visibility state, defaulting to visible when missing.
 * @param id Identifier of the object being queried.
 * @return `true` when the object should be treated as visible.
 */
bool SceneObjectMetadataStore::IsVisible(int id) const {
	auto it = defaults_.find(id);
	return (it != defaults_.end()) ? it->second.visible : true;
}

/**
 * @brief Stores a gameplay/editor tag override for an object.
 * @param id Identifier of the object being updated.
 * @param tag Tag string to associate with the object.
 */
void SceneObjectMetadataStore::SetTag(int id, const std::string& tag) {
	tags_[id] = tag;
}

/**
 * @brief Returns the stored tag for an object, falling back to defaults metadata when needed.
 * @param id Identifier of the object being queried.
 * @return The resolved tag string for the object.
 */
std::string SceneObjectMetadataStore::GetTag(int id) const {
	auto it = tags_.find(id);
	if (it != tags_.end()) {
		return it->second;
	}

	const Defaults* defaults = FindDefaults(id);
	return (defaults != nullptr) ? defaults->tag : std::string{};
}

/**
 * @brief Stores the source texture path associated with an object.
 * @param id Identifier of the object being updated.
 * @param path Texture path to associate with the object.
 */
void SceneObjectMetadataStore::SetTexturePath(int id, const std::string& path) {
	texturePaths_[id] = path;
}

/**
 * @brief Returns the stored texture path for an object, or an empty shared string when none exists.
 * @param id Identifier of the object being queried.
 * @return Reference to the resolved texture path string.
 */
const std::string& SceneObjectMetadataStore::GetTexturePath(int id) const {
	auto it = texturePaths_.find(id);
	return (it != texturePaths_.end()) ? it->second : EmptyString();
}

/**
 * @brief Returns a shared empty string used for missing texture-path lookups.
 * @return Reference to the store-wide empty string instance.
 */
const std::string& SceneObjectMetadataStore::EmptyString() {
	static const std::string empty;
	return empty;
}
