/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManagerAudio.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements Scene helpers for playing and stopping object-bound runtime audio
					using the AudioManager and stored object default metadata.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Core/AudioManager.hpp"

#include "SceneManager.hpp"

#include <iostream>

// -------------------------------------------------------------------------------------------------
// Object-Bound Audio Playback Helpers
// -------------------------------------------------------------------------------------------------

/**
 * @brief Plays the configured spawn sound for an object if one exists.
 * @param objectId Identifier of the object whose spawn audio should be played.
 */
void Scene::PlaySpawnAudio(int objectId) {
	if (!audioManager_) {
		return;
	}

	auto it = defaults_.find(objectId);
	if (it == defaults_.end()) {
		return;
	}

	const Defaults& defs = it->second;
	if (defs.audioOnSpawn.empty()) {
		return;
	}

	if (audioManager_->HasSound(defs.audioOnSpawn)) {
		audioManager_->PlaySound3D(defs.audioOnSpawn, defs.pos.x, defs.pos.y, defs.pos.z);
		std::cout << "[Scene] Playing spawn audio '" << defs.audioOnSpawn << "' for object " << objectId << std::endl;
	}
	else {
		std::cerr << "[Scene] Spawn audio '" << defs.audioOnSpawn << "' not found in AudioManager" << std::endl;
	}
}

/**
 * @brief Plays the configured interaction sound for an object if one exists.
 * @param objectId Identifier of the object whose interaction audio should be played.
 */
void Scene::PlayInteractAudio(int objectId) {
	if (!audioManager_) {
		return;
	}

	auto it = defaults_.find(objectId);
	if (it == defaults_.end()) {
		return;
	}

	const Defaults& defs = it->second;
	if (defs.audioOnInteract.empty()) {
		return;
	}

	if (audioManager_->HasSound(defs.audioOnInteract)) {
		audioManager_->PlaySound3D(defs.audioOnInteract, defs.pos.x, defs.pos.y, defs.pos.z);
		std::cout << "[Scene] Playing interact audio '" << defs.audioOnInteract << "' for object " << objectId << std::endl;
	}
	else {
		std::cerr << "[Scene] Interact audio '" << defs.audioOnInteract << "' not found in AudioManager" << std::endl;
	}
}

/**
 * @brief Plays the configured destroy sound for an object if one exists.
 * @param objectId Identifier of the object whose destroy audio should be played.
 */
void Scene::PlayDestroyAudio(int objectId) {
	if (!audioManager_) {
		return;
	}

	auto it = defaults_.find(objectId);
	if (it == defaults_.end()) {
		return;
	}

	const Defaults& defs = it->second;
	if (defs.audioOnDestroy.empty()) {
		return;
	}

	if (audioManager_->HasSound(defs.audioOnDestroy)) {
		audioManager_->PlaySound3D(defs.audioOnDestroy, defs.pos.x, defs.pos.y, defs.pos.z);
		std::cout << "[Scene] Playing destroy audio '" << defs.audioOnDestroy << "' for object " << objectId << std::endl;
	}
	else {
		std::cerr << "[Scene] Destroy audio '" << defs.audioOnDestroy << "' not found in AudioManager" << std::endl;
	}
}

/**
 * @brief Starts the configured processing sound for an object if one exists.
 * @param objectId Identifier of the object whose looping processing audio should be played.
 */
void Scene::PlayProcessingAudio(int objectId) {
	if (!audioManager_) {
		return;
	}

	auto it = defaults_.find(objectId);
	if (it == defaults_.end()) {
		return;
	}

	const Defaults& defs = it->second;
	if (defs.audioOnProcessing.empty()) {
		return;
	}

	if (audioManager_->HasSound(defs.audioOnProcessing)) {
		audioManager_->PlaySound3D(defs.audioOnProcessing, defs.pos.x, defs.pos.y, defs.pos.z);
		std::cout << "[Scene] Playing processing audio '" << defs.audioOnProcessing << "' for object " << objectId << std::endl;
	}
	else {
		std::cerr << "[Scene] Processing audio '" << defs.audioOnProcessing << "' not found in AudioManager" << std::endl;
	}
}

// -------------------------------------------------------------------------------------------------
// Object-Bound Audio Stop Helpers
// -------------------------------------------------------------------------------------------------

/**
 * @brief Stops the configured processing sound for an object if it is active.
 * @param objectId Identifier of the object whose looping processing audio should be stopped.
 */
void Scene::StopProcessingAudio(int objectId) {
	if (!audioManager_) {
		return;
	}

	auto it = defaults_.find(objectId);
	if (it == defaults_.end()) {
		return;
	}

	const Defaults& defs = it->second;
	if (defs.audioOnProcessing.empty()) {
		return;
	}

	if (audioManager_->HasSound(defs.audioOnProcessing)) {
		audioManager_->StopSound(defs.audioOnProcessing);
		std::cout << "[Scene] Stopped processing audio '" << defs.audioOnProcessing << "' for object " << objectId << std::endl;
	}
}

/**
 * @brief Stops every object-bound audio clip currently referenced by scene defaults.
 */
void Scene::StopAllObjectAudio() {
	if (!audioManager_) {
		return;
	}

	for (const auto& [id, defs] : defaults_) {
		if (!defs.audioOnSpawn.empty() && audioManager_->HasSound(defs.audioOnSpawn)) {
			audioManager_->StopSound(defs.audioOnSpawn);
		}

		if (!defs.audioOnInteract.empty() && audioManager_->HasSound(defs.audioOnInteract)) {
			audioManager_->StopSound(defs.audioOnInteract);
		}

		if (!defs.audioOnDestroy.empty() && audioManager_->HasSound(defs.audioOnDestroy)) {
			audioManager_->StopSound(defs.audioOnDestroy);
		}

		if (!defs.audioOnProcessing.empty() && audioManager_->HasSound(defs.audioOnProcessing)) {
			audioManager_->StopSound(defs.audioOnProcessing);
		}
	}

	std::cout << "[Scene] Stopped all object-bound audio" << std::endl;
}

