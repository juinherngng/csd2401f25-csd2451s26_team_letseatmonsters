/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (35%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(20%)
					Vu Phan Hung, phanhung.vu@digipen.edu   (15%)
					Ng Juin Herng, juinherng.ng@digipen.edu (30%)

 DESCRIPTION:		Implements the Scene class, which is responsible for the high-level
					management, coordination, and per-frame updating of all entities, systems,
					and game logic within a scene.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Core/AudioManager.hpp"
#include "../Core/FilePaths.hpp"
#include "../Core/LevelEditorPanelFonts.hpp"

#include "GraphicsEngine.hpp"
#include "SceneManager.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <Core/RuntimeLevel.hpp>
#include <exception>
#include <fstream>
#include <glm/ext/matrix_clip_space.hpp>
#include <iostream>
#include <random>
#include <unordered_set>

 // Cache for boundary flags used by bounded transitioned cutscenes
static std::vector<bool> sCutsceneBoundaryFlags;

namespace {

	/**
	 * @brief Creates customer payment star frames.
	 * @return Result produced by this operation.
	 */
	std::vector<glm::vec4> CreateCustomerPaymentStarFrames() {
		// staranim-Sheet.png:
		// top row = 11 frames
		// bottom row = 4 frames
		// Your animation system uses row 0 = bottom row.
		constexpr int totalCols = 11;
		constexpr int totalRows = 2;
		const float frameW = 1.0f / static_cast<float>(totalCols);
		const float frameH = 1.0f / static_cast<float>(totalRows);

		std::vector<glm::vec4> frames;
		frames.reserve(16);

		// top row first (visual first row)
		for (int col = 0; col < 11; ++col) {
			frames.emplace_back(col * frameW, 1.0f * frameH, frameW, frameH);
		}

		// bottom row next
		for (int col = 0; col < 4; ++col) {
			frames.emplace_back(col * frameW, 0.0f, frameW, frameH);
		}

		return frames;
	}

	/**
	 * @brief Estimates text width.
	 * @param text Parameter for text.
	 * @param font Parameter for font.
	 * @param scale Parameter for scale.
	 * @return Result produced by this operation.
	 */
	float EstimateTextWidth(const std::string& text, FontSystem::Font* font, float scale) {
		if (!font) {
			return static_cast<float>(text.size()) * 18.0f * scale;
		}

		float width = 0.0f;
		for (char c : text) {
			const FontSystem::Character* ch = font->GetCharacter(c);
			if (ch) {
				width += static_cast<float>(ch->advance >> 6) * scale;
			}
		}
		return width;
	}
}

namespace {

	/**
	 * @brief Initializes default collider.
	 * @param obj Parameter for obj.
	 */
	void InitDefaultCollider(GameObject* obj) {
		if (!obj) {
			return;
		}

		Math::Vector2D col = obj->GetColliderSize();
		// Don't overwrite artist / prefab data if collider already exists
		if (col.x > 0.0f && col.y > 0.0f) {
			return;
		}

		glm::vec3 s = obj->GetScaleGLM();
		obj->SetColliderSize(Math::Vector2D{ s.x, s.y });
		obj->SetColliderOffset(Math::Vector2D{ 0.0f, 0.0f });
	}

	constexpr const char* kCutsceneSkipTexture = "../assets/cutscene_skip.png";
	constexpr glm::vec2 kCutsceneSkipSize{ 320.0f, 156.0f };
	constexpr float kCutsceneSkipMarginRight = 24.0f;
	constexpr float kCutsceneSkipMarginTop = 20.0f;
	constexpr int kCutsceneSkipSortOrder = 5000;
}

/**
 * @brief Triggers customer payment feedback.
 * @param tableObjectID Parameter for table object id.
 * @param amount Parameter for amount.
 * @return Result produced by this operation.
 */
void Scene::TriggerCustomerPaymentFeedback(int tableObjectID, int amount) {
	GameObject* tableObj = GetGameObjectByID(tableObjectID);
	if (!tableObj) {
		return;
	}

	FontSystem::Font* font = ResourceManager::Instance().GetFont("payment_popup_font");
	if (!font) {
		font = FontSystem::FontManager::Instance().LoadFont(
			"payment_popup_font",
			FilePaths::Fonts::TO_THE_POINT,
			72
		);
	}

	static const std::vector<glm::vec4> kStarFrames = CreateCustomerPaymentStarFrames();
	constexpr float kFrameDuration = 0.045f;
	const float kLifetime = static_cast<float>(kStarFrames.size()) * kFrameDuration + 0.05f;

	const glm::vec3 tablePos = tableObj->GetPositionGLM();
	const std::string layer = GetObjectLayer(tableObjectID).empty() ? "6" : GetObjectLayer(tableObjectID);

	// Spawn ONE animated burst only
	if (amount > 0) {
		GameObject* fx = SpawnAnimatedSprite(
			"../assets/staranim-Sheet.png",
			glm::vec3(tablePos.x, tablePos.y - 12.0f, tablePos.z),
			glm::vec2(160.0f, 160.0f),
			kStarFrames,
			kFrameDuration,
			false,
			layer
		);

		if (fx) {
			fx->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
			fx->SetMovableByPhysics(false);
			fx->EnableShadow(false);
			fx->SetRenderSortOrder(350);

			runtimeAnimatedFx_.push_back(RuntimeAnimatedFx{
				fx->GetID(),
				fx->GetScaleGLM(),
				0.0f,
				kLifetime
				});
		}
	}

	// Floating text
	const std::string amountText = (amount > 0)
		? ("+" + std::to_string(amount))
		: "0";

	floatingWorldTextFx_.push_back(FloatingWorldTextFx{
		amountText,
		glm::vec2(tablePos.x, tablePos.y - 28.0f),
		glm::vec2(0.0f, -60.0f),
		amount > 0 ? glm::vec4(1.0f, 0.92f, 0.30f, 1.0f)
				   : glm::vec4(0.85f, 0.85f, 0.85f, 1.0f),
		1.0f,
		0.0f,
		0.9f,
		layer
		});
}

/**
 * @brief Updates runtime animated fx.
 * @param dt Frame delta time in seconds.
 * @return Result produced by this operation.
 */
void Scene::UpdateRuntimeAnimatedFx(float dt) {
	runtimeAnimatedFx_.erase(
		std::remove_if(runtimeAnimatedFx_.begin(), runtimeAnimatedFx_.end(),
			[&](RuntimeAnimatedFx& fx) {
				GameObject* obj = GetGameObjectByID(fx.objectId);
				if (!obj) {
					return true;
				}

				fx.elapsed += dt;
				const float t = std::clamp(fx.elapsed / std::max(fx.lifetime, 0.0001f), 0.0f, 1.0f);

				// Tiny scale pop
				const float scaleMul = 0.85f + 0.30f * t;
				obj->SetScale(glm::vec3(
					fx.baseScale.x * scaleMul,
					fx.baseScale.y * scaleMul,
					fx.baseScale.z
				));

				// Fade near the end
				float alpha = 1.0f;
				if (t > 0.70f) {
					alpha = 1.0f - ((t - 0.70f) / 0.30f);
				}
				obj->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, std::clamp(alpha, 0.0f, 1.0f)));

				if (fx.elapsed >= fx.lifetime) {
					if (GetGameObjectByID(fx.objectId)) {
						DespawnByID(fx.objectId);
					}
					return true;
				}
				return false;
			}),
		runtimeAnimatedFx_.end()
	);
}

/**
 * @brief Updates floating world text fx.
 * @param dt Frame delta time in seconds.
 * @return Result produced by this operation.
 */
void Scene::UpdateFloatingWorldTextFx(float dt) {
	for (auto& fx : floatingWorldTextFx_) {
		fx.elapsed += dt;
		fx.pos += fx.velocity * dt;
	}

	floatingWorldTextFx_.erase(
		std::remove_if(floatingWorldTextFx_.begin(), floatingWorldTextFx_.end(),
			[](const FloatingWorldTextFx& fx) {
				return fx.elapsed >= fx.lifetime;
			}),
		floatingWorldTextFx_.end()
	);
}

/**
 * @brief Renders floating world text fx.
 * @param projection Parameter for projection.
 * @param pauseActive Parameter for pause active.
 * @param cutsceneActive Parameter for cutscene active.
 * @return Result produced by this operation.
 */
void Scene::RenderFloatingWorldTextFx(const glm::mat4& projection, bool pauseActive, bool cutsceneActive) {
	if (pauseActive || cutsceneActive) {
		return;
	}

	FontSystem::Font* font = ResourceManager::Instance().GetFont("payment_popup_font");
	if (!font) {
		return;
	}

	for (const auto& fx : floatingWorldTextFx_) {
		if (!fx.layer.empty()) {
			Layer* layer = GetLayer(fx.layer);
			if (layer && (!layer->IsEnabled() || !layer->IsVisible())) {
				continue;
			}
		}

		const float t = std::clamp(fx.elapsed / std::max(fx.lifetime, 0.0001f), 0.0f, 1.0f);
		const float alpha = 1.0f - t;
		const float scale = fx.baseScale * (1.0f + 0.10f * t);

		FontSystem::Text text;
		text.SetFont(font);
		text.SetText(fx.text);
		text.SetColor(glm::vec4(fx.color.r, fx.color.g, fx.color.b, fx.color.a * alpha));
		text.SetScale(scale);
		text.SetRotationMode(FontSystem::Text::RotationMode::Block);

		const float textWidth = EstimateTextWidth(fx.text, font, scale);
		text.SetPosition(glm::vec2(fx.pos.x - textWidth * 0.5f, fx.pos.y));

		FontSystem::TextRenderer::Instance().RenderText(text, projection);
	}
}

/**
 * @brief Sets simulation active.
 * @param active Parameter for active.
 * @return Result produced by this operation.
 */
void Scene::SetSimulationActive(bool active) {
	simulationActive = active;

	if (active) {
		animationManager.Play();
	}
	else {
		const std::string levelPath = GetCurrentLevelPath();
		const bool isMainMenu = levelPath.find("main_menu") != std::string::npos;

		if (isMainMenu) {
			animationManager.Play();
		}
		else {
			animationManager.Stop();
		}
	}
}

/**
 * @brief Returns whether simulation active.
 * @return True when the operation succeeds or the condition is met.
 */
bool Scene::IsSimulationActive() const {
	return simulationActive;
}

/**
 * @brief Returns object texture path.
 * @param id Parameter for id.
 * @return Requested value.
 */
const std::string& Scene::GetObjectTexturePath(int id) const {
	return entityManager.GetTexturePath(id);
}

/**
 * @brief Sets object texture path.
 * @param id Parameter for id.
 * @param path Path to process.
 * @return Result produced by this operation.
 */
void Scene::SetObjectTexturePath(int id, const std::string& path) {
	entityManager.SetTexturePath(id, path);
}

// Construction / core lifecycle
Scene::Scene(GraphicsEngine& engine, InputManager& inputMgr, AnimationManager& animMgr,
	MovementManager& moveMgr, PhysicsManager& physicsMgr, CollisionManager& collisionMgr)
	: graphicsEngine(engine), inputManager(inputMgr), animationManager(animMgr),
	movementManager(moveMgr), physicsManager(physicsMgr), collisionManager(collisionMgr) {
	// Allow AnimationManager to find objects
	animationManager.SetEntityManager(&entityManager);
	physicsManager.SetScene(this);
	collisionManager.SetScene(this);

	// Basic default layer used when no explicit layer name is given
	AddLayer("1");
}

