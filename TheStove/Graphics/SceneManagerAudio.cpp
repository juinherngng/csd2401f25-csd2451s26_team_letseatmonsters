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
#include "../Core/Logger.hpp"

#include "SceneManager.hpp"

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

	const Defaults* defs = objectMetadata_.FindDefaults(objectId);
	if (defs == nullptr) {
		return;
	}

	if (defs->audioOnSpawn.empty()) {
		return;
	}

	if (audioManager_->HasSound(defs->audioOnSpawn)) {
		audioManager_->PlaySound3D(defs->audioOnSpawn, defs->pos.x, defs->pos.y, defs->pos.z);
		TS_LOG_DEBUG("[Scene] Playing spawn audio '" << defs->audioOnSpawn << "' for object " << objectId);
	}
	else {
		TS_LOG_WARN("[Scene] Spawn audio '" << defs->audioOnSpawn << "' not found in AudioManager");
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

	const Defaults* defs = objectMetadata_.FindDefaults(objectId);
	if (defs == nullptr) {
		return;
	}

	if (defs->audioOnInteract.empty()) {
		return;
	}

	if (audioManager_->HasSound(defs->audioOnInteract)) {
		audioManager_->PlaySound3D(defs->audioOnInteract, defs->pos.x, defs->pos.y, defs->pos.z);
		TS_LOG_DEBUG("[Scene] Playing interact audio '" << defs->audioOnInteract << "' for object " << objectId);
	}
	else {
		TS_LOG_WARN("[Scene] Interact audio '" << defs->audioOnInteract << "' not found in AudioManager");
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

	const Defaults* defs = objectMetadata_.FindDefaults(objectId);
	if (defs == nullptr) {
		return;
	}

	if (defs->audioOnDestroy.empty()) {
		return;
	}

	if (audioManager_->HasSound(defs->audioOnDestroy)) {
		audioManager_->PlaySound3D(defs->audioOnDestroy, defs->pos.x, defs->pos.y, defs->pos.z);
		TS_LOG_DEBUG("[Scene] Playing destroy audio '" << defs->audioOnDestroy << "' for object " << objectId);
	}
	else {
		TS_LOG_WARN("[Scene] Destroy audio '" << defs->audioOnDestroy << "' not found in AudioManager");
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

	const Defaults* defs = objectMetadata_.FindDefaults(objectId);
	if (defs == nullptr) {
		return;
	}

	if (defs->audioOnProcessing.empty()) {
		return;
	}

	if (audioManager_->HasSound(defs->audioOnProcessing)) {
		audioManager_->PlaySound3D(defs->audioOnProcessing, defs->pos.x, defs->pos.y, defs->pos.z);
		TS_LOG_DEBUG("[Scene] Playing processing audio '" << defs->audioOnProcessing << "' for object " << objectId);
	}
	else {
		TS_LOG_WARN("[Scene] Processing audio '" << defs->audioOnProcessing << "' not found in AudioManager");
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

	const Defaults* defs = objectMetadata_.FindDefaults(objectId);
	if (defs == nullptr) {
		return;
	}

	if (defs->audioOnProcessing.empty()) {
		return;
	}

	if (audioManager_->HasSound(defs->audioOnProcessing)) {
		audioManager_->StopSound(defs->audioOnProcessing);
		TS_LOG_DEBUG("[Scene] Stopped processing audio '" << defs->audioOnProcessing << "' for object " << objectId);
	}
}

/**
 * @brief Stops every object-bound audio clip currently referenced by scene defaults.
 */
void Scene::StopAllObjectAudio() {
	if (!audioManager_) {
		return;
	}

	for (const auto& [id, defs] : objectMetadata_.GetAllDefaults()) {
		(void)id;
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

	TS_LOG_DEBUG("[Scene] Stopped all object-bound audio");
}