/**
 * @brief Loads scene.
 * @param sceneName Parameter for scene name.
 * @return Result produced by this operation.
 */
void Scene::LoadScene(const std::string& sceneName) {
	(void)sceneName;
	currentLevelPath_.clear();

	// Ensure we start EMPTY per rubric (no auto-spawned objects)
	ClearAll();

	if (defaultSceneSetupHook_) {
		defaultSceneSetupHook_(*this);
	}
}

/**
 * @brief Updates this object.
 * @param deltaTime Frame delta time in seconds.
 * @param window Parameter for window.
 * @return Result produced by this operation.
 */
void Scene::Update(float deltaTime, GLFWwindow* window) {
#ifdef _DEBUG
	UpdateAnimationControls();
#endif

	UpdateCutscenePhase(deltaTime);
	if (!UpdateInputPhase(deltaTime)) {
		return;
	}

	const float physicsDt = physicsStep_.resolveDt(inputManager, deltaTime);
	lastPhysicsDt_ = physicsDt;

	UpdateSimulationPhase(deltaTime, physicsDt);
	HandleDeferredLoads();
	UpdateUiPhase(deltaTime, window);
	FinalizeFramePhase(deltaTime);
}

// Advances both cutscene state machines and level-transition state.
/**
 * @brief Updates cutscene phase.
 * @param deltaTime Frame delta time in seconds.
 * @return Result produced by this operation.
 */
void Scene::UpdateCutscenePhase(float deltaTime) {

	// Drive both cutscene players every frame so transitions progress
	UpdateCutsceneTransitioned(deltaTime);
	UpdateCutscene(deltaTime);

	UpdateLevelTransition();
}

// Consumes input, toggles editor/FPS UI, and may clear the whole scene.
/**
 * @brief Updates input phase.
 * @param deltaTime Frame delta time in seconds.
 * @return Result produced by this operation.
 */
bool Scene::UpdateInputPhase(float deltaTime) {
#if defined(_DEBUG) && !defined(ENABLE_DEBUG_UI)
	(void)deltaTime;
#endif
	// Handle pending pause audio (pause channels after fade completes)
#ifndef _DEBUG
	if (pauseAudioPending_ && audioManager_) {
		pauseAudioTimer_ -= deltaTime;
		if (pauseAudioTimer_ <= 0.0f) {
			// Fade completed, now pause the channels to stop playback
			if (!pauseMusicChannel_.empty()) {
				audioManager_->PauseChannel(pauseMusicChannel_);
			}
			if (!pauseAmbienceChannel_.empty()) {
				audioManager_->PauseChannel(pauseAmbienceChannel_);
			}
			pauseAudioPending_ = false;
			std::cout << "[Scene] Paused audio channels after fade" << std::endl;
		}
	}
#endif

	if (pendingClear_) {
		ClearAll();
		RebuildColliders();
		pendingClear_ = false;
		return false;
	}

	// While any cutscene is active, discard input so UI/buttons cannot be pressed (this might need tweaking later, for future cutscenes that need input)
	if (IsAnyCutsceneActive()) {
		const bool spaceHeld = inputManager.IsKeyPressed(GLFW_KEY_SPACE);
		if (spaceHeld && !cutsceneSkipSpaceHeld_ && !cutsceneSkipConsumed_) {
			SkipActiveCutscene();
			cutsceneSkipConsumed_ = true;
		}
		cutsceneSkipSpaceHeld_ = spaceHeld;
		inputManager.ClearState();
	}
	else {
		cutsceneSkipSpaceHeld_ = false;
		cutsceneSkipConsumed_ = false;
#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
		inputCommandHandler.ProcessCommands(inputManager, physicsManager, movementManager, spriteID, useForces_, showAuxDebug_);
#endif
	}

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	if (inputManager.IsKeyJustPressed(GLFW_KEY_L)) {
		mLevelEditor.Toggle();
	}

#endif

#ifndef _DEBUG

	// Toggle FPS display with F1 in Release
	if (inputManager.IsKeyJustPressed(GLFW_KEY_F1)) {
		showFPS_ = !showFPS_;
		if (showFPS_) {
			// Lazy-load a small font for FPS
			FontSystem::Font* f = ResourceManager::Instance().GetFont("fps_font");
			if (!f) {
				f = FontSystem::FontManager::Instance().LoadFont("fps_font", FilePaths::Fonts::TO_THE_POINT, 48);
			}

			if (f) {
				fpsText_.SetFont(f);
				fpsText_.SetColor(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)); // yellow for visibility
				fpsText_.SetScale(1.5f); // 1.5x size for better visibility
				// Position will be set dynamically in update loop to align to right side
				fpsAccumTime_ = 0.0f;
				fpsAccumFrames_ = 0;
				fpsValue_ = 60; // Start with a visible value
				std::string fpsStr = std::string("FPS: ") + std::to_string(fpsValue_);
				fpsText_.SetText(fpsStr);
				// Set initial position on the right side
				float estimatedTextWidth = static_cast<float>(fpsStr.length()) * 20.0f * fpsText_.GetScale();
				float rightPadding = 20.0f;
				float topPadding = 60.0f;
				fpsText_.SetPosition(glm::vec2(static_cast<float>(GraphicsEngine::kRefW) - estimatedTextWidth - rightPadding, topPadding));
			}
		}
	}

#endif

	return true;
}

// Updates game logic, hooks, forces/physics, NPC movement, and collision constraints.
/**
 * @brief Updates simulation phase.
 * @param deltaTime Frame delta time in seconds.
 * @param physicsDt Parameter for physics dt.
 * @return Result produced by this operation.
 */
void Scene::UpdateSimulationPhase(float deltaTime, float physicsDt) {
	// Always update logic (menu buttons need this even with simulation disabled)
	logicManager.StartAll(*this);
	logicManager.UpdateAll(deltaTime, *this, inputManager);

	// Seat customers at tables once
	if (customerUpdateHook_) {
		customerUpdateHook_(physicsDt, *this);
	}

	if (simulationActive && simulationUpdateHook_) {
		simulationUpdateHook_(deltaTime, *this);
	}

	if (simulationActive) {
		if (useForces_) {
			physicsManager.UpdatePhysics(physicsDt, entityManager, inputManager);
		}
		const collision::WalkArea walk = GetWalkArea();
		npcSystem.Update(physicsDt, entityManager, collisionManager, walk);
		HandlePlayerCollisions(physicsDt, entityManager);
		ApplyFinalConstraints(entityManager);

		// Update 3D audio listener position to the center of the reference canvas
		if (audioManager_) {
			float listenerX = static_cast<float>(GraphicsEngine::kRefW) * 0.5f;
			float listenerY = static_cast<float>(GraphicsEngine::kRefH) * 0.5f;
			audioManager_->SetListenerPosition(listenerX, listenerY, 0.0f);
		}
	}
}

// Rebuilds a new level, resets simulation/input state, and triggers post-load hooks.
/**
 * @brief Handles deferred loads.
 * @return Result produced by this operation.
 */
void Scene::HandleDeferredLoads() {
	// Process deferred level load after logic iteration completes
	if (hasPendingLevel_) {
		if (!pendingLevelPath_.empty()) {
			if (!RuntimeLevel::LoadAndBuild(pendingLevelPath_, *this)) {
				std::cerr << "[Scene] Deferred level load failed: " << pendingLevelPath_ << std::endl;
			}
			else {
				SetCurrentLevelPath(pendingLevelPath_);
				RebuildColliders();
				SetSimulationActive(pendingLevelSimActive_);
				inputManager.ClearState(); // avoid stale click replay

				// If we are coming from a cutscene, fade in the new level now
				if (cutTrans_.fadeInAfterLoad) {
					auto& gfx = GetGraphicsEngine();

					// Ensure a fade is active; if not, start a fade-in-only transition
					if (!gfx.IsTransitionActive() || !gfx.IsAtBlackout()) {
						// outSeconds = 0 starts from the current frame, then we only fade in
						gfx.StartSceneTransition(0.1f, cutTrans_.inSeconds);
					}

					gfx.ContinueTransitionFadeIn();
					cutTrans_.fadeInAfterLoad = false;

					// Check if we're loading main menu (from win/lose cutscene)
					// If simulation is NOT active, this is likely the main menu
					// Re-enable layer 10 only when returning to main menu from win/lose
					if (postLevelLoadHook_) {
						postLevelLoadHook_(*this, pendingLevelSimActive_);
					}
				}

				LEPANELFONTS::EnsureFontsForTextObjectsLoaded();
				//Economy::Reset();
			}
		}

		hasPendingLevel_ = false;
		pendingLevelPath_.clear();
	}
}

// Advances particles and UI slide animations; may draw debug overlays.
/**
 * @brief Updates ui phase.
 * @param deltaTime Frame delta time in seconds.
 * @param window Parameter for window.
 * @return Result produced by this operation.
 */
void Scene::UpdateUiPhase(float deltaTime, GLFWwindow* window) {
	// Update runtime particles
	particleSystem_.Update(deltaTime, entityManager);

	// Update any UI slide-in animations regardless of simulation flag
	UpdateUiSlides(deltaTime);

	UpdateRuntimeAnimatedFx(deltaTime);
	UpdateFloatingWorldTextFx(deltaTime);

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	debugVisualizer.DrawDebugInfo(entityManager, collisionManager, movementManager, spriteID, showAuxDebug_);
#endif
	(void)window;
}

// Despawns queued entities, updates FPS text, and handles pause-overlay toggles.
/**
 * @brief Performs finalize frame phase.
 * @param deltaTime Frame delta time in seconds.
 * @return Result produced by this operation.
 */
void Scene::FinalizeFramePhase(float deltaTime) {
#ifdef _DEBUG
	(void)deltaTime;
#endif
	for (int id : pendingDespawns_) {
		DespawnByID(id);
	}

	pendingDespawns_.clear();

#ifndef _DEBUG
	// Update FPS accumulator when enabled (release builds only)
	if (showFPS_) {
		fpsAccumTime_ += deltaTime;
		fpsAccumFrames_ += 1;
		if (fpsAccumTime_ >= fpsUpdateInterval_) {
			float avg = static_cast<float>(fpsAccumFrames_) / fpsAccumTime_;
			fpsValue_ = static_cast<int>(avg + 0.5f);
			fpsAccumTime_ = 0.0f;
			fpsAccumFrames_ = 0;
			std::string fpsStr = std::string("FPS: ") + std::to_string(fpsValue_);
			fpsText_.SetText(fpsStr);

			// Position FPS text on the right side of the screen
			// Use reference canvas width (kRefW) since projection uses reference space
			float estimatedTextWidth = static_cast<float>(fpsStr.length()) * 20.0f * fpsText_.GetScale();
			float rightPadding = 20.0f;
			float topPadding = 60.0f;
			fpsText_.SetPosition(glm::vec2(static_cast<float>(GraphicsEngine::kRefW) - estimatedTextWidth - rightPadding, topPadding));
		}
	}
#endif

	// Handle ESC to toggle pause overlay in Release
#ifndef _DEBUG
	if (inputManager.IsKeyJustPressed(GLFW_KEY_ESCAPE)) {
		if (IsPauseOverlayActive()) {
			HidePauseOverlay();
			RequestResumeFromPauseOverlay();
			inputManager.ConsumeNextKeyPress(GLFW_KEY_ESCAPE);
		}
		else if (IsSimulationActive()) {
			// Only allow pause during gameplay (not in main menu)
			ShowPauseOverlay();
			inputManager.ConsumeNextKeyPress(GLFW_KEY_ESCAPE);
		}
	}
#endif

#ifndef _DEBUG
	if (resumeFromPausePending_ && !pauseOverlayActive_) {
		SetSimulationActive(true);
		resumeFromPausePending_ = false;
	}
#endif

}

/**
 * @brief Resets resize baseline.
 * @return Result produced by this operation.
 */
void Scene::ResetResizeBaseline() {
	resetBaseline_ = true;
}

/**
 * @brief Draws ui.
 * @return Result produced by this operation.
 */
void Scene::DrawUI() {
	if (mLevelEditor.IsEnabled()) {
		mLevelEditor.DrawUI(*this);
	}
}

/**
 * @brief Clears all.
 * @return Result produced by this operation.
 */
void Scene::ClearAll() {
	// Stop all object-bound audio before tearing down the scene
	StopAllObjectAudio();

	// Clear scripts first so they no longer reference objects
	logicManager.Clear(*this);
	entityManager.Clear();
	animationManager.Clear();
	movementManager.Clear();
	npcSystem.Clear();
	if (customerResetHook_) {
		customerResetHook_(*this);
	}
	//ClearMenuButtonTexts();
	runtimeAnimatedFx_.clear();
	floatingWorldTextFx_.clear();

	spriteID = -1;
	dinoID = -1;
	otherID = -1;
	otherID2 = -1;
}

/**
 * @brief Performs request clear all.
 * @return Result produced by this operation.
 */
void Scene::RequestClearAll() {
	pendingClear_ = true;
}

/**
 * @brief Returns graphics engine.
 * @return Requested value.
 */
GraphicsEngine& Scene::GetGraphicsEngine() {
	return graphicsEngine;
}

/**
 * @brief Returns graphics engine.
 * @return Requested value.
 */
const GraphicsEngine& Scene::GetGraphicsEngine() const {
	return graphicsEngine;
}

/**
 * @brief Sets player id.
 * @param id Parameter for id.
 * @return Result produced by this operation.
 */
void Scene::SetPlayerID(int id) {
	spriteID = id;
}

/**
 * @brief Performs spawn static sprite.
 * @param texturePath Parameter for texture path.
 * @param position Parameter for position.
 * @param size Parameter for size.
 * @param layer Parameter for layer.
 * @return Result produced by this operation.
 */
GameObject* Scene::SpawnStaticSprite(const std::string& texturePath,
	const glm::vec3 position,
	const glm::vec2 size,
	const std::string& layer) {
	GameObject* obj = entityManager.SpawnStaticSprite(texturePath, position, size);

	if (obj) {
		int id = obj->GetID();
		AssignObjectToLayer(id, layer);

		InitDefaultCollider(obj);
	}

	// Disabled by default, controlled by JSON
	obj->EnableShadow(false);

	obj->SetShadowSize(glm::vec2(size.x * 0.8f, size.y * 0.33f)); // ellipse sized to sprite
	obj->SetShadowOffset(glm::vec2(0.0f, 55.0f));       // sit near feet (tweak per origin)
	obj->SetShadowOpacity(0.65f);

	return obj;
}

/**
 * @brief Performs spawn animated sprite.
 * @param texturePath Parameter for texture path.
 * @param position Parameter for position.
 * @param size Parameter for size.
 * @param frames Parameter for frames.
 * @param frameDuration Parameter for frame duration.
 * @param loop Parameter for loop.
 * @param layer Parameter for layer.
 * @return Result produced by this operation.
 */
GameObject* Scene::SpawnAnimatedSprite(const std::string& texturePath,
	const glm::vec3 position,
	const glm::vec2 size,
	const std::vector<glm::vec4>& frames,
	float frameDuration, bool loop,
	const std::string& layer) {
	GameObject* obj = entityManager.SpawnAnimatedSprite(texturePath, position, size, frames, frameDuration, loop);

	if (obj) {
		int id = obj->GetID();
		AssignObjectToLayer(id, layer);
		InitDefaultCollider(obj);

		// This is the missing step:
		animationManager.AttachRuntimeAnimation(id, frames, frameDuration, loop);
	}

	if (obj) {
		obj->EnableShadow(false);
		obj->SetShadowSize(glm::vec2(size.x * 0.8f, size.y * 0.33f));
		obj->SetShadowOffset(glm::vec2(0.0f, 55.0f));
		obj->SetShadowOpacity(0.65f);
	}

	return obj;
}

/**
 * @brief Performs spawn static sprite at same pos.
 * @param ownerID Parameter for owner id.
 * @param texturePath Parameter for texture path.
 * @param width Width value in pixels.
 * @param height Height value in pixels.
 * @param layer Parameter for layer.
 * @return Result produced by this operation.
 */
GameObject* Scene::SpawnStaticSpriteAtSamePos(int ownerID,
	const std::string& texturePath,
	float width,
	float height,
	const std::string& layer) {
	GameObject* owner = GetGameObjectByID(ownerID);
	if (!owner) {
		return nullptr;
	}

	glm::vec3 pos = owner->GetPositionGLM();

	GameObject* obj = SpawnStaticSprite(
		texturePath,
		glm::vec3(pos.x, pos.y, pos.z),
		glm::vec2(width, height),
		layer
	);
	if (!obj) {
		return nullptr;
	}

	const int id = obj->GetID();

	SetObjectTexturePath(id, texturePath);

	obj->SetColliderSize(Math::Vector2D(width, height));
	obj->SetColliderOffset(Math::Vector2D(0.0f, 0.0f));

	Scene::Defaults defs{};
	defs.pos = glm::vec3(pos.x, pos.y, pos.z);
	defs.size = glm::vec2(width, height);
	defs.rot = 0.0f;
	defs.colSize = glm::vec2(width, height);
	defs.colOff = glm::vec2(0.0f, 0.0f);
	defs.vel = glm::vec2(0.0f, 0.0f);
	defs.texture = texturePath;
	defs.tag = "ingredient";      // feel free to use something else
	defs.layer = layer;
	SetDefaults(id, defs);

	ClampToWalkArea(obj);

	return obj;
}

/**
 * @brief Returns game object by id.
 * @param targetID Parameter for target id.
 * @return Requested value.
 */
GameObject* Scene::GetGameObjectByID(int targetID) {
	return entityManager.GetByID(targetID);
}

/**
 * @brief Returns all objects raw.
 * @return Requested value.
 */
std::vector<GameObject*> Scene::GetAllObjectsRaw() {
	return entityManager.GetAllObjects();
}

/**
 * @brief Returns object storage raw.
 * @return Requested value.
 */
const std::vector<std::unique_ptr<GameObject>>& Scene::GetObjectStorageRaw() const {
	return entityManager.GetObjectStorage();
}

/**
 * @brief Performs despawn by id.
 * @param targetID Parameter for target id.
 * @return Result produced by this operation.
 */
void Scene::DespawnByID(int targetID) {
	// Play destroy audio before removing the object
	PlayDestroyAudio(targetID);

	// Remove stale layer membership and metadata before despawning.
	auto defaultsIt = defaults_.find(targetID);
	if (defaultsIt != defaults_.end()) {
		const std::string& layerName = defaultsIt->second.layer;
		if (!layerName.empty()) {
			auto layerIt = layers.find(layerName);
			if (layerIt != layers.end()) {
				layerIt->second.RemoveObject(targetID);
			}
		}

		defaults_.erase(defaultsIt);
	}

	logicManager.RemoveAllFor(targetID, *this);

	objectTags_.erase(targetID);
	mTexturePathByID.erase(targetID);

	animationManager.RemoveAnimator(targetID);

	// Remove from entity manager (handles transforms too)
	entityManager.DespawnByID(targetID);
}

/**
 * @brief Collects renderable pointers.
 * @param out Output value for out.
 * @return Result produced by this operation.
 */
void Scene::CollectRenderablePointers(std::vector<GameObject*>& out) {
	out.clear();
	const bool cutsceneActive = IsAnyCutsceneActive();

	const auto& all = entityManager.GetObjectStorage();
	out.reserve(all.size());

	for (const auto& objPtr : all) {
		GameObject* g = objPtr.get();
		if (!g) {
			continue;
		}

		// Skip if object is marked invisible in defaults (per-object visibility)
		const int objId = g->GetID();
		auto defIt = defaults_.find(objId);
		if (defIt != defaults_.end()) {
			if (!defIt->second.visible) {
				continue;
			}
		}

		const std::string layerName = (defIt != defaults_.end()) ? defIt->second.layer : "";

		// While cutscenes are active, render only cutscene/pause overlay layers.
		// This prevents gameplay objects/HUD strips from bleeding through in debug builds.
		if (cutsceneActive && layerName != cutTrans_.uiLayer && layerName != cutscene_.uiLayer && layerName != "999999") {
			continue;
		}

		// Check the layer's visibility flag
		Layer* layer = GetObjectLayerPtr(objId);
		if (layer) {
			if (!layer->IsEnabled()) {
				continue;
			}

			if (!layer->IsVisible()) {
				continue;
			}
		}

		// Set the render layer on the object for use in GraphicsEngine
		const std::string& texturePath = GetObjectTexturePath(objId);
		const bool isFootstepVfx = texturePath.find("run_vfx.png") != std::string::npos;
		g->SetRenderLayer(isFootstepVfx ? 0 : GetLayerSortKeyCached(layerName));

		out.push_back(g);
	}

	std::sort(
		out.begin(),
		out.end(),
		[&](GameObject* a, GameObject* b) {
			int la = a->GetRenderLayer();
			int lb = b->GetRenderLayer();

			// Different layers: smaller layer number drawn first (behind),
			// higher layer number drawn later (on top)
			if (la != lb) {
				return la < lb;
			}

			// Same layer: use explicit sort order first
			int sa = a->GetRenderSortOrder();
			int sb = b->GetRenderSortOrder();
			if (sa != sb) {
				return sa < sb;
			}

			// Same layer & same sort order - lower Y drawn first (higher on screen appears behind)
			return a->GetPosition().y < b->GetPosition().y;
		}
	);
}

/**
 * @brief Sets scene background.
 * @param texturePath Parameter for texture path.
 * @return Result produced by this operation.
 */
void Scene::SetSceneBackground(const std::string& texturePath) {
	sceneBackgroundPath_ = texturePath;
	graphicsEngine.SetBackground(texturePath);
}

/**
 * @brief Sets scene background overlay.
 * @param texturePath Parameter for texture path.
 * @return Result produced by this operation.
 */
void Scene::SetSceneBackgroundOverlay(const std::string& texturePath) {
	sceneBackgroundOverlayPath_ = texturePath;
	graphicsEngine.SetBackgroundOverlay(texturePath);
}

/**
 * @brief Clears scene background overlay.
 * @return Result produced by this operation.
 */
void Scene::ClearSceneBackgroundOverlay() {
	sceneBackgroundOverlayPath_.clear();
	graphicsEngine.ClearBackgroundOverlay();
}

/**
 * @brief Sets transform from level.
 * @param id Parameter for id.
 * @param pos Parameter for pos.
 * @param scale Parameter for scale.
 * @param rotationDeg Parameter for rotation deg.
 * @return Result produced by this operation.
 */
void Scene::SetTransformFromLevel(int id,
	const glm::vec3& pos,
	const glm::vec3& scale,
	float rotationDeg) {
	// Convert degrees to radians ONCE here
	const float rotationRad = rotationDeg * 3.14159265358979323846f / 180.0f;

	// EntityManager transform wrappers forward directly to the owning GameObject.
	entityManager.SetPosition(id, pos);
	entityManager.SetScale(id, scale);
	entityManager.SetRotation(id, rotationRad);

	GameObject* obj = GetGameObjectByID(id);
	if (obj) {
		obj->SetPosition(pos);
		obj->SetScale(scale);
		obj->SetRotation(rotationRad, glm::vec3(0.0f, 0.0f, 1.0f));
	}

	collisionManager.MarkStaticStateDirty();
}

/**
 * @brief Returns whether animations.
 * @param id Parameter for id.
 * @return True when the operation succeeds or the condition is met.
 */
bool Scene::HasAnimations(int id) const {
	return animationManager.HasAnimator(id);
}

/**
 * @brief Returns animation list.
 * @param id Parameter for id.
 * @return Requested value.
 */
std::vector<std::string> Scene::GetAnimationList(int id) const {
	return animationManager.GetAnimationNames(id);
}

/**
 * @brief Returns current animation name.
 * @param id Parameter for id.
 * @return Requested value.
 */
std::string Scene::GetCurrentAnimationName(int id) const {
	return animationManager.GetCurrentAnimation(id);
}

/**
 * @brief Sets animation.
 * @param objID Parameter for obj id.
 * @param animName Parameter for anim name.
 * @return Result produced by this operation.
 */
void Scene::SetAnimation(int objID, const std::string& animName) {
	animationManager.SetAnimation(objID, animName);
}

/**
 * @brief Performs attach player animations.
 * @param objID Parameter for obj id.
 * @return Result produced by this operation.
 */
void Scene::AttachPlayerAnimations(int objID) {
	animationManager.AttachPlayerAnimations(objID);
}

/**
 * @brief Performs attach dino animations.
 * @param objID Parameter for obj id.
 * @return Result produced by this operation.
 */
void Scene::AttachDinoAnimations(int objID) {
	animationManager.AttachDinoAnimations(objID);
}

/**
 * @brief Performs attach customers animations.
 * @param objID Parameter for obj id.
 * @return Result produced by this operation.
 */
void Scene::AttachCustomersAnimations(int objID) {
	animationManager.AttachCustomersAnimations(objID, GetObjectTexturePath(objID));
}

/**
 * @brief Performs attach customers animations.
 * @param objID Parameter for obj id.
 * @param texturePath Parameter for texture path.
 * @return Result produced by this operation.
 */
void Scene::AttachCustomersAnimations(int objID, const std::string& texturePath) {
	animationManager.AttachCustomersAnimations(objID, texturePath);
}

/**
 * @brief Performs attach work vfx cut animations.
 * @param objID Parameter for obj id.
 * @return Result produced by this operation.
 */
void Scene::AttachWorkVfxCutAnimations(int objID) {
	animationManager.AttachWorkVfxCutAnimations(objID);
}

/**
 * @brief Performs attach work vfx grill animations.
 * @param objID Parameter for obj id.
 * @return Result produced by this operation.
 */
void Scene::AttachWorkVfxGrillAnimations(int objID) {
	animationManager.AttachWorkVfxGrillAnimations(objID);
}

/**
 * @brief Performs attach work vfx stove animations.
 * @param objID Parameter for obj id.
 * @return Result produced by this operation.
 */
void Scene::AttachWorkVfxStoveAnimations(int objID) {
	animationManager.AttachWorkVfxStoveAnimations(objID);
}

/**
 * @brief Performs attach menu animations.
 * @param objID Parameter for obj id.
 * @return Result produced by this operation.
 */
void Scene::AttachMenuAnimations(int objID) {
	animationManager.AttachMenuAnimations(objID);
}

/**
 * @brief Performs mark animated.
 * @param id Parameter for id.
 * @param state Parameter for state.
 * @return Result produced by this operation.
 */
void Scene::MarkAnimated(int id, bool state) {
	if (!state) {
		// Turning OFF animation: remove any per-object animation state
		if (GameObject* obj = GetGameObjectByID(id)) {
			// Ensure it renders the full texture as a static sprite
			obj->SetUVRect({ 0.f, 0.f, 1.f, 1.f });
		}
	}
	else {
		// Turning ON: no-op here; AttachDinoAnimations() will populate maps.
		// (HasAnimations() will start returning true once frames are attached.)
	}
}

/**
 * @brief Performs attach logic for tag.
 * @param id Parameter for id.
 * @param tag Parameter for tag.
 * @return Result produced by this operation.
 */
void Scene::AttachLogicForTag(int id, const std::string& tag) {
	// ALWAYS wipe old logic from this object
	logicManager.RemoveAllFor(id, *this);

	if (tagLogicBinder_) {
		tagLogicBinder_(*this, id, tag);
	}
}

/**
 * @brief Sets object tag.
 * @param id Parameter for id.
 * @param tag Parameter for tag.
 * @return Result produced by this operation.
 */
void Scene::SetObjectTag(int id, const std::string& tag) {
	objectTags_[id] = tag;
}

/**
 * @brief Returns object tag.
 * @param id Parameter for id.
 * @return Requested value.
 */
std::string Scene::GetObjectTag(int id) const {
	auto it = objectTags_.find(id);
	if (it != objectTags_.end()) {
		return it->second;
	}

	// Fallback: if not stored, try defaults (still not hardcoding IDs)
	auto defIt = defaults_.find(id);
	if (defIt != defaults_.end() && !defIt->second.tag.empty()) {
		return defIt->second.tag;
	}

	return ""; // unknown/untagged
}

/**
 * @brief Performs tag uses velocity.
 * @param tag Parameter for tag.
 * @return Result produced by this operation.
 */
bool Scene::TagUsesVelocity(const std::string& tag) const {
	if (tagUsesVelocityHook_) {
		return tagUsesVelocityHook_(tag);
	}
	return false;
}

/**
 * @brief Applies tag rules.
 * @param id Parameter for id.
 * @param tag Parameter for tag.
 * @param speedX Parameter for speed x.
 * @param speedY Parameter for speed y.
 * @return Result produced by this operation.
 */
void Scene::ApplyTagRules(int id, const std::string& tag, float speedX, float speedY) {
	if (tagRuleHook_) {
		tagRuleHook_(*this, id, tag, speedX, speedY);
	}
}

/**
 * @brief Returns movement manager.
 * @return Requested value.
 */
MovementManager& Scene::GetMovementManager() {
	return movementManager;
}

/**
 * @brief Returns movement manager.
 * @return Requested value.
 */
const MovementManager& Scene::GetMovementManager() const {
	return movementManager;
}

/**
 * @brief Returns collision manager.
 * @return Requested value.
 */
CollisionManager& Scene::GetCollisionManager() {
	return collisionManager;
}

/**
 * @brief Returns collision manager.
 * @return Requested value.
 */
const CollisionManager& Scene::GetCollisionManager() const {
	return collisionManager;
}

/**
 * @brief Returns collision world.
 * @return Requested value.
 */
collision::World& Scene::GetCollisionWorld() {
	return collisionManager.GetCollisionWorld();
}

/**
 * @brief Returns collision world.
 * @return Requested value.
 */
const collision::World& Scene::GetCollisionWorld() const {
	return collisionManager.GetCollisionWorld();
}

/**
 * @brief Returns layer sort key cached.
 * @param layerName Parameter for layer name.
 * @return Requested value.
 */
int Scene::GetLayerSortKeyCached(const std::string& layerName) const {
	auto it = layerSortKeyCache_.find(layerName);
	if (it != layerSortKeyCache_.end()) {
		return it->second;
	}

	int result = 1;
	if (!layerName.empty()) {
		result = 0;
		for (char c : layerName) {
			if (!std::isdigit(static_cast<unsigned char>(c))) {
				result = 1000000;
				break;
			}
			result = result * 10 + (c - '0');
		}
	}

	layerSortKeyCache_.emplace(layerName, result);
	return result;
}

/**
 * @brief Adds layer.
 * @param name Parameter for name.
 * @return Result produced by this operation.
 */
void Scene::AddLayer(const std::string& name) {
	layers.try_emplace(name, name); // Only add if missing
	layerSortKeyCache_.erase(name);
	collisionManager.MarkStaticStateDirty();
}

/**
 * @brief Returns layer.
 * @param name Parameter for name.
 * @return Requested value.
 */
Layer* Scene::GetLayer(const std::string& name) {
	auto it = layers.find(name);
	return it != layers.end() ? &(it->second) : nullptr;
}

/**
 * @brief Returns all layers.
 * @return Requested value.
 */
const std::unordered_map<std::string, Layer>& Scene::GetAllLayers() const {
	return layers;
}

/**
 * @brief Returns object layer.
 * @param objectID Identifier of the target object.
 * @return Requested value.
 */
std::string Scene::GetObjectLayer(int objectID) const {
	auto it = defaults_.find(objectID);
	if (it != defaults_.end()) {
		return it->second.layer;
	}

	return "";
}

/**
 * @brief Returns object layer ptr.
 * @param objectID Identifier of the target object.
 * @return Requested value.
 */
Layer* Scene::GetObjectLayerPtr(int objectID) {
	auto it = defaults_.find(objectID);
	if (it == defaults_.end() || it->second.layer.empty()) {
		return nullptr;
	}

	auto layerIt = layers.find(it->second.layer);
	return layerIt != layers.end() ? &(layerIt->second) : nullptr;
}

/**
 * @brief Returns object layer ptr.
 * @param objectID Identifier of the target object.
 * @return Requested value.
 */
const Layer* Scene::GetObjectLayerPtr(int objectID) const {
	auto it = defaults_.find(objectID);
	if (it == defaults_.end() || it->second.layer.empty()) {
		return nullptr;
	}

	auto layerIt = layers.find(it->second.layer);
	return layerIt != layers.end() ? &(layerIt->second) : nullptr;
}

/**
 * @brief Returns whether layer enabled.
 * @param layerName Parameter for layer name.
 * @return True when the operation succeeds or the condition is met.
 */
bool Scene::IsLayerEnabled(const std::string& layerName) const {
	auto it = layers.find(layerName);
	if (it == layers.end()) {
		return true;
	}
	return it->second.IsEnabled();
}

/**
 * @brief Returns whether object layer enabled.
 * @param objectID Identifier of the target object.
 * @return True when the operation succeeds or the condition is met.
 */
bool Scene::IsObjectLayerEnabled(int objectID) const {
	const std::string layerName = GetObjectLayer(objectID);
	if (layerName.empty()) {
		return true;
	}
	return IsLayerEnabled(layerName);
}

/**
 * @brief Performs assign object to layer.
 * @param id Parameter for id.
 * @param newLayer Parameter for new layer.
 * @return Result produced by this operation.
 */
void Scene::AssignObjectToLayer(int id, const std::string& newLayer) {
	std::string layerName = newLayer;
	if (layerName.empty()) layerName = "1";

	const auto defIt = defaults_.find(id);
	if (defIt != defaults_.end()) {
		const std::string& oldLayerName = defIt->second.layer;
		if (!oldLayerName.empty() && oldLayerName != layerName) {
			auto oldLayerIt = layers.find(oldLayerName);
			if (oldLayerIt != layers.end()) {
				oldLayerIt->second.RemoveObject(id);
			}
		}
	}

	// Register object ID with chosen layer (creates if missing)
	Layer& layer = layers[layerName];
	if (layer.GetName().empty()) {
		layer.SetName(layerName);
	}

	layer.AddObject(id);

	// Store on metadata used by the editor + JSON
	defaults_[id].layer = layerName;
	collisionManager.MarkStaticStateDirty();
}

/**
 * @brief Removes layer.
 * @param name Parameter for name.
 * @return Result produced by this operation.
 */
void Scene::RemoveLayer(const std::string& name) {
	auto it = layers.find(name);
	if (it != layers.end()) {
		layers.erase(it);
	}

	layerSortKeyCache_.erase(name);
	collisionManager.MarkStaticStateDirty();
}

/**
 * @brief Performs queue level load.
 * @param path Path to process.
 * @param activateSimulation Parameter for activate simulation.
 * @return Result produced by this operation.
 */
void Scene::QueueLevelLoad(const std::string& path, bool activateSimulation) {
	pendingLevelPath_ = path;
	pendingLevelSimActive_ = activateSimulation;
	hasPendingLevel_ = true;
}

/**
 * @brief Performs request state change.
 * @param newState Parameter for new state.
 * @return Result produced by this operation.
 */
void Scene::RequestStateChange(int newState) {
	pendingState_ = newState;
	hasPendingStateChange_ = true;
}

/**
 * @brief Performs show pause overlay.
 * @return Result produced by this operation.
 */
void Scene::ShowPauseOverlay() {
#ifndef _DEBUG
	if (pauseOverlayActive_) return;
	pauseOverlayActive_ = true;

	std::cout << "[Scene] ShowPauseOverlay()\n";

	// Pause simulation while overlay is active
	SetSimulationActive(false);

	// Fade out level BGM and ambience when entering pause menu, then pause
	if (audioManager_) {
		const float pauseFadeOut = 0.2f; // 200ms fade out for smooth transition

		// Store current volumes before fading so we can restore them on resume
		pausedBgmVolume_ = audioManager_->GetBgmVolume();
		pausedAmbienceVolume_ = audioManager_->GetBgmVolume() * 0.5f;

		// Fade to 0, the AudioManager will handle the fade over time
		// We'll pause the channels after the fade completes (handled in Update or via callback)
		if (!pauseMusicChannel_.empty()) {
			audioManager_->FadeChannel(pauseMusicChannel_, 0.0f, pauseFadeOut);
		}
		if (!pauseAmbienceChannel_.empty()) {
			audioManager_->FadeChannel(pauseAmbienceChannel_, 0.0f, pauseFadeOut);
		}

		// Schedule pause after fade completes
		pauseAudioPending_ = true;
		pauseAudioTimer_ = pauseFadeOut;

		std::cout << "[Scene] Fading out level BGM and ambience for pause menu" << std::endl;
	}

	const std::string uiLayer = "999999";

	// Pause overlay background
	if (GameObject* dim = SpawnStaticSprite(FilePaths::Textures::PAUSED_BG,
		{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f },
		{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) },
		uiLayer)) {
		pauseOverlayObjectIds_.push_back(dim->GetID());
		std::cout << "  [Scene] Pause background id=" << dim->GetID() << "\n";
	}

	auto spawnPauseBtn = [&](const char* tex, const glm::vec2& pos, const std::string& action) {
		if (GameObject* b = SpawnStaticSprite(tex, { pos.x, pos.y, 0.0f }, { 350.0f, 100.0f }, uiLayer)) {
			const int id = b->GetID();
			pauseOverlayObjectIds_.push_back(id);
			SetObjectTexturePath(id, tex);

			if (pauseOverlayButtonBinder_) {
				pauseOverlayButtonBinder_(*this, id, action);
			}
		}
		else {
			std::cout << "  [Scene] ERROR: failed to spawn pause button for action=" << action << "\n";
		}
		};

	spawnPauseBtn(FilePaths::Textures::BTN_RESUME, { 1300.f, 454.f }, "resume");
	spawnPauseBtn(FilePaths::Textures::BTN_HOW, { 1300.f, 584.f }, "howtoplay");
	spawnPauseBtn(FilePaths::Textures::BTN_QUIT, { 1300.f, 714.f }, "quit");
#endif
}

/**
 * @brief Performs request resume from pause overlay.
 * @return Result produced by this operation.
 */
void Scene::RequestResumeFromPauseOverlay() {
#ifndef _DEBUG
	// Defer simulation re-enable until end-of-frame to avoid
	// running physics/collision in the same frame the pause UI click is handled.
	resumeFromPausePending_ = true;
#endif
}

/**
 * @brief Performs hide pause overlay.
 * @return Result produced by this operation.
 */
void Scene::HidePauseOverlay() {
#ifndef _DEBUG
	if (!pauseOverlayActive_) return;
	for (int id : pauseOverlayObjectIds_) {
		DespawnByID(id);
	}
	pauseOverlayObjectIds_.clear();
	pauseOverlayActive_ = false;

	// Cancel any pending pause if we're resuming before fade completes
	pauseAudioPending_ = false;

	// Resume and fade in level BGM and ambience when leaving pause menu
	if (audioManager_) {
		const float pauseFadeIn = 0.2f; // 200ms fade in for smooth transition

		// Resume channels first (they were paused after fade out)
		if (!pauseMusicChannel_.empty()) {
			audioManager_->ResumeChannel(pauseMusicChannel_);
		}
		if (!pauseAmbienceChannel_.empty()) {
			audioManager_->ResumeChannel(pauseAmbienceChannel_);
		}

		// Then fade back to original volumes
		if (!pauseMusicChannel_.empty()) {
			audioManager_->FadeChannel(pauseMusicChannel_, pausedBgmVolume_, pauseFadeIn);
		}
		if (!pauseAmbienceChannel_.empty()) {
			audioManager_->FadeChannel(pauseAmbienceChannel_, pausedAmbienceVolume_, pauseFadeIn);
		}

		std::cout << "[Scene] Resumed and fading in level BGM and ambience after pause menu" << std::endl;
	}
#endif
}

// text objects for menu buttons (disabled for now)
#if 0
/**
 * @brief Creates menu button texts.
 * @return Result produced by this operation.
 */
void Scene::CreateMenuButtonTexts() {
	ClearMenuButtonTexts();

	// Get or load a font for menu buttons
	FontSystem::Font* font = ResourceManager::Instance().GetFont("menu_font");
	if (!font) {
		// Try to load a default font
		font = FontSystem::FontManager::Instance().LoadFont(
			"menu_font",
			"../assets/Font/ChrustyRock-ORLA.ttf",
			48
		);
		if (!font) {
			// Try alternative font
			font = FontSystem::FontManager::Instance().LoadFont(
				"menu_font",
				"../assets/Font/ToThePointRegular-n9y4.ttf",
				48
			);
		}
		if (!font) {
			std::cerr << "[Scene] Failed to load font for menu buttons\n";
			return;
		}
	}

	// Find menu button objects by their tags and create text for them
	const std::vector<std::pair<std::string, std::string>> buttonLabels = {
		{"btn_play", "PLAY"},
		{"btn_howtoplay", "HOW TO PLAY"},
		{"btn_quit", "QUIT"}
	};

	const auto& allObjects = entityManager.GetObjectStorage();
	for (const auto& [tag, label] : buttonLabels) {
		// Find the button object with this tag
		for (const auto& objPtr : allObjects) {
			GameObject* obj = objPtr.get();
			if (!obj) continue;

			// Check if this object has the matching tag
			Scene::Defaults defs = GetDefaults(obj->GetID());
			if (defs.tag == tag) {
				// Create text for this button
				MenuButtonText menuText;
				menuText.buttonID = obj->GetID();
				menuText.label = label;
				menuText.textObj.SetFont(font);
				menuText.textObj.SetText(label);

				// Get button position and size
				glm::vec3 btnPos = obj->GetPositionGLM();
				glm::vec3 btnSize = obj->GetScaleGLM();

				// Calculate actual text width and height using font glyph metrics when available
				FontSystem::Font* f = font;
				float textWidth = 0.0f;
				float maxHeight = 0.0f;
				if (f) {
					for (char c : label) {
						const FontSystem::Character* ch = f->GetCharacter(c);
						if (ch) {
							textWidth += static_cast<float>(ch->advance >> 6);
							maxHeight = std::max(maxHeight, static_cast<float>(ch->size.y));
						}
					}
				}

				// Fallback if metrics not available
				if (textWidth <= 0.0f) {
					float baseFontSize = 36.0f;
					textWidth = label.length() * baseFontSize * 0.4f;
					maxHeight = baseFontSize * 0.8f;
				}

				// Calculate scale to fit text within button bounds (with padding)
				float paddingW = 0.75f; // width padding
				float paddingH = 0.7f;  // height padding
				float targetW = btnSize.x * paddingW;
				float targetH = btnSize.y * paddingH;
				float scaleX = targetW / textWidth;
				float scaleY = targetH / maxHeight;
				float scale = std::min(scaleX, scaleY);

				// Final dimensions
				float finalWidth = textWidth * scale;
				float finalHeight = maxHeight * scale;

				// Position text centered on button (FontSystem renders from top-left baseline aware)
				float textX = btnPos.x - (finalWidth * 0.5f);
				float textY = btnPos.y - (finalHeight * 0.5f);

				menuText.textObj.SetPosition(glm::vec2(textX, textY));
				menuText.textObj.SetScale(scale);
				menuText.textObj.SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // White

				menuButtonTexts_.push_back(menuText);
				break; // Found the button for this tag
			}
		}
	}

	if (!menuButtonTexts_.empty()) {
		std::cout << "[Scene] Created text for " << menuButtonTexts_.size() << " menu buttons\n";
	}
}

/**
 * @brief Renders menu button texts.
 * @return Result produced by this operation.
 */
void Scene::RenderMenuButtonTexts() {
	if (menuButtonTexts_.empty()) {
		return;
	}

	glm::mat4 projection = graphicsEngine.GetProjection();

	// Save current GL viewport so we can restore after drawing
	GLint prevViewport[4];
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	// If we're rendering into the scene FBO (non-default framebuffer), set viewport to FBO size
	GLint boundFBO = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &boundFBO);
	if (boundFBO != 0) {
		// Draw in FBO pixel coords (FBO matches reference canvas size)
		glViewport(0, 0, graphicsEngine.GetSceneWidth(), graphicsEngine.GetSceneHeight());
	}
	else {
		// We're rendering to the default framebuffer: apply the letterboxed viewport so positions match
		graphicsEngine.ApplyViewport();
	}

	// Disable depth test for text rendering and enable alpha blending
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Render each text object - positions are in reference space that matches projection
	for (auto& menuText : menuButtonTexts_) {
		FontSystem::TextRenderer::Instance().RenderText(menuText.textObj, projection);
	}

	// Restore GL state
	glDisable(GL_BLEND);
	// Restore previous viewport (default framebuffer expects full window viewport)
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}

/**
 * @brief Clears menu button texts.
 * @return Result produced by this operation.
 */
void Scene::ClearMenuButtonTexts() {
	menuButtonTexts_.clear();
}

#endif

/**
 * @brief Renders fpstext.
 * @return Result produced by this operation.
 */
void Scene::RenderFPSText() {
#ifndef _DEBUG
	if (!showFPS_) {
		return;
	}

	//static bool firstRender = true;
	//if (firstRender) {
	//	std::cout << "[Scene] RenderFPSText() called for the first time" << std::endl;
	//	firstRender = false;
	//}

	glm::mat4 projection = graphicsEngine.GetProjection();

	// Save current GL viewport so we can restore after drawing
	GLint prevViewport[4];
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	// If we're rendering into the scene FBO (non-default framebuffer), set viewport to FBO size
	GLint boundFBO = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &boundFBO);
	if (boundFBO != 0) {
		// Draw in FBO pixel coords (FBO matches reference canvas size)
		glViewport(0, 0, graphicsEngine.GetSceneWidth(), graphicsEngine.GetSceneHeight());
	}
	else {
		// We're rendering to the default framebuffer: apply the letterboxed viewport so positions match
		graphicsEngine.ApplyViewport();
	}

	// Disable depth test for text rendering and enable alpha blending
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Render FPS text
	FontSystem::TextRenderer::Instance().RenderText(fpsText_, projection);

	// Restore GL state
	glDisable(GL_BLEND);
	// Restore previous viewport (default framebuffer expects full window viewport)
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
#endif
}

/**
 * @brief Performs play spawn audio.
 * @param objectId Identifier of the target object.
 * @return Result produced by this operation.
 */
void Scene::PlaySpawnAudio(int objectId) {
	if (!audioManager_) return;

	auto it = defaults_.find(objectId);
	if (it == defaults_.end()) return;

	const Defaults& defs = it->second;
	if (defs.audioOnSpawn.empty()) return;

	// Check if sound exists and play it at the object's position
	if (audioManager_->HasSound(defs.audioOnSpawn)) {
		audioManager_->PlaySound3D(defs.audioOnSpawn, defs.pos.x, defs.pos.y, defs.pos.z);
		std::cout << "[Scene] Playing spawn audio '" << defs.audioOnSpawn << "' for object " << objectId << std::endl;
	}
	else {
		std::cerr << "[Scene] Spawn audio '" << defs.audioOnSpawn << "' not found in AudioManager" << std::endl;
	}
}

/**
 * @brief Performs play interact audio.
 * @param objectId Identifier of the target object.
 * @return Result produced by this operation.
 */
void Scene::PlayInteractAudio(int objectId) {
	if (!audioManager_) return;

	auto it = defaults_.find(objectId);
	if (it == defaults_.end()) return;

	const Defaults& defs = it->second;
	if (defs.audioOnInteract.empty()) return;

	if (audioManager_->HasSound(defs.audioOnInteract)) {
		audioManager_->PlaySound3D(defs.audioOnInteract, defs.pos.x, defs.pos.y, defs.pos.z);
		std::cout << "[Scene] Playing interact audio '" << defs.audioOnInteract << "' for object " << objectId << std::endl;
	}
	else {
		std::cerr << "[Scene] Interact audio '" << defs.audioOnInteract << "' not found in AudioManager" << std::endl;
	}
}

/**
 * @brief Performs play destroy audio.
 * @param objectId Identifier of the target object.
 * @return Result produced by this operation.
 */
void Scene::PlayDestroyAudio(int objectId) {
	if (!audioManager_) return;

	auto it = defaults_.find(objectId);
	if (it == defaults_.end()) return;

	const Defaults& defs = it->second;
	if (defs.audioOnDestroy.empty()) return;

	if (audioManager_->HasSound(defs.audioOnDestroy)) {
		audioManager_->PlaySound3D(defs.audioOnDestroy, defs.pos.x, defs.pos.y, defs.pos.z);
		std::cout << "[Scene] Playing destroy audio '" << defs.audioOnDestroy << "' for object " << objectId << std::endl;
	}
	else {
		std::cerr << "[Scene] Destroy audio '" << defs.audioOnDestroy << "' not found in AudioManager" << std::endl;
	}
}

/**
 * @brief Performs play processing audio.
 * @param objectId Identifier of the target object.
 * @return Result produced by this operation.
 */
void Scene::PlayProcessingAudio(int objectId) {
	if (!audioManager_) return;

	auto it = defaults_.find(objectId);
	if (it == defaults_.end()) return;

	const Defaults& defs = it->second;
	if (defs.audioOnProcessing.empty()) return;

	if (audioManager_->HasSound(defs.audioOnProcessing)) {
		audioManager_->PlaySound3D(defs.audioOnProcessing, defs.pos.x, defs.pos.y, defs.pos.z);
		std::cout << "[Scene] Playing processing audio '" << defs.audioOnProcessing << "' for object " << objectId << std::endl;
	}
	else {
		std::cerr << "[Scene] Processing audio '" << defs.audioOnProcessing << "' not found in AudioManager" << std::endl;
	}
}

/**
 * @brief Performs stop processing audio.
 * @param objectId Identifier of the target object.
 * @return Result produced by this operation.
 */
void Scene::StopProcessingAudio(int objectId) {
	if (!audioManager_) return;

	auto it = defaults_.find(objectId);
	if (it == defaults_.end()) return;

	const Defaults& defs = it->second;
	if (defs.audioOnProcessing.empty()) return;

	if (audioManager_->HasSound(defs.audioOnProcessing)) {
		audioManager_->StopSound(defs.audioOnProcessing);
		std::cout << "[Scene] Stopped processing audio '" << defs.audioOnProcessing << "' for object " << objectId << std::endl;
	}
}

/**
 * @brief Performs stop all object audio.
 * @return Result produced by this operation.
 */
void Scene::StopAllObjectAudio() {
	if (!audioManager_) return;

	// Stop all audio that was bound to objects
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

// Cutscene management

/**
 * @brief Performs start cutscene.
 * @param imagePaths Parameter for image paths.
 * @param holdSecondsPerImage Parameter for hold seconds per image.
 * @param fadeSeconds Parameter for fade seconds.
 * @param levelJsonPath Parameter for level json path.
 * @param activateSimulation Parameter for activate simulation.
 * @return Result produced by this operation.
 */
void Scene::StartCutscene(const std::vector<std::string>& imagePaths,
	float holdSecondsPerImage,
	float fadeSeconds,
	const std::string& levelJsonPath,
	bool activateSimulation) {
	if (imagePaths.empty()) {
		// If nothing to show, load level immediately
		QueueLevelLoad(levelJsonPath, activateSimulation);
		return;
	}

	SetSimulationActive(false);
	HidePauseOverlay();
	SpawnCutsceneSkipPrompt();

	// Reset state
	CleanupCutsceneObjects();
	cutscene_.active = true;
	cutscene_.images = imagePaths;
	cutscene_.current = 0;
	cutscene_.holdTime = std::max(0.0f, holdSecondsPerImage);
	cutscene_.fadeTime = std::max(0.0f, fadeSeconds);
	cutscene_.t = 0.0f;
	cutscene_.phase = CutsceneState::Phase::FadeIn;
	cutscene_.targetLevelJson = levelJsonPath;
	cutscene_.targetActivateSim = activateSimulation;
	cutscene_.queuedFinalLoad = false;

	// Spawn the first sprite full-screen
	const glm::vec3 center{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f };
	const glm::vec2 fullSize{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) };

	if (GameObject* s = SpawnStaticSprite(cutscene_.images[0], center, fullSize, cutscene_.uiLayer)) {
		cutscene_.spriteA = s->GetID();
		SetSpriteAlpha(s, 0.0f);
	}
	else {
		cutscene_.active = false;
		DespawnCutsceneSkipPrompt();
		QueueLevelLoad(levelJsonPath, activateSimulation);
	}
}

/**
 * @brief Performs spawn cutscene skip prompt.
 * @return Result produced by this operation.
 */
void Scene::SpawnCutsceneSkipPrompt() {
	if (cutsceneSkipPromptId_ >= 0 && GetGameObjectByID(cutsceneSkipPromptId_)) {
		return;
	}

	const float x = static_cast<float>(GraphicsEngine::kRefW) - (kCutsceneSkipSize.x * 0.5f) - kCutsceneSkipMarginRight;
	const float y = (kCutsceneSkipSize.y * 0.5f) + kCutsceneSkipMarginTop;

	if (GameObject* prompt = SpawnStaticSprite(
		kCutsceneSkipTexture,
		glm::vec3(x, y, 0.0f),
		kCutsceneSkipSize,
		"999999")) {
		cutsceneSkipPromptId_ = prompt->GetID();
		SetObjectTexturePath(cutsceneSkipPromptId_, kCutsceneSkipTexture);
		prompt->SetRenderSortOrder(kCutsceneSkipSortOrder);
	}
}

/**
 * @brief Performs despawn cutscene skip prompt.
 * @return Result produced by this operation.
 */
void Scene::DespawnCutsceneSkipPrompt() {
	if (cutsceneSkipPromptId_ < 0) {
		return;
	}

	if (GetGameObjectByID(cutsceneSkipPromptId_)) {
		DespawnByID(cutsceneSkipPromptId_);
	}
	cutsceneSkipPromptId_ = -1;
}

/**
 * @brief Updates cutscene.
 * @param dt Frame delta time in seconds.
 * @return Result produced by this operation.
 */
void Scene::UpdateCutscene(float dt) {
	if (!cutscene_.active) return;

	// Helper to get object for id
	auto getObj = [&](int id) -> GameObject* { return GetGameObjectByID(id); };

	// Advance timers
	cutscene_.t += dt;

	switch (cutscene_.phase) {
	case CutsceneState::Phase::FadeIn:
	{
		// Fade in spriteA from 0 -> 1
		float alpha = (cutscene_.fadeTime > 0.0f) ? std::min(1.0f, cutscene_.t / cutscene_.fadeTime) : 1.0f;
		if (GameObject* a = getObj(cutscene_.spriteA)) {
			SetSpriteAlpha(a, alpha);
		}
		if (alpha >= 1.0f) {
			cutscene_.phase = CutsceneState::Phase::Hold;
			cutscene_.t = 0.0f;
		}
		break;
	}
	case CutsceneState::Phase::Hold:
	{
		if (cutscene_.t >= cutscene_.holdTime) {
			// Prepare next image if any
			if (cutscene_.current + 1 < cutscene_.images.size()) {
				// Spawn next spriteB on top (or cross-fade if supported)
				const glm::vec3 center{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f };
				const glm::vec2 fullSize{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) };
				if (GameObject* b = SpawnStaticSprite(cutscene_.images[cutscene_.current + 1], center, fullSize, cutscene_.uiLayer)) {
					cutscene_.spriteB = b->GetID();
					// Start with alpha=0 to fade in
					SetSpriteAlpha(b, 0.0f);
					cutscene_.phase = CutsceneState::Phase::FadeOut;
					cutscene_.t = 0.0f;
				}
				else {
					// Could not spawn next; jump to end
					cutscene_.current = static_cast<size_t>(cutscene_.images.size());
					cutscene_.phase = CutsceneState::Phase::FadeOut;
					cutscene_.t = 0.0f;
				}
			}
			else {
				// Last image finished holding -> end cutscene and load level
				CleanupCutsceneObjects();
				DespawnCutsceneSkipPrompt();
				cutscene_.active = false;
				if (!cutscene_.queuedFinalLoad) {
					cutscene_.queuedFinalLoad = true;
					QueueLevelLoad(cutscene_.targetLevelJson, cutscene_.targetActivateSim);
				}
			}
		}
		break;
	}
	case CutsceneState::Phase::FadeOut:
	{
		// Cross-fade: spriteA goes 1->0, spriteB goes 0->1
		float tNorm = (cutscene_.fadeTime > 0.0f) ? std::min(1.0f, cutscene_.t / cutscene_.fadeTime) : 1.0f;
		float alphaA = 1.0f - tNorm;
		float alphaB = tNorm;

		if (GameObject* a = getObj(cutscene_.spriteA)) SetSpriteAlpha(a, alphaA);
		if (GameObject* b = getObj(cutscene_.spriteB)) SetSpriteAlpha(b, alphaB);

		if (tNorm >= 1.0f) {
			// Despawn old A, promote B to A, advance index
			if (cutscene_.spriteA >= 0) DespawnByID(cutscene_.spriteA);
			cutscene_.spriteA = cutscene_.spriteB;
			cutscene_.spriteB = -1;
			cutscene_.current += 1;
			cutscene_.phase = CutsceneState::Phase::Hold;
			cutscene_.t = 0.0f;
		}
		break;
	}
	}
}

/**
 * @brief Performs cleanup cutscene objects.
 * @return Result produced by this operation.
 */
void Scene::CleanupCutsceneObjects() {
	if (cutscene_.spriteA >= 0) {
		DespawnByID(cutscene_.spriteA);
		cutscene_.spriteA = -1;
	}
	if (cutscene_.spriteB >= 0) {
		DespawnByID(cutscene_.spriteB);
		cutscene_.spriteB = -1;
	}
}

/**
 * @brief Sets sprite alpha.
 * @param obj Parameter for obj.
 * @param alpha Parameter for alpha.
 * @return Result produced by this operation.
 */
void Scene::SetSpriteAlpha(GameObject* obj, float alpha) {
	if (!obj) return;
	// Clamp and apply as RGBA tint
	float a = std::clamp(alpha, 0.0f, 1.0f);
	obj->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, a));
	// Keep full UV rect so texture is still visible
	obj->SetUVRect({ 0.f, 0.f, 1.f, 1.f });
}

/**
 * @brief Performs start cutscene transitioned.
 * @param imagePaths Parameter for image paths.
 * @param levelJsonPath Parameter for level json path.
 * @param activateSimulation Parameter for activate simulation.
 * @param fadeOutSeconds Parameter for fade out seconds.
 * @param fadeInSeconds Parameter for fade in seconds.
 * @param holdSeconds Parameter for hold seconds.
 * @param crossfadeFromIndex Parameter for crossfade from index.
 * @param crossfadeSeconds Parameter for crossfade seconds.
 * @return Result produced by this operation.
 */
void Scene::StartCutsceneTransitioned(const std::vector<std::string>& imagePaths,
	const std::string& levelJsonPath,
	bool activateSimulation,
	float fadeOutSeconds,
	float fadeInSeconds,
	float holdSeconds,
	int crossfadeFromIndex,
	float crossfadeSeconds) {
	if (imagePaths.empty()) {
		QueueLevelLoad(levelJsonPath, activateSimulation);
		return;
	}

	SetSimulationActive(false);
	HidePauseOverlay();
	SpawnCutsceneSkipPrompt();

	if (cutTrans_.currentSpriteId >= 0) DespawnByID(cutTrans_.currentSpriteId);
	cutTrans_ = {};
	cutTrans_.active = true;
	cutTrans_.images = imagePaths;
	cutTrans_.index = 0;
	cutTrans_.targetLevelJson = levelJsonPath;
	cutTrans_.targetActivateSim = activateSimulation;
	cutTrans_.outSeconds = fadeOutSeconds;
	cutTrans_.inSeconds = fadeInSeconds;
	cutTrans_.holdSeconds = std::max(0.0f, holdSeconds);
	cutTrans_.holdElapsed = 0.0f;
	cutTrans_.holding = false;
	cutTrans_.awaitingBlackout = false;
	cutTrans_.awaitingInitialFadeIn = false;

	// Crossfade configuration
	cutTrans_.useCrossfade = (crossfadeFromIndex >= 0);
	cutTrans_.crossfadeSeconds = std::max(0.05f, crossfadeSeconds);
	cutTrans_.crossfadeFromIndex = crossfadeFromIndex;

	// Note: do not spawn the first image yet.
	auto& gfx = GetGraphicsEngine();
	gfx.StartSceneTransition(cutTrans_.outSeconds, cutTrans_.inSeconds);
	cutTrans_.awaitingBlackout = true;
}

/**
 * @brief Performs start cutscene transitioned bounded.
 * @param imagePaths Parameter for image paths.
 * @param boundaryFlags Parameter for boundary flags.
 * @param levelJsonPath Parameter for level json path.
 * @param activateSimulation Parameter for activate simulation.
 * @param fadeOutSeconds Parameter for fade out seconds.
 * @param fadeInSeconds Parameter for fade in seconds.
 * @param holdSeconds Parameter for hold seconds.
 * @param crossfadeFromIndex Parameter for crossfade from index.
 * @param crossfadeSeconds Parameter for crossfade seconds.
 * @return Result produced by this operation.
 */
void Scene::StartCutsceneTransitionedBounded(const std::vector<std::string>& imagePaths,
	const std::vector<bool>& boundaryFlags,
	const std::string& levelJsonPath,
	bool activateSimulation,
	float fadeOutSeconds,
	float fadeInSeconds,
	float holdSeconds,
	int crossfadeFromIndex,
	float crossfadeSeconds) {
	// Store boundary flags in the static cache so UpdateCutsceneTransitioned can use them
	sCutsceneBoundaryFlags = boundaryFlags;

	// Delegate to the regular transitioned cutscene with the same parameters
	StartCutsceneTransitioned(imagePaths,
		levelJsonPath,
		activateSimulation,
		fadeOutSeconds,
		fadeInSeconds,
		holdSeconds,
		crossfadeFromIndex,
		crossfadeSeconds);
}

/**
 * @brief Updates cutscene transitioned.
 * @param dt Frame delta time in seconds.
 * @return Result produced by this operation.
 */
void Scene::UpdateCutsceneTransitioned(float dt) {
	if (!cutTrans_.active) return;

	auto* gfx = &GetGraphicsEngine();
	if (!gfx) return;

	// Crossfade 5 -> 6 
	if (cutTrans_.useCrossfade && cutTrans_.crossfading) {
		cutTrans_.crossfadeT += dt;
		float tNorm = std::min(1.0f, cutTrans_.crossfadeT / cutTrans_.crossfadeSeconds);

		if (GameObject* a = GetGameObjectByID(cutTrans_.currentSpriteId)) SetSpriteAlpha(a, 1.0f - tNorm);
		if (GameObject* b = GetGameObjectByID(cutTrans_.nextSpriteId))    SetSpriteAlpha(b, tNorm);

		if (tNorm >= 1.0f) {
			if (cutTrans_.currentSpriteId >= 0) DespawnByID(cutTrans_.currentSpriteId);
			cutTrans_.currentSpriteId = cutTrans_.nextSpriteId;
			cutTrans_.nextSpriteId = -1;
			cutTrans_.crossfading = false;
			cutTrans_.holding = true;
			cutTrans_.holdElapsed = 0.0f;
		}
		return;
	}

	// Hold timing for subsequent transitions
	if (cutTrans_.holding && !gfx->IsTransitionActive()) {
		cutTrans_.holdElapsed += dt;
		if (cutTrans_.holdElapsed >= cutTrans_.holdSeconds) {
			const size_t nextIndex = cutTrans_.index + 1;
			if (nextIndex < cutTrans_.images.size()) {
				const bool isBoundary = (nextIndex < sCutsceneBoundaryFlags.size())
					? sCutsceneBoundaryFlags[nextIndex]
					: true;

				// Boundary-aware: decide transition vs instantaneous swap
				if (isBoundary) {
					// Crossfade at specific boundary index
					if (cutTrans_.useCrossfade && static_cast<int>(nextIndex) == cutTrans_.crossfadeFromIndex) {

						// Disable layer 10 object visibility (this is never re-enabled) temp fix for now
						if (Layer* menuLayer = GetLayer("10")) {
							menuLayer->SetVisible(false);
							menuLayer->SetEnabled(false);
						}

						const glm::vec3 center{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f };
						const glm::vec2 full{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) };
						const glm::vec3 topCenter{ center.x, center.y, 0.001f };
						if (GameObject* b = SpawnStaticSprite(cutTrans_.images[nextIndex], topCenter, full, cutTrans_.uiLayer)) {
							cutTrans_.nextSpriteId = b->GetID();
							SetSpriteAlpha(b, 0.0f);
							cutTrans_.crossfading = true;
							cutTrans_.crossfadeT = 0.0f;
							cutTrans_.index = nextIndex;
							cutTrans_.holding = false;
						}
						else {
							gfx->StartSceneTransition(cutTrans_.outSeconds, cutTrans_.inSeconds);
							cutTrans_.awaitingBlackout = true;
							cutTrans_.holding = false;
							cutTrans_.holdElapsed = 0.0f;
						}
					}
					else {
						// Normal boundary: use fade-out/in
						gfx->StartSceneTransition(cutTrans_.outSeconds, cutTrans_.inSeconds);
						cutTrans_.awaitingBlackout = true;
						cutTrans_.holding = false;
						cutTrans_.holdElapsed = 0.0f;
					}
				}
				else {
					// Intra-chapter frame: instantaneous swap without transition
					// Spawn next, promote immediately, maintain holding for next frame delay
					if (cutTrans_.currentSpriteId >= 0) DespawnByID(cutTrans_.currentSpriteId);
					const glm::vec3 center{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f };
					const glm::vec2 full{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) };
					if (GameObject* s = SpawnStaticSprite(cutTrans_.images[nextIndex], center, full, cutTrans_.uiLayer)) {
						cutTrans_.currentSpriteId = s->GetID();
					}
					cutTrans_.index = nextIndex;
					cutTrans_.holdElapsed = 0.0f;
					cutTrans_.holding = true; // continue holding sequence for next frame
				}
			}
			else {
				// End: boundary or not, finish with fade and load level
				gfx->StartSceneTransition(cutTrans_.outSeconds, cutTrans_.inSeconds);
				cutTrans_.awaitingBlackout = true;
				cutTrans_.holding = false;

				// Fade out cutscene BGM as we transition to the level
#ifndef _DEBUG
				if (cutsceneFadeOutHook_) {
					cutsceneFadeOutHook_(*this, cutTrans_.outSeconds);
				}
#endif
			}
		}
	}

	// Blackout handoff: spawn first image or next image, then fade-in and hold
	if (cutTrans_.awaitingBlackout && gfx->IsAtBlackout()) {
		cutTrans_.awaitingBlackout = false;

		// First image case: index==0 and nothing spawned yet
		if (cutTrans_.currentSpriteId < 0 && cutTrans_.index == 0) {
			const glm::vec3 center{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f };
			const glm::vec2 full{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) };
			if (GameObject* s = SpawnStaticSprite(cutTrans_.images[0], center, full, cutTrans_.uiLayer)) {
				cutTrans_.currentSpriteId = s->GetID();
			}
			gfx->ContinueTransitionFadeIn();
			cutTrans_.holding = true;
			cutTrans_.holdElapsed = 0.0f;

			// Start win cutscene BGM after initial fade-in (when first image appears)
			// Check if this is the win cutscene by looking at the image paths
#ifndef _DEBUG
			if (cutsceneFirstFrameHook_ && !cutTrans_.images.empty()) {
				cutsceneFirstFrameHook_(*this, cutTrans_.images[0]);
			}
#endif
			return;
		}

		const size_t nextIndex = cutTrans_.index + 1;
		if (nextIndex < cutTrans_.images.size()) {
			if (cutTrans_.currentSpriteId >= 0) DespawnByID(cutTrans_.currentSpriteId);

			const glm::vec3 center{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f };
			const glm::vec2 full{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) };
			if (GameObject* s = SpawnStaticSprite(cutTrans_.images[nextIndex], center, full, cutTrans_.uiLayer)) {
				cutTrans_.currentSpriteId = s->GetID();
			}
			cutTrans_.index = nextIndex;

			gfx->ContinueTransitionFadeIn();
			cutTrans_.holding = true;
			cutTrans_.holdElapsed = 0.0f;
		}
		else {
			// Last blackout → load level, then fade-in after build (existing logic)
			if (cutTrans_.currentSpriteId >= 0) {
				DespawnByID(cutTrans_.currentSpriteId);
				cutTrans_.currentSpriteId = -1;
			}

			// Stop the cutscene BGM completely before loading the level
#ifndef _DEBUG
			if (cutsceneBeforeFinalLoadHook_) {
				cutsceneBeforeFinalLoadHook_(*this, cutTrans_.outSeconds);
			}
#endif
			DespawnCutsceneSkipPrompt();
			cutTrans_.active = false;
			QueueLevelLoad(cutTrans_.targetLevelJson, cutTrans_.targetActivateSim);
			cutTrans_.fadeInAfterLoad = true; // handled in Scene::Update after LoadAndBuild
		}
	}
}

/**
 * @brief Triggers order ui slide in.
 * @param targetPos Parameter for target pos.
 * @param size Parameter for size.
 * @param layer Parameter for layer.
 * @param texturePath Parameter for texture path.
 * @param duration Parameter for duration.
 * @return Result produced by this operation.
 */
int Scene::TriggerOrderUiSlideIn(const glm::vec2& targetPos,
	const glm::vec2& size,
	const std::string& layer,
	const std::string& texturePath,
	float duration) {
	// spawn off-screen (above)
	glm::vec2 startPos = { targetPos.x, -size.y * 0.5f };

	GameObject* ui = SpawnStaticSprite(texturePath.c_str(),
		{ startPos.x, startPos.y, 0.f },
		size,
		layer);
	if (!ui) return -1;

	const int id = ui->GetID();

	// whatever you already do to register the slide...
	// AddUiSlide(id, startPos, targetPos, duration);

	SetObjectTexturePath(id, texturePath);

	// REGISTER THE SLIDE
	UiSlide s;
	s.objectId = id;
	s.startPos = startPos;
	s.targetPos = targetPos;
	s.t = 0.0f;
	s.duration = std::max(0.001f, duration);
	s.active = true;
	uiSlides_.push_back(s);

	return id;
}

/**
 * @brief Updates ui slides.
 * @param dt Frame delta time in seconds.
 * @return Result produced by this operation.
 */
void Scene::UpdateUiSlides(float dt) {
	if (uiSlides_.empty()) return;

	// We allow slide updates even when simulation is paused,
	// so UI remains responsive.
	for (auto& s : uiSlides_) {
		if (!s.active) continue;

		s.t += dt;
		const float norm = std::clamp(s.t / s.duration, 0.0f, 1.0f);
		const float eased = EaseOutCubic(norm);

		const float x = s.startPos.x + (s.targetPos.x - s.startPos.x) * eased;
		const float y = s.startPos.y + (s.targetPos.y - s.startPos.y) * eased;

		if (GameObject* obj = GetGameObjectByID(s.objectId)) {
			glm::vec3 p = obj->GetPositionGLM();
			p.x = x;
			p.y = y;
			obj->SetPosition(p);
		}

		if (norm >= 1.0f) {
			s.active = false;
			// Snap to exact target
			if (GameObject* obj = GetGameObjectByID(s.objectId)) {
				glm::vec3 p = obj->GetPositionGLM();
				p.x = s.targetPos.x;
				p.y = s.targetPos.y;
				obj->SetPosition(p);
			}
		}
	}

	// Remove finished slides
	uiSlides_.erase(
		std::remove_if(uiSlides_.begin(), uiSlides_.end(),
			[](const UiSlide& s) { return !s.active; }),
		uiSlides_.end()
	);
}

/**
 * @brief Renders level text objects.
 * @return Result produced by this operation.
 */
void Scene::RenderLevelTextObjects() {
	const auto& objs = LEPANELFONTS::GetTextObjects();
	if (objs.empty()) return;

	const bool cutsceneActive = IsAnyCutsceneActive();
	if (cutsceneActive) return;

	const bool pauseActive = IsPauseOverlayActive();
	static const std::unordered_set<std::string> kHudTextNames = {
		"MoneyText", "QuotaText", "TimerText"
	};

	glm::mat4 projection = graphicsEngine.GetProjection();

	GLint prevViewport[4];
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	GLint boundFBO = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &boundFBO);

	if (boundFBO != 0) glViewport(0, 0, graphicsEngine.GetSceneWidth(), graphicsEngine.GetSceneHeight());
	else graphicsEngine.ApplyViewport();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	RenderFloatingWorldTextFx(projection, pauseActive, cutsceneActive);

	for (const auto& o : objs) {
		// Hide HUD text during cutscenes or pause overlay
		if ((cutsceneActive || pauseActive) && kHudTextNames.count(o.name)) continue;

		if (o.text.empty()) continue;
		if (o.fontName.empty()) continue;
		if (o.colorA <= 0.001f) continue;

		// (your existing layer visibility check is fine)
		if (!o.layer.empty()) {
			Layer* layer = GetLayer(o.layer);
			if (layer && (!layer->IsEnabled() || !layer->IsVisible()))
				continue;
		}

		FontSystem::Font* font = ResourceManager::Instance().GetFont(o.fontName);
		if (!font) continue;

		FontSystem::Text t;
		t.SetFont(font);
		t.SetText(o.text);
		t.SetColor(glm::vec4(o.colorR, o.colorG, o.colorB, o.colorA));
		t.SetScale(o.scale);
		t.SetRotation(o.rotation);
		t.SetPosition(glm::vec2(o.x, o.y));
		t.SetRotationMode(o.useBlockRotation
			? FontSystem::Text::RotationMode::Block
			: FontSystem::Text::RotationMode::PerCharacter);

		FontSystem::TextRenderer::Instance().RenderText(t, projection);
	}

	glDisable(GL_BLEND);
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}

/**
 * @brief Performs start level transition.
 * @param levelJsonPath Parameter for level json path.
 * @param activateSimulation Parameter for activate simulation.
 * @param fadeOutSeconds Parameter for fade out seconds.
 * @param fadeInSeconds Parameter for fade in seconds.
 * @return Result produced by this operation.
 */
void Scene::StartLevelTransition(const std::string& levelJsonPath,
	bool activateSimulation,
	float fadeOutSeconds,
	float fadeInSeconds) {
	// Avoid double-triggering
	if (levelTrans_.active) return;

#ifndef _DEBUG
	// Freeze gameplay immediately
	SetSimulationActive(false);
#endif
	HidePauseOverlay();

	levelTrans_.active = true;
	levelTrans_.awaitingBlackout = true;
	levelTrans_.targetJson = levelJsonPath;
	levelTrans_.targetActivateSim = activateSimulation;
	levelTrans_.outSec = fadeOutSeconds;
	levelTrans_.inSec = fadeInSeconds;

	// Reuse existing "fade in after load" behavior that you already have:
	// Scene::Update checks cutTrans_.fadeInAfterLoad and uses cutTrans_.inSeconds.
	cutTrans_.inSeconds = fadeInSeconds;

	auto& gfx = GetGraphicsEngine();
	gfx.StartSceneTransition(levelTrans_.outSec, levelTrans_.inSec);
}

/**
 * @brief Updates level transition.
 * @return Result produced by this operation.
 */
void Scene::UpdateLevelTransition() {
	if (!levelTrans_.active) return;

	auto& gfx = GetGraphicsEngine();

	// Once we're fully black, queue the load.
	if (levelTrans_.awaitingBlackout && gfx.IsAtBlackout()) {
		levelTrans_.awaitingBlackout = false;

		QueueLevelLoad(levelTrans_.targetJson, levelTrans_.targetActivateSim);

		// This makes your existing code fade-in right after LoadAndBuild succeeds
		cutTrans_.fadeInAfterLoad = true;

		levelTrans_.active = false;
	}
}

/**
 * @brief Performs skip active cutscene.
 * @return Result produced by this operation.
 */
void Scene::SkipActiveCutscene() {
	if (cutTrans_.active) {
		auto& gfx = GetGraphicsEngine();
		const float skipFadeOutSeconds = cutTrans_.outSeconds * 2.0f;

		// Force restart so skip timing applies even if a transition is already active.
		if (gfx.IsTransitionActive()) {
			gfx.CancelSceneTransition();
		}
		gfx.StartSceneTransition(skipFadeOutSeconds, cutTrans_.inSeconds);
		cutTrans_.outSeconds = skipFadeOutSeconds;

		cutTrans_.awaitingBlackout = true;
		cutTrans_.holding = false;
		cutTrans_.crossfading = false;
		cutTrans_.holdElapsed = 0.0f;

		if (!cutTrans_.images.empty()) {
			cutTrans_.index = cutTrans_.images.size() - 1;
		}

#ifndef _DEBUG
		if (skipCutsceneAudioHook_) {
			skipCutsceneAudioHook_(*this, skipFadeOutSeconds);
		}
#endif
	}

	if (cutscene_.active) {
		CleanupCutsceneObjects();
		DespawnCutsceneSkipPrompt();
		cutscene_.active = false;

		if (!cutscene_.queuedFinalLoad) {
			cutscene_.queuedFinalLoad = true;
			QueueLevelLoad(cutscene_.targetLevelJson, cutscene_.targetActivateSim);
		}
	}
}
