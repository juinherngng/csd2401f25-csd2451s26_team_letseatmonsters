/*
----------------------------------------------------------------------------------------------------
 FILE NAME:          MyoonchiDinerBindings.cpp
 PROJECT NAME:       Project GAM200
 AUTHOR:             Ng Juin Herng, juinherng.ng@digipen.edu (60%)
 CO-AUTHOR:          Seah Wang Hua, wanghua.seah@digipen.edu (40%)

 DESCRIPTION:
	Central game-integration layer for Myoonchi Diner.

	This file registers and implements Scene hooks that connect engine systems
	to game-specific behavior. In practice, this file is the "policy + glue"
	layer between data-driven level content and runtime game logic.

	Responsibilities:
	  1) Object bootstrap from level JSON
		 - Tag -> logic binding (table dispatch)
		 - Role IDs and authored velocity application
		 - Runtime animation attachment rules

	  2) Global simulation policy
		 - Economy update and time-based SFX warnings
		 - Level-specific customer/economy tuning

	  3) Scene lifecycle orchestration
		 - Default scene setup
		 - Post-load menu/audio setup
		 - Cutscene audio transitions and cleanup

	  4) Tutorial runtime controller
		 - Step-gated tutorial progression
		 - Context-sensitive world highlights
		 - Completion popup handling and return-to-menu flow

	  5) Pause/navigation integration
		 - Pause overlay button action binding
		 - Navigation blocker extraction for pathfinding

	Design notes:
	  - Anonymous-namespace helpers are internal and file-local.
	  - Hook callbacks are intentionally lightweight wrappers that delegate
		detailed behavior to focused helper functions.
	  - Tutorial flow is updated from the customer update hook so popup input
		remains responsive even when simulation is paused/disabled.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "EngineCore/AudioManager.hpp"
#include "EngineCore/EngineRng.hpp"
#include "EngineCore/FilePaths.hpp"
#include "EngineCore/InputManager.hpp"
#include "EngineCore/LevelEditorPanelFonts.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/CustomerManagerLogic.hpp"
#include "GameCore/CustomerTableLogic.hpp"
#include "GameCore/ExitGateLogic.hpp"
#include "GameCore/HowToPlayButtonLogic.hpp"
#include "GameCore/InGamePauseTriggerLogic.hpp"
#include "GameCore/IngredientBoxLogic.hpp"
#include "GameCore/IngredientLogic.hpp"
#include "GameCore/MenuKeyboardNavigation.hpp"
#include "GameCore/MenuButtonLogic.hpp"
#include "GameCore/OrderUILogic.hpp"
#include "GameCore/PauseButtonLogic.hpp"
#include "GameCore/PlateLogic.hpp"
#include "GameCore/PlayerLogic.hpp"
#include "GameCore/Quota.hpp"
#include "GameCore/SettingsMenuLogic.hpp"
#include "GameCore/SimpleNpcLogic.hpp"
#include "GameCore/StartGamePromptLogic.hpp"
#include "GameCore/TableLogic.hpp"
#include "GameCore/TrashCanLogic.hpp"
#include "GameCore/WorkTableLogic.hpp"
#include "MyoonchiDiner/GamePaths.hpp"
#include "MyoonchiDiner/MyoonchiDinerBindings.hpp"

namespace {
	constexpr int kAmbientVfxRows = 14;
	constexpr int kAmbientVfxCols = 6;

	constexpr const char* kShinePointTag = "bg_vfx_shine_point";
	constexpr const char* kCandlePointTag = "bg_vfx_candle_point";
	constexpr const char* kButterflyPointTag = "bg_vfx_butterfly_point";
	constexpr const char* kButterflyAltPointTag = "bg_vfx_butterfly_alt_point";
	constexpr const char* kLeafLaneTag = "bg_vfx_leaf_lane";

	static std::vector<glm::vec4> CreateVfxFramesFromTopRow(int topRowOneBased, int startCol, int endCol) {
		std::vector<glm::vec4> frames;

		const int engineRow = kAmbientVfxRows - topRowOneBased; // row 0 = bottom
		const float frameW = 1.0f / static_cast<float>(kAmbientVfxCols);
		const float frameH = 1.0f / static_cast<float>(kAmbientVfxRows);
		const float v = static_cast<float>(engineRow) * frameH;

		for (int col = startCol; col <= endCol; ++col) {
			const float u = static_cast<float>(col) * frameW;
			frames.emplace_back(u, v, frameW, frameH);
		}

		return frames;
	}

	static std::vector<glm::vec4> CreateVfxFramesFromBottomRow(int bottomRowOneBased, int startCol, int endCol) {
		return CreateVfxFramesFromTopRow(kAmbientVfxRows - bottomRowOneBased + 1, startCol, endCol);
	}

	static float RandomRange(float minValue, float maxValue) {
		std::uniform_real_distribution<float> dist(minValue, maxValue);
		return dist(EngineRng::Get());
	}

	static int RandomIndex(int maxExclusive) {
		std::uniform_int_distribution<int> dist(0, maxExclusive - 1);
		return dist(EngineRng::Get());
	}

	struct AmbientPoint {
		glm::vec2 pos{ 0.0f, 0.0f };
		glm::vec2 size{ 96.0f, 96.0f };
		std::string layer{ "0" };
	};

	struct AmbientVfxInstance {
		int objectID = -1;
		glm::vec2 velocity{ 0.0f, 0.0f };
		float lifetime = 0.0f;
		float totalLifetime = 0.0f;
		float shrinkWindowRatio = 0.30f;
		float shrinkDuration = -1.0f;
		glm::vec3 baseScale{ 1.0f, 1.0f, 1.0f };
		bool killWhenOffscreen = false;
		bool persistent = false;
		bool shrinkOutAtEnd = false;
	};

	struct AmbientVfxController {
		std::vector<AmbientPoint> shinePoints_;
		std::vector<AmbientPoint> candlePoints_;
		std::vector<AmbientPoint> butterflyPoints_;
		std::vector<AmbientPoint> butterflyAltPoints_;
		std::vector<float> leafLaneYs_;
		std::vector<AmbientVfxInstance> active_;
		bool cached_ = false;
		bool candlesSpawned_ = false;

		float shineCooldown_ = 0.0f;
		float butterflyCooldown_ = 0.0f;
		float butterflyAltCooldown_ = 0.0f;
		float leafCooldown_ = 0.0f;

		void Reset(Scene& scene) {
			for (auto& inst : active_) {
				if (inst.objectID >= 0 && scene.GetGameObjectByID(inst.objectID)) {
					scene.RequestDespawn(inst.objectID);
				}
			}

			active_.clear();
			shinePoints_.clear();
			candlePoints_.clear();
			butterflyPoints_.clear();
			butterflyAltPoints_.clear();
			leafLaneYs_.clear();
			cached_ = false;
			candlesSpawned_ = false;

			shineCooldown_ = 0.0f;
			butterflyCooldown_ = 0.0f;
			butterflyAltCooldown_ = 0.0f;
			leafCooldown_ = 0.0f;
		}

		void CachePoints(Scene& scene) {
			shinePoints_.clear();
			candlePoints_.clear();
			butterflyPoints_.clear();
			butterflyAltPoints_.clear();
			leafLaneYs_.clear();

			for (GameObject* obj : scene.GetAllObjectsRaw()) {
				if (!obj) continue;

				const int id = obj->GetID();
				const Scene::Defaults d = scene.GetDefaults(id);

				if (d.tag == kShinePointTag) {
					AmbientPoint p;
					const glm::vec3 pos = obj->GetPositionGLM();
					p.pos = glm::vec2(pos.x, pos.y);
					p.size = (d.size.x > 0.0f && d.size.y > 0.0f) ? d.size : glm::vec2(96.0f, 96.0f);
					p.layer = d.layer.empty() ? "0" : d.layer;
					shinePoints_.push_back(p);
				}
				else if (d.tag == kCandlePointTag) {
					AmbientPoint p;
					const glm::vec3 pos = obj->GetPositionGLM();
					p.pos = glm::vec2(pos.x, pos.y);
					p.size = (d.size.x > 0.0f && d.size.y > 0.0f) ? d.size : glm::vec2(56.0f, 56.0f);
					p.layer = d.layer.empty() ? "1" : d.layer;
					candlePoints_.push_back(p);
				}
				else if (d.tag == kButterflyPointTag) {
					AmbientPoint p;
					const glm::vec3 pos = obj->GetPositionGLM();
					p.pos = glm::vec2(pos.x, pos.y);
					p.size = (d.size.x > 0.0f && d.size.y > 0.0f) ? d.size : glm::vec2(160.0f, 160.0f);
					p.layer = d.layer.empty() ? "0" : d.layer;
					butterflyPoints_.push_back(p);
				}
				else if (d.tag == kButterflyAltPointTag) {
					AmbientPoint p;
					const glm::vec3 pos = obj->GetPositionGLM();
					p.pos = glm::vec2(pos.x, pos.y);
					p.size = (d.size.x > 0.0f && d.size.y > 0.0f) ? d.size : glm::vec2(160.0f, 160.0f);
					p.layer = d.layer.empty() ? "0" : d.layer;
					butterflyAltPoints_.push_back(p);
				}
				else if (d.tag == kLeafLaneTag) {
					const glm::vec3 pos = obj->GetPositionGLM();
					leafLaneYs_.push_back(pos.y);
				}
			}

			cached_ = true;
		}

		void UpdateInstances(float dt, Scene& scene) {
			for (auto& inst : active_) {
				if (inst.objectID < 0) {
					continue;
				}

				GameObject* obj = scene.GetGameObjectByID(inst.objectID);
				if (!obj) {
					inst.objectID = -1;
					continue;
				}

				if (inst.velocity.x != 0.0f || inst.velocity.y != 0.0f) {
					glm::vec3 pos = obj->GetPositionGLM();
					pos.x += inst.velocity.x * dt;
					pos.y += inst.velocity.y * dt;
					obj->SetPosition(pos);
				}

				if (!inst.persistent) {
					inst.lifetime -= dt;
				}

				if (!inst.persistent && inst.shrinkOutAtEnd && inst.totalLifetime > 0.0f) {
					const float shrinkStartLife = (inst.shrinkDuration > 0.0f)
						? std::min(inst.shrinkDuration, inst.totalLifetime)
						: inst.totalLifetime * std::clamp(inst.shrinkWindowRatio, 0.05f, 0.95f);
					float scaleMul = 1.0f;
					if (inst.lifetime < shrinkStartLife) {
						scaleMul = std::clamp(inst.lifetime / shrinkStartLife, 0.0f, 1.0f);
					}

					obj->SetScale(glm::vec3(
						inst.baseScale.x * scaleMul,
						inst.baseScale.y * scaleMul,
						inst.baseScale.z
					));
				}

				bool shouldKill = (!inst.persistent && inst.lifetime <= 0.0f);

				if (!shouldKill && inst.killWhenOffscreen) {
					const glm::vec3 pos = obj->GetPositionGLM();
					if (pos.x < -220.0f || pos.x > static_cast<float>(GraphicsEngine::kRefW) + 220.0f) {
						shouldKill = true;
					}
				}

				if (shouldKill) {
					scene.RequestDespawn(inst.objectID);
					inst.objectID = -1;
				}
			}

			active_.erase(
				std::remove_if(active_.begin(), active_.end(),
					[](const AmbientVfxInstance& inst) {
						return inst.objectID < 0;
					}),
				active_.end()
			);
		}

		void SpawnShine(Scene& scene) {
			if (shinePoints_.empty()) {
				return;
			}

			const AmbientPoint& point = shinePoints_[RandomIndex(static_cast<int>(shinePoints_.size()))];

			// Shine = row 3 from top on the 14x6 sheet, 3 frames: 0..2
			const std::vector<glm::vec4> frames = CreateVfxFramesFromTopRow(3, 0, 2);
			const float frameDuration = 0.15f;

			GameObject* fx = scene.SpawnAnimatedSprite(
				MyoonchiPaths::Textures::AMBIENT_VFX_SHEET,
				glm::vec3(point.pos.x, point.pos.y, 0.0f),
				point.size,
				frames,
				frameDuration,
				false,
				point.layer
			);

			if (!fx) {
				return;
			}

			fx->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
			fx->SetColliderOffset(Math::Vector2D(0.0f, 0.0f));
			fx->SetMovableByPhysics(false);
			fx->EnableShadow(false);
			fx->SetRenderSortOrder(0);

			scene.SetObjectTag(fx->GetID(), "ambient_vfx_shine");

			AmbientVfxInstance inst;
			inst.objectID = fx->GetID();
			inst.velocity = glm::vec2(0.0f, 0.0f);
			inst.lifetime = static_cast<float>(frames.size()) * frameDuration + 0.05f;
			inst.totalLifetime = inst.lifetime;
			inst.baseScale = glm::vec3(point.size.x, point.size.y, 1.0f);
			inst.killWhenOffscreen = false;
			active_.push_back(inst);
		}

		void SpawnCandles(Scene& scene) {
			if (candlesSpawned_ || candlePoints_.empty()) {
				return;
			}

			const std::vector<glm::vec4> frames = CreateVfxFramesFromTopRow(14, 0, 3);
			const float frameDuration = 0.14f;

			for (const AmbientPoint& point : candlePoints_) {
				GameObject* fx = scene.SpawnAnimatedSprite(
					MyoonchiPaths::Textures::AMBIENT_VFX_SHEET,
					glm::vec3(point.pos.x, point.pos.y, 0.0f),
					point.size,
					frames,
					frameDuration,
					true,
					point.layer
				);

				if (!fx) {
					continue;
				}

				fx->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
				fx->SetColliderOffset(Math::Vector2D(0.0f, 0.0f));
				fx->SetMovableByPhysics(false);
				fx->EnableShadow(false);
				// Keep candles above the table hover mask so level 1 hover feedback
				// never visually cuts through the permanent candle VFX.
				fx->SetRenderSortOrder(205);

				scene.SetObjectTag(fx->GetID(), "ambient_vfx_candle");

				AmbientVfxInstance inst;
				inst.objectID = fx->GetID();
				inst.velocity = glm::vec2(0.0f, 0.0f);
				inst.totalLifetime = 0.0f;
				inst.baseScale = glm::vec3(point.size.x, point.size.y, 1.0f);
				inst.killWhenOffscreen = false;
				inst.persistent = true;
				active_.push_back(inst);
			}

			candlesSpawned_ = true;
		}

		void SpawnLeaf(Scene& scene) {
			const bool leftToRight = RandomRange(0.0f, 1.0f) < 0.5f;
			// Let both sides use either authored leaf variant, but when a leaf enters
			// from the right we reverse the frame order so the motion reads more naturally.
			const int topRowOneBased = (RandomRange(0.0f, 1.0f) < 0.5f) ? 6 : 7;
			std::vector<glm::vec4> frames = CreateVfxFramesFromTopRow(topRowOneBased, 0, 5);
			if (!leftToRight) {
				std::reverse(frames.begin(), frames.end());
			}

			const float y = !leafLaneYs_.empty()
				? leafLaneYs_[RandomIndex(static_cast<int>(leafLaneYs_.size()))]
				: RandomRange(180.0f, 700.0f);

			const float speed = RandomRange(80.0f, 130.0f);
			const glm::vec2 velocity = leftToRight
				? glm::vec2(speed, 0.0f)
				: glm::vec2(-speed, 0.0f);

			const float startX = leftToRight
				? -140.0f
				: static_cast<float>(GraphicsEngine::kRefW) + 140.0f;

			const glm::vec2 size(128.0f, 128.0f);
			GameObject* fx = scene.SpawnAnimatedSprite(
				MyoonchiPaths::Textures::AMBIENT_VFX_SHEET,
				glm::vec3(startX, y, 0.0f),
				size,
				frames,
				0.08f,
				true,
				"10"
			);

			if (!fx) {
				return;
			}

			fx->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
			fx->SetColliderOffset(Math::Vector2D(0.0f, 0.0f));
			fx->SetMovableByPhysics(false);
			fx->EnableShadow(false);
			fx->SetRenderSortOrder(10);

			scene.SetObjectTag(fx->GetID(), "ambient_vfx_leaf");

			AmbientVfxInstance inst;
			inst.objectID = fx->GetID();
			inst.velocity = velocity;
			inst.lifetime = (static_cast<float>(GraphicsEngine::kRefW) + 320.0f) / speed + 0.5f;
			inst.totalLifetime = inst.lifetime;
			inst.baseScale = glm::vec3(size.x, size.y, 1.0f);
			inst.killWhenOffscreen = true;
			active_.push_back(inst);
		}

		void SpawnButterfly(Scene& scene, const AmbientPoint& point) {
			std::vector<glm::vec4> frames = CreateVfxFramesFromTopRow(8, 0, 5);
			const std::vector<glm::vec4> row9 = CreateVfxFramesFromTopRow(9, 0, 5);
			const std::vector<glm::vec4> row10 = CreateVfxFramesFromTopRow(10, 0, 5);
			const std::vector<glm::vec4> row11 = CreateVfxFramesFromTopRow(11, 0, 3);
			frames.insert(frames.end(), row9.begin(), row9.end());
			frames.insert(frames.end(), row10.begin(), row10.end());
			frames.insert(frames.end(), row11.begin(), row11.end());

			if (frames.empty()) {
				return;
			}

			const float frameDuration = 0.09f;
			GameObject* fx = scene.SpawnAnimatedSprite(
				MyoonchiPaths::Textures::AMBIENT_VFX_SHEET,
				glm::vec3(point.pos.x, point.pos.y, 0.0f),
				point.size,
				frames,
				frameDuration,
				false,
				point.layer
			);

			if (!fx) {
				return;
			}

			fx->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
			fx->SetColliderOffset(Math::Vector2D(0.0f, 0.0f));
			fx->SetMovableByPhysics(false);
			fx->EnableShadow(false);
			fx->SetRenderSortOrder(1);

			scene.SetObjectTag(fx->GetID(), "ambient_vfx_butterfly");

			AmbientVfxInstance inst;
			inst.objectID = fx->GetID();
			inst.velocity = glm::vec2(0.0f, 0.0f);
			inst.lifetime = static_cast<float>(frames.size()) * frameDuration + 0.05f;
			inst.totalLifetime = inst.lifetime;
			inst.baseScale = glm::vec3(point.size.x, point.size.y, 1.0f);
			inst.killWhenOffscreen = false;
			inst.shrinkOutAtEnd = true;
			active_.push_back(inst);
		}

		void SpawnButterflyAlt(Scene& scene, const AmbientPoint& point) {
			// Alternate butterfly = rows 3 then 2 when counted from bottom to top.
			std::vector<glm::vec4> frames = CreateVfxFramesFromBottomRow(3, 0, 5);
			const std::vector<glm::vec4> lastRow = CreateVfxFramesFromBottomRow(2, 0, 1);
			frames.insert(frames.end(), lastRow.begin(), lastRow.end());

			if (frames.empty()) {
				return;
			}

			const float frameDuration = 0.09f;
			GameObject* fx = scene.SpawnAnimatedSprite(
				MyoonchiPaths::Textures::AMBIENT_VFX_SHEET,
				glm::vec3(point.pos.x, point.pos.y, 0.0f),
				point.size,
				frames,
				frameDuration,
				false,
				point.layer
			);

			if (!fx) {
				return;
			}

			fx->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
			fx->SetColliderOffset(Math::Vector2D(0.0f, 0.0f));
			fx->SetMovableByPhysics(false);
			fx->EnableShadow(false);
			fx->SetRenderSortOrder(1);

			scene.SetObjectTag(fx->GetID(), "ambient_vfx_butterfly_alt");

			AmbientVfxInstance inst;
			inst.objectID = fx->GetID();
			inst.velocity = glm::vec2(0.0f, 0.0f);
			inst.lifetime = static_cast<float>(frames.size()) * frameDuration + 0.06f;
			inst.totalLifetime = inst.lifetime;
			inst.shrinkDuration = 0.06f;
			inst.baseScale = glm::vec3(point.size.x, point.size.y, 1.0f);
			inst.killWhenOffscreen = false;
			inst.shrinkOutAtEnd = true;
			active_.push_back(inst);
		}

		void Update(float dt, Scene& scene) {
			if (!scene.IsSimulationActive()) {
				return;
			}

			if (!cached_) {
				CachePoints(scene);
			}

			UpdateInstances(dt, scene);

			const std::string levelPath = scene.GetCurrentLevelPath();
			const bool isLevel1 = levelPath.find("kitchen01") != std::string::npos;
			const bool isLevel2 = levelPath.find("kitchen02") != std::string::npos;
			const bool isTutorial = levelPath.find("tutorial") != std::string::npos;

			if (isLevel1 || isTutorial) {
				SpawnCandles(scene);
			}

			if (isLevel1 || isTutorial) {
				shineCooldown_ -= dt;
				if (shineCooldown_ <= 0.0f && !shinePoints_.empty()) {
					SpawnShine(scene);
					shineCooldown_ = RandomRange(7.0f, 10.0f);
				}
			}
			else if (isLevel2) {
				butterflyCooldown_ -= dt;
				if (butterflyCooldown_ <= 0.0f && !butterflyPoints_.empty()) {
					SpawnButterfly(scene, butterflyPoints_[RandomIndex(static_cast<int>(butterflyPoints_.size()))]);
					butterflyCooldown_ = RandomRange(9.0f, 13.0f);
				}

				butterflyAltCooldown_ -= dt;
				if (butterflyAltCooldown_ <= 0.0f && !butterflyAltPoints_.empty()) {
					SpawnButterflyAlt(scene, butterflyAltPoints_[RandomIndex(static_cast<int>(butterflyAltPoints_.size()))]);
					butterflyAltCooldown_ = RandomRange(9.0f, 13.0f);
				}

				leafCooldown_ -= dt;
				if (leafCooldown_ <= 0.0f) {
					SpawnLeaf(scene);
					leafCooldown_ = RandomRange(7.0f, 10.0f);
				}
			}
		}
	};

	enum class QuitPopupYesAction {
		QuitApplication,
		ReturnToMainMenu
	};

	struct QuitPopupState {
		bool shown_ = false;
		std::vector<int> objectIDs_{};

		int yesButtonID_ = -1;
		int noButtonID_ = -1;

		bool mouseHeld_ = false;
		bool yesHovered_ = false;
		bool noHovered_ = false;
		QuitPopupYesAction yesAction_ = QuitPopupYesAction::QuitApplication;

		static constexpr const char* kQuitPopupTexture_ = "../assets/UI/quit_popup.png";
		static constexpr const char* kReturnPopupTexture_ = "../assets/UI/return_popup.png";

		static constexpr const char* kYesTexture_ = "../assets/UI/yes_s.png";
		static constexpr const char* kYesHoverTexture_ = "../assets/UI/yes_h.png";

		static constexpr const char* kNoTexture_ = "../assets/UI/no_s.png";
		static constexpr const char* kNoHoverTexture_ = "../assets/UI/no_h.png";

		// Match tutorial popup layering behavior, but force highest sort order too.
		static constexpr const char* kUiLayer_ = "999999";
		static constexpr int kPopupSortOrder_ = 1000000;
		static constexpr int kButtonSortOrder_ = 1000001;

		static bool GetMouseWorld(InputManager& input, glm::vec2& outWorld) {
			if (GraphicsEngine::Instance().GetMouseWorldInScene(outWorld)) {
				return true;
			}
			const glm::vec3 w = input.ScreenToWorld(
				static_cast<float>(input.GetMousePosition().x),
				static_cast<float>(input.GetMousePosition().y));
			outWorld = glm::vec2(w.x, w.y);
			return true;
		}

		static bool IsPointInObject(Scene& scene, int objectID, const glm::vec2& p) {
			GameObject* obj = scene.GetGameObjectByID(objectID);
			if (!obj) return false;

			const glm::vec3 pos = obj->GetPositionGLM();
			const glm::vec3 sz = obj->GetScaleGLM();
			const glm::vec2 min(pos.x - sz.x * 0.5f, pos.y - sz.y * 0.5f);
			const glm::vec2 max(pos.x + sz.x * 0.5f, pos.y + sz.y * 0.5f);

			return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
		}

		static void TrySetObjectTexture(Scene& scene, int objectID, const char* texturePath, const std::string& cacheKey) {
			if (objectID < 0 || !texturePath) {
				return;
			}

			GameObject* obj = scene.GetGameObjectByID(objectID);
			if (!obj) {
				return;
			}

			if (Texture* tex = ResourceManager::Instance().LoadTexture(cacheKey, texturePath)) {
				obj->SetTexture(tex);
				scene.SetObjectTexturePath(objectID, texturePath);
			}
		}

		void Clear(Scene& scene) {
			for (int id : objectIDs_) {
				if (id >= 0 && scene.GetGameObjectByID(id)) {
					// Defer destruction to avoid in-frame invalidation while UI logic is still updated
					scene.RequestDespawn(id);
				}
			}

			objectIDs_.clear();
			yesButtonID_ = -1;
			noButtonID_ = -1;

			shown_ = false;
			mouseHeld_ = false;
			yesHovered_ = false;
			noHovered_ = false;
			scene.SetMenuModalActive(false);
			MenuKeyboardNavigation::ClearFocus(MenuKeyboardNavigation::BuildScopeKey(scene, "quit_popup"));
		}

		void Show(Scene& scene, QuitPopupYesAction yesAction) {
			if (shown_) {
				return;
			}
			ResourceManager::Instance().LoadTexture(
				"animatedsprite_../assets/VFX/staranim-Sheet2.png",
				"../assets/VFX/staranim-Sheet2.png");
			shown_ = true;
			yesAction_ = yesAction;
			scene.SetMenuModalActive(true);

			const glm::vec3 center{
				static_cast<float>(GraphicsEngine::kRefW) * 0.5f,
				static_cast<float>(GraphicsEngine::kRefH) * 0.5f,
				0.0f
			};

			const char* popupTexture = (yesAction_ == QuitPopupYesAction::ReturnToMainMenu)
				? kReturnPopupTexture_
				: kQuitPopupTexture_;

			if (GameObject* popup = scene.SpawnStaticSprite(
				popupTexture,
				center,
				glm::vec2(1152.0f, 648.0f),
				kUiLayer_)) {
				popup->SetRenderSortOrder(kPopupSortOrder_);
				objectIDs_.push_back(popup->GetID());
			}

			const glm::vec2 buttonSize(280.0f, 80.0f);

			if (GameObject* yes = scene.SpawnStaticSprite(
				kYesTexture_,
				glm::vec3(center.x, center.y + 120.0f, 0.0f),
				buttonSize,
				kUiLayer_)) {
				yes->SetRenderSortOrder(kButtonSortOrder_);
				yesButtonID_ = yes->GetID();
				objectIDs_.push_back(yesButtonID_);
				scene.SetObjectTexturePath(yesButtonID_, kYesTexture_);
			}

			if (GameObject* no = scene.SpawnStaticSprite(
				kNoTexture_,
				glm::vec3(center.x, center.y + 210.0f, 0.0f),
				buttonSize,
				kUiLayer_)) {
				no->SetRenderSortOrder(kButtonSortOrder_);
				noButtonID_ = no->GetID();
				objectIDs_.push_back(noButtonID_);
				scene.SetObjectTexturePath(noButtonID_, kNoTexture_);
			}

			yesHovered_ = false;
			noHovered_ = false;
			MenuKeyboardNavigation::ClearFocus(MenuKeyboardNavigation::BuildScopeKey(scene, "quit_popup"));
		}

		void UpdateHoverVisuals(Scene& scene, bool yesHot, bool noHot) {
			if (yesButtonID_ >= 0) {
				if (yesHot != yesHovered_) {
					yesHovered_ = yesHot;
					if (yesHovered_) {
						scene.TriggerUiButtonHoverFeedback(yesButtonID_);
						if (AudioManager* audioManager = scene.GetAudioManager()) {
							if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
								audioManager->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager->GetVfxVolume(), false);
							}
						}
					}
					TrySetObjectTexture(
						scene,
						yesButtonID_,
						yesHovered_ ? kYesHoverTexture_ : kYesTexture_,
						yesHovered_ ? "quit_popup_yes_h" : "quit_popup_yes");
				}
			}

			if (noButtonID_ >= 0) {
				if (noHot != noHovered_) {
					noHovered_ = noHot;
					if (noHovered_) {
						scene.TriggerUiButtonHoverFeedback(noButtonID_);
						if (AudioManager* audioManager = scene.GetAudioManager()) {
							if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
								audioManager->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager->GetVfxVolume(), false);
							}
						}
					}
					TrySetObjectTexture(
						scene,
						noButtonID_,
						noHovered_ ? kNoHoverTexture_ : kNoTexture_,
						noHovered_ ? "quit_popup_no_h" : "quit_popup_no");
				}
			}
		}

		void Update(Scene& scene, InputManager& input) {
			if (!shown_) {
				return;
			}

			if (input.IsKeyJustPressed(GLFW_KEY_ESCAPE)) {
				if (AudioManager* audioManager = scene.GetAudioManager()) {
					if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_UI_BACK)) {
						audioManager->PlaySound(MyoonchiPaths::Audio::SFX_UI_BACK, audioManager->GetVfxVolume(), false);
					}
				}
				Clear(scene);
				input.ConsumeNextKeyPress(GLFW_KEY_ESCAPE);
				return;
			}

			GLFWwindow* window = glfwGetCurrentContext();
			if (!window) {
				return;
			}

			glm::vec2 mouseWorld{};
			GetMouseWorld(input, mouseWorld);
			const bool yesOver = yesButtonID_ >= 0 && IsPointInObject(scene, yesButtonID_, mouseWorld);
			const bool noOver = noButtonID_ >= 0 && IsPointInObject(scene, noButtonID_, mouseWorld);
			const int focusedButtonId = MenuKeyboardNavigation::UpdateFocus(
				scene,
				input,
				MenuKeyboardNavigation::BuildScopeKey(scene, "quit_popup"),
				{ yesButtonID_, noButtonID_ },
				yesOver ? yesButtonID_ : (noOver ? noButtonID_ : -1));
			const bool yesHot = yesOver || (focusedButtonId == yesButtonID_);
			const bool noHot = noOver || (focusedButtonId == noButtonID_);
			UpdateHoverVisuals(scene, yesHot, noHot);

			const bool mouseDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
			const bool clickEdge = mouseDown && !mouseHeld_;
			mouseHeld_ = mouseDown;
			const bool keyboardSubmit = MenuKeyboardNavigation::ConsumeSubmitPress(input);

			if (!clickEdge && !keyboardSubmit) {
				return;
			}

			if (clickEdge) {
				input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);
			}

			if (yesHot && (yesOver || focusedButtonId == yesButtonID_)) {
				if (AudioManager* audioManager = scene.GetAudioManager()) {
					if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
						audioManager->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager->GetVfxVolume(), false);
					}

					if (yesAction_ == QuitPopupYesAction::ReturnToMainMenu) {
						const float transitionFadeOut = 0.35f;
						const std::array<const char*, 6> channelsToFade = {
							MyoonchiPaths::Audio::BGM_LEVEL_THEME,
							MyoonchiPaths::Audio::BGM_KITCHEN_AMBIENCE,
							MyoonchiPaths::Audio::BGM_FOREST_AMBIENCE,
							MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE,
							MyoonchiPaths::Audio::BGM_WIN_CUTSCENE,
							MyoonchiPaths::Audio::SFX_GAMEOVER
						};

						for (const char* channelName : channelsToFade) {
							if (audioManager->HasSound(channelName) && audioManager->IsSoundPlaying(channelName)) {
								audioManager->FadeChannel(channelName, 0.0f, transitionFadeOut);
							}
						}
					}
				}

				if (yesAction_ == QuitPopupYesAction::ReturnToMainMenu) {
					Clear(scene);
					scene.HidePauseOverlay();
					scene.RequestResumeFromPauseOverlay();
					scene.StartLevelTransition(MyoonchiPaths::Levels::MAIN_MENU, false);
				}
				else {
					if (GLFWwindow* win = glfwGetCurrentContext()) {
						glfwSetWindowShouldClose(win, GLFW_TRUE);
					}
				}
				return;
			}

			if (noHot && (noOver || focusedButtonId == noButtonID_)) {
				if (AudioManager* audioManager = scene.GetAudioManager()) {
					if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
						audioManager->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager->GetVfxVolume(), false);
					}
				}

				Clear(scene);
				// Prevent stale edge/held states from leaking into resume logic on the next frame after closing the popup
				input.ClearState();
			}
		}
	};

	static QuitPopupState gQuitPopup;

	class QuitPopupOpenButtonLogic final : public GameObjectLogic {
	public:
		explicit QuitPopupOpenButtonLogic(int ownerID, QuitPopupYesAction yesAction)
			: GameObjectLogic(ownerID), yesActionOnOpen_(yesAction) {}

		void Update(float dt, Scene& scene, InputManager& input) override {
			(void)dt;

			if (gQuitPopup.shown_) {
				gQuitPopup.Update(scene, input);
				return;
			}

			if (!scene.ShouldUseRuntimeParityMode()) {
				if (hovered_) {
					hovered_ = false;
					const std::string cacheKey = "pause_quit_normal_" + std::to_string(GetOwnerID());
					QuitPopupState::TrySetObjectTexture(scene, GetOwnerID(), normalTexturePath_.c_str(), cacheKey);
				}
				return;
			}

			if (scene.IsHowToPlayOverlayActive() || scene.IsMenuModalActive()) {
				if (hovered_) {
					hovered_ = false;
					const std::string cacheKey = "pause_quit_normal_" + std::to_string(GetOwnerID());
					QuitPopupState::TrySetObjectTexture(scene, GetOwnerID(), normalTexturePath_.c_str(), cacheKey);
				}
				return;
			}

			if (!initialized_) {
				normalTexturePath_ = scene.GetObjectTexturePath(GetOwnerID());
				hoverTexturePath_ = BuildHoverTexturePath(normalTexturePath_);
				initialized_ = true;
			}

			GLFWwindow* window = glfwGetCurrentContext();
			if (!window) {
				return;
			}

			glm::vec2 mouseWorld{};
			QuitPopupState::GetMouseWorld(input, mouseWorld);

			const bool mouseOver = QuitPopupState::IsPointInObject(scene, GetOwnerID(), mouseWorld);
			const std::vector<int> buttonIds = scene.IsPauseOverlayActive()
				? MenuKeyboardNavigation::CollectPauseOverlayButtons(scene)
				: MenuKeyboardNavigation::CollectCurrentSceneTopLevelButtons(scene);
			const std::string scopeKey = scene.IsPauseOverlayActive()
				? MenuKeyboardNavigation::GetPauseOverlayScopeKey(scene)
				: MenuKeyboardNavigation::GetCurrentSceneTopLevelScopeKey(scene);
			const int focusedButtonId = MenuKeyboardNavigation::UpdateFocus(
				scene,
				input,
				scopeKey,
				buttonIds,
				mouseOver ? GetOwnerID() : -1);
			const bool hoveredNow = mouseOver || (focusedButtonId == GetOwnerID());
			if (hoveredNow != hovered_) {
				hovered_ = hoveredNow;
				if (hovered_) {
					scene.TriggerUiButtonHoverFeedback(GetOwnerID());
					if (AudioManager* audioManager = scene.GetAudioManager()) {
						if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
							audioManager->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager->GetVfxVolume(), false);
						}
					}
				}

				const std::string cacheKey = hovered_
					? ("pause_quit_hover_" + std::to_string(GetOwnerID()))
					: ("pause_quit_normal_" + std::to_string(GetOwnerID()));

				QuitPopupState::TrySetObjectTexture(
					scene,
					GetOwnerID(),
					hovered_ ? hoverTexturePath_.c_str() : normalTexturePath_.c_str(),
					cacheKey);
			}

			const bool mouseDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
			const bool clickEdge = mouseDown && !mouseHeld_;
			mouseHeld_ = mouseDown;
			const bool keyboardSubmit = (focusedButtonId == GetOwnerID()) && MenuKeyboardNavigation::ConsumeSubmitPress(input);

			if ((mouseOver && clickEdge) || keyboardSubmit) {
				if (AudioManager* audioManager = scene.GetAudioManager()) {
					if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
						audioManager->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager->GetVfxVolume(), false);
					}
				}

				if (mouseOver && clickEdge) {
					input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);
				}
				gQuitPopup.Show(scene, yesActionOnOpen_);
			}
		}

	private:
		static std::string BuildHoverTexturePath(const std::string& normalPath) {
			if (normalPath.empty()) {
				return normalPath;
			}

			const std::string suffix = "_s.png";
			if (normalPath.size() >= suffix.size() &&
				normalPath.compare(normalPath.size() - suffix.size(), suffix.size(), suffix) == 0) {
				std::string p = normalPath;
				p.replace(p.size() - suffix.size(), suffix.size(), "_h.png");
				return p;
			}

			const std::string ext = ".png";
			if (normalPath.size() >= ext.size() &&
				normalPath.compare(normalPath.size() - ext.size(), ext.size(), ext) == 0) {
				std::string p = normalPath;
				p.insert(p.size() - ext.size(), "_h");
				return p;
			}

			return normalPath;
		}

		bool mouseHeld_ = false;
		bool initialized_ = false;
		bool hovered_ = false;
		std::string normalTexturePath_{};
		std::string hoverTexturePath_{};
		QuitPopupYesAction yesActionOnOpen_ = QuitPopupYesAction::QuitApplication;
	};
}

namespace {
	/************************************************************************/
	/*!
	\brief
		File-local query/helper utilities used by tutorial flow and hook
		callbacks. These helpers perform object scans and lightweight state
		inference from Scene + LogicManager.

	\details
		Most helpers return IDs or booleans and intentionally avoid side effects.
		This keeps hook implementations deterministic and easy to reason about.
	*/
	/************************************************************************/

	static int FindFirstByTagAndTexture(Scene& scene, const std::string& tag, const char* texContains) {
		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) continue;
			const int id = obj->GetID();
			if (scene.GetObjectTag(id) != tag) continue;

			const std::string& tex = scene.GetObjectTexturePath(id);
			if (texContains && tex.find(texContains) == std::string::npos) continue;
			return id;
		}
		return -1;
	}

	static std::optional<DishType> TryGetCurrentOrderDish(Scene& scene) {
		LogicManager& logic = scene.GetLogicManager();

		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) continue;

			auto* table = logic.GetLogicForObject<CustomerTableLogic>(obj->GetID());
			if (!table || !table->HasSeatedCustomer()) continue;

			for (int customerId : table->GetSeatedCustomerIDs()) {
				if (customerId < 0) continue;

				auto* npc = logic.GetLogicForObject<SimpleNpcLogic>(customerId);
				if (!npc) continue;

				if (npc->IsWaitingForFood() && npc->HasOrderBeenTaken() && !npc->HasDishServed()) {
					return npc->GetDesiredDishType();
				}
			}
		}

		return std::nullopt;
	}

	static bool IsHeldItemPlate(Scene& scene, LogicManager& logic, int heldItemID) {
		if (heldItemID < 0) return false;

		// Primary check
		if (logic.GetLogicForObject<PlateLogic>(heldItemID) != nullptr) {
			return true;
		}

		// Fallback by texture path
		const std::string& tex = scene.GetObjectTexturePath(heldItemID);
		return (tex.find("plate") != std::string::npos || tex.find("Plate") != std::string::npos);
	}

	static int FindFirstTableHoldingAnyPlate(Scene& scene) {
		LogicManager& logic = scene.GetLogicManager();

		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) continue;

			TableLogic* table = logic.GetLogicForObject<TableLogic>(obj->GetID());
			if (!table || !table->HasItem()) continue;

			if (IsHeldItemPlate(scene, logic, table->GetHeldItemID())) {
				return obj->GetID();
			}
		}

		return -1;
	}

	static std::vector<int> FindAllEmptyTables(Scene& scene) {
		LogicManager& logic = scene.GetLogicManager();
		std::vector<int> ids;

		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) continue;

			const int id = obj->GetID();
			if (scene.GetObjectTag(id) != "table") continue;

			TableLogic* table = logic.GetLogicForObject<TableLogic>(id);
			if (!table) continue;
			if (table->HasItem()) continue;

			ids.push_back(id);
		}

		return ids;
	}

	static bool HasAnySeatedCustomer(Scene& scene) {
		LogicManager& logic = scene.GetLogicManager();

		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) continue;

			auto* table = logic.GetLogicForObject<CustomerTableLogic>(obj->GetID());
			if (table && table->HasSeatedCustomer()) {
				return true;
			}
		}
		return false;
	}

	static bool HasActiveOrderUi(Scene& scene) {
		// Reuse your existing criteria for "customer order is active / visible"
		return TryGetCurrentOrderDish(scene).has_value();
	}

	static bool IsTutorialLevelLoaded(Scene& scene) {
		const std::string levelPath = scene.GetCurrentLevelPath();
		return levelPath == MyoonchiPaths::Levels::TUTORIAL ||
			levelPath.find("tutorial") != std::string::npos;
	}

	static bool IsDayClearLevelPath(const std::string& levelPath) {
		return levelPath == FilePaths::Levels::WIN ||
			levelPath.find("win") != std::string::npos ||
			levelPath.find("dayclear") != std::string::npos ||
			levelPath.find("day_clear") != std::string::npos;
	}

	static bool IsDayClearLevelLoaded(Scene& scene) {
		const std::string levelPath = scene.GetCurrentLevelPath();
		return IsDayClearLevelPath(levelPath);
	}

	static const char* IngredientBoxTokenForDish(DishType d) {
		switch (d) {
		case DishType::VegDish:  return "VegIngredientBox";
		case DishType::MeatDish: return "MeatIngredientBox";
		case DishType::SoupDish: return "MeatIngredientBox"; // first ingredient in your recipe UI
		default:                 return "VegIngredientBox";
		}
	}

	static const char* StationTokenForDish(DishType d) {
		switch (d) {
		case DishType::VegDish:  return "Cutting_Board";
		case DishType::MeatDish: return "Grills";
		case DishType::SoupDish: return "Grills"; // first station in your recipe UI
		default:                 return "Cutting_Board";
		}
	}

	static const char* StationTokenForRawIngredient(IngredientType t) {
		switch (t) {
		case IngredientType::Vegetable: return "Cutting_Board";
		case IngredientType::Meat:      return "Grills";
		case IngredientType::Shroom:    return "Stove";
		default:                        return nullptr;
		}
	}

	static const char* IngredientBoxTokenForDishSecond(DishType d) {
		switch (d) {
		case DishType::VegDish:  return "VegIngredientBox";   // veg + veg
		case DishType::MeatDish: return "VegIngredientBox";   // meat + veg
		case DishType::SoupDish: return "ShroomIngredientBox"; // meat + shroom
		default:                 return "VegIngredientBox";
		}
	}

	static const char* StationTokenForDishSecond(DishType d) {
		switch (d) {
		case DishType::VegDish:  return "Cutting_Board";
		case DishType::MeatDish: return "Cutting_Board";
		case DishType::SoupDish: return "Stove";
		default:                 return "Cutting_Board";
		}
	}

	static int FindTableHoldingPlate(Scene& scene, int minIngredients, bool requireUnpreparedDish) {
		LogicManager& logic = scene.GetLogicManager();

		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) continue;

			TableLogic* table = logic.GetLogicForObject<TableLogic>(obj->GetID());
			if (!table || !table->HasItem()) continue;

			const int held = table->GetHeldItemID();
			PlateLogic* plate = logic.GetLogicForObject<PlateLogic>(held);
			if (!plate) continue;

			if (plate->GetIngredientCount() < minIngredients) continue;
			if (requireUnpreparedDish && plate->HasPreparedDish()) continue;

			return obj->GetID();
		}

		return -1;
	}

	static int FindCustomerTableReadyForPayment(Scene& scene) {
		LogicManager& logic = scene.GetLogicManager();

		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) continue;

			auto* table = logic.GetLogicForObject<CustomerTableLogic>(obj->GetID());
			if (!table || !table->HasSeatedCustomer()) continue;

			for (int customerId : table->GetSeatedCustomerIDs()) {
				if (customerId < 0) continue;

				auto* npc = logic.GetLogicForObject<SimpleNpcLogic>(customerId);
				if (!npc) continue;

				if (!npc->HasPaid() && (npc->IsPaying() || npc->HasFinishedEating())) {
					return obj->GetID();
				}
			}
		}

		return -1;
	}
}

namespace {
	/************************************************************************/
	/*!
	\brief
		Tutorial finite-state controller.

	\details
		Owns tutorial progression state, highlight overlays, and completion popup
		interaction. The flow is level-gated (tutorial-only) and is safe to tick
		every frame, including while gameplay simulation is inactive.
	*/
	/************************************************************************/
	enum class TutorialStep {
		Move = 0,
		WaitForFirstCustomerOrder,
		PickFirstIngredient,
		ProcessFirstIngredient,
		PlateFirstIngredient,
		PickSecondIngredient,
		ProcessSecondIngredient,
		GetPlateAndPlaceOnTable,
		CombineDishOnPlate,
		ServeDish,
		WaitForCustomerToFinishFood,
		CollectMoneyFromCustomer,
		FinalCustomerFreePlay,
		Done
	};

	struct TutorialFlow {
		bool active = false;
		TutorialStep step = TutorialStep::Move;
		glm::vec2 moveStart{ 0.0f, 0.0f };
		bool moveStartCaptured = false;
		int lastMoney = 0;
		int paymentsCollected_ = 0;
		std::unordered_set<int> paidCustomerIDs_{};
		bool completionPopupShown_ = false;
		std::vector<int> completionPopupIDs_{};

		float blinkAccum_ = 0.0f;
		bool blinkOn_ = true;
		static constexpr float kBlinkHalfPeriod_ = 0.18f;

		static constexpr float kOutlineOffset = 2.5f;
		static constexpr int kOutlineSortOrder = 200;
		static constexpr int kOutlineMaskSortOrder = 201;
		const glm::vec4 kOutlineTint{ 0.0f, 1.0f, 1.0f, 0.92f };

		struct OutlineSet {
			int sourceID = -1;
			std::array<int, 5> ids{ -1, -1, -1, -1, -1 }; // 4 cyan edges + 1 center mask
		};

		OutlineSet ingredientOutline_{};
		OutlineSet stationOutline_{};
		std::vector<OutlineSet> extraStationOutlines_{};

		int completionMenuButtonID_ = -1;
		bool popupMouseHeld_ = false;
		bool completionMenuHovered_ = false;
		static constexpr const char* kCompletionMenuHoverTex_ = "../assets/UI/return_h.png";

		bool GetMouseWorld(InputManager& input, glm::vec2& outWorld) {
			if (GraphicsEngine::Instance().GetMouseWorldInScene(outWorld)) {
				return true;
			}
			glm::vec3 w = input.ScreenToWorld(
				static_cast<float>(input.GetMousePosition().x),
				static_cast<float>(input.GetMousePosition().y));
			outWorld = glm::vec2(w.x, w.y);
			return true;
		}

		bool IsPointInObject(Scene& scene, int objectID, const glm::vec2& p) {
			GameObject* obj = scene.GetGameObjectByID(objectID);
			if (!obj) return false;

			const glm::vec3 pos = obj->GetPositionGLM();
			const glm::vec3 sz = obj->GetScaleGLM();
			const glm::vec2 min(pos.x - sz.x * 0.5f, pos.y - sz.y * 0.5f);
			const glm::vec2 max(pos.x + sz.x * 0.5f, pos.y + sz.y * 0.5f);

			return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
		}

		void RefreshPaymentProgress(Scene& scene, LogicManager& logic) {
			int newlyPaidCount = 0;

			for (GameObject* obj : scene.GetAllObjectsRaw()) {
				if (!obj) continue;

				const int id = obj->GetID();
				auto* npc = logic.GetLogicForObject<SimpleNpcLogic>(id);
				if (!npc || !npc->HasPaid()) continue;

				if (paidCustomerIDs_.insert(id).second) {
					++newlyPaidCount;
				}
			}

			if (newlyPaidCount <= 0) {
				return;
			}

			paymentsCollected_ += newlyPaidCount;
			lastMoney = Economy::gPlayerMoney;

			if (paymentsCollected_ >= 2) {
				step = TutorialStep::Done;
				scene.SetRuntimeTextByName("TutorialText", "");
				ShowCompletionPopup(scene);
				return;
			}

			if (step != TutorialStep::Done && step != TutorialStep::FinalCustomerFreePlay) {
				step = TutorialStep::FinalCustomerFreePlay;
				scene.SetRuntimeTextByName("TutorialText", "Serve the last customer to complete the Tutorial!");
			}
		}

		void UpdateCompletionButtonHoverVisual(Scene& scene, const glm::vec2& mouseWorld) {
			if (completionMenuButtonID_ < 0) {
				return;
			}

			const bool isHoveredNow = IsPointInObject(scene, completionMenuButtonID_, mouseWorld);
			if (isHoveredNow == completionMenuHovered_) {
				return;
			}
			completionMenuHovered_ = isHoveredNow;

			if (completionMenuHovered_) {
				scene.TriggerUiButtonHoverFeedback(completionMenuButtonID_);
				if (AudioManager* audioManager = scene.GetAudioManager()) {
					if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
						audioManager->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager->GetVfxVolume(), false);
					}
				}
			}

			GameObject* button = scene.GetGameObjectByID(completionMenuButtonID_);
			if (!button) {
				return;
			}

			const char* targetPath = completionMenuHovered_ ? kCompletionMenuHoverTex_ : FilePaths::Textures::BTN_RETURN;
			const std::string cacheKey = completionMenuHovered_ ? "tutorial_completion_return_h" : "tutorial_completion_return_s";
			if (Texture* tex = ResourceManager::Instance().LoadTexture(cacheKey, targetPath)) {
				button->SetTexture(tex);
				scene.SetObjectTexturePath(completionMenuButtonID_, targetPath);
			}
		}

		void HandleCompletionPopupInput(Scene& scene) {
			GLFWwindow* window = glfwGetCurrentContext();
			if (!window) {
				return;
			}

			double mx = 0.0;
			double my = 0.0;
			glfwGetCursorPos(window, &mx, &my);

			glm::vec2 mouseWorld{};
			if (!GraphicsEngine::Instance().GetMouseWorldInScene(mouseWorld)) {
				InputManager& input = InputManager::Get();
				glm::vec3 w = input.ScreenToWorld(static_cast<float>(mx), static_cast<float>(my));
				mouseWorld = glm::vec2(w.x, w.y);
			}

			UpdateCompletionButtonHoverVisual(scene, mouseWorld);

			const bool mouseDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
			const bool clickEdge = mouseDown && !popupMouseHeld_;
			popupMouseHeld_ = mouseDown;

			if (!clickEdge) {
				return;
			}

			if (IsPointInObject(scene, completionMenuButtonID_, mouseWorld)) {
				ClearCompletionPopup(scene);
				if (AudioManager* audioManager = scene.GetAudioManager()) {
					if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
						audioManager->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager->GetVfxVolume(), false);
					}

					const float transitionFadeOut = 0.35f;
					const std::array<const char*, 6> channelsToFade = {
						MyoonchiPaths::Audio::BGM_LEVEL_THEME,
						MyoonchiPaths::Audio::BGM_KITCHEN_AMBIENCE,
						MyoonchiPaths::Audio::BGM_FOREST_AMBIENCE,
						MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE,
						MyoonchiPaths::Audio::BGM_WIN_CUTSCENE,
						MyoonchiPaths::Audio::SFX_GAMEOVER
					};

					for (const char* channelName : channelsToFade) {
						if (audioManager->HasSound(channelName) && audioManager->IsSoundPlaying(channelName)) {
							audioManager->FadeChannel(channelName, 0.0f, transitionFadeOut);
						}
					}
				}
				scene.StartLevelTransition(MyoonchiPaths::Levels::MAIN_MENU, false);
			}
		}

		void ClearExtraStationOutlines(Scene& scene) {
			for (auto& set : extraStationOutlines_) {
				DespawnOutlineSet(scene, set);
			}
			extraStationOutlines_.clear();
		}

		void EnsureExtraStationOutlines(Scene& scene, const std::vector<int>& targetIDs) {
			if (extraStationOutlines_.size() < targetIDs.size()) {
				extraStationOutlines_.resize(targetIDs.size());
			}

			for (size_t i = 0; i < targetIDs.size(); ++i) {
				EnsureOutlineTarget(scene, extraStationOutlines_[i], targetIDs[i]);
			}

			for (size_t i = targetIDs.size(); i < extraStationOutlines_.size(); ++i) {
				DespawnOutlineSet(scene, extraStationOutlines_[i]);
			}
		}


		void DespawnOutlineSet(Scene& scene, OutlineSet& set) {
			for (int id : set.ids) {
				if (id >= 0 && scene.GetGameObjectByID(id)) {
					scene.DespawnByID(id);
				}
			}
			set.ids = { -1, -1, -1, -1, -1 };
			set.sourceID = -1;
		}

		void SetOutlineVisible(Scene& scene, OutlineSet& set, bool visible) {
			const float a = visible ? kOutlineTint.a : 0.0f;
			for (int i = 0; i < 4; ++i) {
				const int id = set.ids[i];
				if (id < 0) continue;
				if (GameObject* o = scene.GetGameObjectByID(id)) {
					o->SetColorTint(glm::vec4(kOutlineTint.r, kOutlineTint.g, kOutlineTint.b, a));
				}
			}
			// mask stays opaque to preserve interior
			if (set.ids[4] >= 0) {
				if (GameObject* m = scene.GetGameObjectByID(set.ids[4])) {
					m->SetColorTint(glm::vec4(1.f, 1.f, 1.f, 1.f));
				}
			}
		}

		void SyncOutlineSet(Scene& scene, OutlineSet& set) {
			if (set.sourceID < 0) return;
			GameObject* src = scene.GetGameObjectByID(set.sourceID);
			if (!src) {
				DespawnOutlineSet(scene, set);
				return;
			}

			const glm::vec3 p = src->GetPositionGLM();
			const glm::vec3 s = src->GetScaleGLM();
			const float r = src->GetRotation();
			const std::string layer = scene.GetObjectLayer(set.sourceID);

			const std::array<glm::vec2, 4> offsets{
				glm::vec2(-kOutlineOffset, 0.0f),
				glm::vec2(kOutlineOffset, 0.0f),
				glm::vec2(0.0f, -kOutlineOffset),
				glm::vec2(0.0f,  kOutlineOffset)
			};

			for (int i = 0; i < 4; ++i) {
				const int id = set.ids[i];
				if (id < 0) continue;
				if (GameObject* o = scene.GetGameObjectByID(id)) {
					o->SetPosition(glm::vec3(p.x + offsets[i].x, p.y + offsets[i].y, p.z));
					o->SetScale(glm::vec3(std::abs(s.x), std::abs(s.y), 1.0f));
					o->SetRotation(r, glm::vec3(0, 0, 1));
					o->SetRenderSortOrder(kOutlineSortOrder);
					scene.AssignObjectToLayer(id, layer);
				}
			}

			const int maskID = set.ids[4];
			if (maskID >= 0) {
				if (GameObject* m = scene.GetGameObjectByID(maskID)) {
					m->SetPosition(p);
					m->SetScale(glm::vec3(std::abs(s.x), std::abs(s.y), 1.0f));
					m->SetRotation(r, glm::vec3(0, 0, 1));
					m->SetRenderSortOrder(kOutlineMaskSortOrder);
					scene.AssignObjectToLayer(maskID, layer);
				}
			}
		}

		void SpawnOutlineSet(Scene& scene, OutlineSet& set, int sourceID) {
			DespawnOutlineSet(scene, set);

			GameObject* src = scene.GetGameObjectByID(sourceID);
			if (!src) return;

			const std::string tex = scene.GetObjectTexturePath(sourceID);
			if (tex.empty()) return;

			const std::string layer = scene.GetObjectLayer(sourceID);
			const glm::vec3 p = src->GetPositionGLM();
			const glm::vec3 s = src->GetScaleGLM();
			const float r = src->GetRotation();

			const std::array<glm::vec2, 4> offsets{
				glm::vec2(-kOutlineOffset, 0.0f),
				glm::vec2(kOutlineOffset, 0.0f),
				glm::vec2(0.0f, -kOutlineOffset),
				glm::vec2(0.0f,  kOutlineOffset)
			};

			for (int i = 0; i < 4; ++i) {
				GameObject* o = scene.SpawnStaticSprite(
					tex,
					glm::vec3(p.x + offsets[i].x, p.y + offsets[i].y, p.z),
					glm::vec2(std::abs(s.x), std::abs(s.y)),
					layer
				);
				if (!o) continue;

				o->SetColorTint(kOutlineTint);
				o->SetColliderSize(Math::Vector2D(0.f, 0.f));
				o->SetMovableByPhysics(false);
				o->EnableShadow(false);
				o->SetRotation(r, glm::vec3(0, 0, 1));
				o->SetRenderSortOrder(kOutlineSortOrder);

				if (Shader* outlineShader = ResourceManager::Instance().GetShader("hover_outline")) {
					o->SetShader(outlineShader);
				}

				set.ids[i] = o->GetID();
			}

			GameObject* m = scene.SpawnStaticSprite(
				tex,
				p,
				glm::vec2(std::abs(s.x), std::abs(s.y)),
				layer
			);
			if (m) {
				m->SetColorTint(glm::vec4(1.f, 1.f, 1.f, 1.f));
				m->SetColliderSize(Math::Vector2D(0.f, 0.f));
				m->SetMovableByPhysics(false);
				m->EnableShadow(false);
				m->SetRotation(r, glm::vec3(0, 0, 1));
				m->SetRenderSortOrder(kOutlineMaskSortOrder);
				set.ids[4] = m->GetID();
			}

			set.sourceID = sourceID;
		}

		void EnsureOutlineTarget(Scene& scene, OutlineSet& set, int targetID) {
			if (targetID < 0) {
				DespawnOutlineSet(scene, set);
				return;
			}
			if (set.sourceID != targetID) {
				SpawnOutlineSet(scene, set, targetID);
			}
			else {
				SyncOutlineSet(scene, set);
			}
		}

		void ClearHighlights(Scene& scene) {
			DespawnOutlineSet(scene, ingredientOutline_);
			DespawnOutlineSet(scene, stationOutline_);
			ClearExtraStationOutlines(scene);
			blinkAccum_ = 0.0f;
			blinkOn_ = true;
		}

		void UpdateHighlights(Scene& scene, PlayerLogic* playerLogic, LogicManager& logic, float dt) {
			int targetIngredientID = -1;
			int targetStationID = -1;
			std::vector<int> targetStationIDs;

			const DishType dish = TryGetCurrentOrderDish(scene).value_or(DishType::VegDish);

			if (step == TutorialStep::PickFirstIngredient) {
				targetIngredientID = FindFirstByTagAndTexture(scene, "ingredient_box", IngredientBoxTokenForDish(dish));
			}
			else if (step == TutorialStep::ProcessFirstIngredient) {
				const char* stationToken = StationTokenForDish(dish);
				if (playerLogic && playerLogic->IsHolding()) {
					const int heldID = playerLogic->GetCarriedItemID();
					if (auto* ing = logic.GetLogicForObject<IngredientLogic>(heldID); ing && ing->IsRaw()) {
						if (const char* mapped = StationTokenForRawIngredient(ing->GetType())) {
							stationToken = mapped;
						}
					}
				}
				targetStationID = FindFirstByTagAndTexture(scene, "work_table", stationToken);
			}
			else if (step == TutorialStep::PlateFirstIngredient) {
				const bool holdingPlate =
					playerLogic && playerLogic->IsHolding() &&
					IsHeldItemPlate(scene, logic, playerLogic->GetCarriedItemID());

				if (holdingPlate) {
					// Only after picking up plate: highlight all empty tables
					targetIngredientID = -1;
					targetStationID = -1;
					targetStationIDs = FindAllEmptyTables(scene);
				}
				else {
					// Before picking up plate: highlight only plate box
					targetIngredientID = FindFirstByTagAndTexture(scene, "plate_box", nullptr);
					targetStationID = -1;
					targetStationIDs.clear();
				}
			}
			else if (step == TutorialStep::PickSecondIngredient) {
				targetIngredientID = FindFirstByTagAndTexture(scene, "ingredient_box", IngredientBoxTokenForDishSecond(dish));
			}
			else if (step == TutorialStep::ProcessSecondIngredient) {
				const char* stationToken = StationTokenForDishSecond(dish);
				if (playerLogic && playerLogic->IsHolding()) {
					const int heldID = playerLogic->GetCarriedItemID();
					if (auto* ing = logic.GetLogicForObject<IngredientLogic>(heldID); ing && ing->IsRaw()) {
						if (const char* mapped = StationTokenForRawIngredient(ing->GetType())) {
							stationToken = mapped;
						}
					}
				}
				targetStationID = FindFirstByTagAndTexture(scene, "work_table", stationToken);
			}
			else if (step == TutorialStep::GetPlateAndPlaceOnTable) {
				const bool holdingPlate =
					playerLogic && playerLogic->IsHolding() &&
					IsHeldItemPlate(scene, logic, playerLogic->GetCarriedItemID());

				if (holdingPlate) {
					// Only after picking up plate: highlight all empty tables
					targetIngredientID = -1;
					targetStationID = -1;
					targetStationIDs = FindAllEmptyTables(scene);
				}
				else {
					// Before picking up plate: highlight only plate box
					targetIngredientID = FindFirstByTagAndTexture(scene, "plate_box", nullptr);
					targetStationID = -1;
					targetStationIDs.clear();
				}
			}
			else if (step == TutorialStep::CombineDishOnPlate) {
				targetStationID = FindTableHoldingPlate(scene, 1, true); // table holding plate with first ingredient
			}
			else if (step == TutorialStep::CollectMoneyFromCustomer) {
				targetStationID = FindCustomerTableReadyForPayment(scene);
			}

			EnsureOutlineTarget(scene, ingredientOutline_, targetIngredientID);

			if (!targetStationIDs.empty()) {
				EnsureOutlineTarget(scene, stationOutline_, targetStationIDs.front());

				if (targetStationIDs.size() > 1) {
					std::vector<int> extras(targetStationIDs.begin() + 1, targetStationIDs.end());
					EnsureExtraStationOutlines(scene, extras);
				}
				else {
					ClearExtraStationOutlines(scene);
				}
			}
			else {
				EnsureOutlineTarget(scene, stationOutline_, targetStationID);
				ClearExtraStationOutlines(scene);
			}

			blinkAccum_ += dt;
			while (blinkAccum_ >= kBlinkHalfPeriod_) {
				blinkAccum_ -= kBlinkHalfPeriod_;
				blinkOn_ = !blinkOn_;
			}

			SetOutlineVisible(scene, ingredientOutline_, blinkOn_);
			SetOutlineVisible(scene, stationOutline_, blinkOn_);
			for (auto& set : extraStationOutlines_) {
				SetOutlineVisible(scene, set, blinkOn_);
			}
		}

		void Reset(Scene& scene, bool enable) {
			ClearHighlights(scene);
			ClearCompletionPopup(scene);
			active = enable;
			step = TutorialStep::Move;
			moveStart = { 0.0f, 0.0f };
			moveStartCaptured = false;
			lastMoney = Economy::gPlayerMoney;
			paymentsCollected_ = 0;
			paidCustomerIDs_.clear();
			scene.SetRuntimeTextByName("TutorialText", active ? "Click anywhere to move." : "");
		}

		void Advance(Scene& scene, const std::string& nextText) {
			if (step != TutorialStep::Done) {
				step = static_cast<TutorialStep>(static_cast<int>(step) + 1);
			}
			scene.SetRuntimeTextByName("TutorialText", nextText);
		}

		void Update(Scene& scene, float dt) {
			const bool isTutorial = IsTutorialLevelLoaded(scene);
			if (!isTutorial) {
				ClearHighlights(scene);
				ClearCompletionPopup(scene);
				active = false;
				return;
			}

			if (completionPopupShown_) {
				ClearHighlights(scene);
				HandleCompletionPopupInput(scene);
				return;
			}

			if (!active || !scene.IsSimulationActive()) {
				ClearHighlights(scene);
				return;
			}

			const int playerId = scene.GetPlayerID();
			GameObject* player = scene.GetGameObjectByID(playerId);
			if (!player) return;

			LogicManager& logic = scene.GetLogicManager();
			PlayerLogic* playerLogic = logic.GetLogicForObject<PlayerLogic>(playerId);
			if (!playerLogic) return;

			RefreshPaymentProgress(scene, logic);

			switch (step) {
			case TutorialStep::Move:
			{
				glm::vec3 p = player->GetPositionGLM();
				if (!moveStartCaptured) {
					moveStart = { p.x, p.y };
					moveStartCaptured = true;
				}
				glm::vec2 d = glm::vec2(p.x, p.y) - moveStart;
				if (glm::dot(d, d) > (40.0f * 40.0f)) {
					Advance(scene, "Wait for a customer to arrive.");
				}
				break;
			}

			case TutorialStep::WaitForFirstCustomerOrder:
			{
				if (!HasAnySeatedCustomer(scene)) {
					scene.SetRuntimeTextByName("TutorialText", "Wait for a customer to arrive.");
				}
				else if (!HasActiveOrderUi(scene)) {
					scene.SetRuntimeTextByName("TutorialText", "Wait for a customer to arrive.");
				}
				else {
					Advance(scene, "Pick the first ingredient from the correct ingredient box as seen in the Order.");
				}
				break;
			}

			case TutorialStep::PickFirstIngredient:
			{
				if (playerLogic->IsHolding()) {
					const int heldId = playerLogic->GetCarriedItemID();
					if (auto* ing = logic.GetLogicForObject<IngredientLogic>(heldId); ing && ing->IsRaw()) {
						Advance(scene, "Bring it to the correct workstation as seen in the Order.");
					}
				}
				break;
			}

			case TutorialStep::ProcessFirstIngredient:
			{
				bool startedProcessing = false;
				for (GameObject* obj : scene.GetAllObjectsRaw()) {
					if (!obj) continue;
					if (auto* wt = logic.GetLogicForObject<WorkTableLogic>(obj->GetID()); wt && wt->IsProcessing()) {
						startedProcessing = true;
						break;
					}
				}

				bool holdingProcessed = false;
				if (playerLogic->IsHolding()) {
					const int heldId = playerLogic->GetCarriedItemID();
					if (auto* ing = logic.GetLogicForObject<IngredientLogic>(heldId); ing && ing->IsProcessed()) {
						holdingProcessed = true;
					}
				}

				if (startedProcessing || holdingProcessed) {
					Advance(scene, "Take a plate from the plate box, place it on any empty table.");
				}
				break;
			}

			case TutorialStep::PlateFirstIngredient:
			{
				bool firstOnPlate = false;
				for (GameObject* obj : scene.GetAllObjectsRaw()) {
					if (!obj) continue;
					if (auto* plate = logic.GetLogicForObject<PlateLogic>(obj->GetID());
						plate && plate->GetIngredientCount() >= 1 && !plate->HasPreparedDish()) {
						firstOnPlate = true;
						break;
					}
				}
				if (FindFirstTableHoldingAnyPlate(scene) >= 0) {
					Advance(scene, "Pick the second ingredient from the correct ingredient box as seen in the Order.");
				}
				break;
			}

			case TutorialStep::PickSecondIngredient:
			{
				if (playerLogic->IsHolding()) {
					const int heldId = playerLogic->GetCarriedItemID();
					if (auto* ing = logic.GetLogicForObject<IngredientLogic>(heldId); ing && ing->IsRaw()) {
						Advance(scene, "Bring the second ingredient to its workstation.");
					}
				}
				break;
			}

			case TutorialStep::ProcessSecondIngredient:
			{
				bool startedProcessing = false;
				for (GameObject* obj : scene.GetAllObjectsRaw()) {
					if (!obj) continue;
					if (auto* wt = logic.GetLogicForObject<WorkTableLogic>(obj->GetID()); wt && wt->IsProcessing()) {
						startedProcessing = true;
						break;
					}
				}

				bool holdingProcessed = false;
				if (playerLogic->IsHolding()) {
					const int heldId = playerLogic->GetCarriedItemID();
					if (auto* ing = logic.GetLogicForObject<IngredientLogic>(heldId); ing && ing->IsProcessed()) {
						holdingProcessed = true;
					}
				}

				if (startedProcessing || holdingProcessed) {
					Advance(scene, "Collect a plate and place it on any empty table.");
				}
				break;
			}

			case TutorialStep::GetPlateAndPlaceOnTable:
			{
				if (FindFirstTableHoldingAnyPlate(scene) >= 0) {
					Advance(scene, "Place both processed ingredients on the same plate to combine and make the dish.");
				}
				break;
			}

			case TutorialStep::CombineDishOnPlate:
			{
				bool dishReady = false;
				for (GameObject* obj : scene.GetAllObjectsRaw()) {
					if (!obj) continue;
					if (auto* plate = logic.GetLogicForObject<PlateLogic>(obj->GetID()); plate && plate->HasPreparedDish()) {
						dishReady = true;
						break;
					}
				}
				if (dishReady) {
					Advance(scene, "Serve the completed dish to a customer table.");
				}
				break;
			}

			case TutorialStep::ServeDish:
			{
				bool dishServed = false;
				for (GameObject* obj : scene.GetAllObjectsRaw()) {
					if (!obj) continue;

					auto* table = logic.GetLogicForObject<CustomerTableLogic>(obj->GetID());
					if (!table || !table->HasSeatedCustomer()) continue;

					for (int customerId : table->GetSeatedCustomerIDs()) {
						if (customerId < 0) continue;

						auto* npc = logic.GetLogicForObject<SimpleNpcLogic>(customerId);
						if (!npc) continue;

						if (npc->HasDishServed()) {
							dishServed = true;
							break;
						}
					}

					if (dishServed) {
						Advance(scene, "Wait for customer to finish food.");
					}
					break;
				}
				break;
			}

			case TutorialStep::WaitForCustomerToFinishFood:
			{
				bool readyForPayment = false;
				for (GameObject* obj : scene.GetAllObjectsRaw()) {
					if (!obj) continue;

					auto* table = logic.GetLogicForObject<CustomerTableLogic>(obj->GetID());
					if (!table || !table->HasSeatedCustomer()) continue;

					for (int customerId : table->GetSeatedCustomerIDs()) {
						if (customerId < 0) continue;

						auto* npc = logic.GetLogicForObject<SimpleNpcLogic>(customerId);
						if (!npc) continue;

						if (!npc->HasPaid() && (npc->IsPaying() || npc->HasFinishedEating())) {
							readyForPayment = true;
							break;
						}
					}

					if (readyForPayment) {
						Advance(scene, "Collect money from customer.");
					}
					break;
				}

				break;
			}

			case TutorialStep::CollectMoneyFromCustomer:
			{
				scene.SetRuntimeTextByName("TutorialText", "Collect money from customer.");
				break;
			}

			case TutorialStep::FinalCustomerFreePlay:
			{
				scene.SetRuntimeTextByName("TutorialText", "Serve the last customer to complete the Tutorial!");
				break;
			}

			case TutorialStep::Done:
				break;
			}

			if (step == TutorialStep::Done) {
				ClearHighlights(scene);
			}
			else {
				UpdateHighlights(scene, playerLogic, logic, dt);
			}
		}

		void ClearCompletionPopup(Scene& scene) {
			for (int id : completionPopupIDs_) {
				if (id >= 0 && scene.GetGameObjectByID(id)) {
					scene.DespawnByID(id);
				}
			}
			completionPopupIDs_.clear();
			completionPopupShown_ = false;
			completionMenuButtonID_ = -1;
			popupMouseHeld_ = false;
			completionMenuHovered_ = false;
		}

		void ShowCompletionPopup(Scene& scene) {
			if (completionPopupShown_) return;
			completionPopupShown_ = true;

			// Freeze gameplay while popup is shown.
			scene.SetSimulationActive(false);

			const glm::vec3 center{
				static_cast<float>(GraphicsEngine::kRefW) * 0.5f,
				static_cast<float>(GraphicsEngine::kRefH) * 0.5f,
				0.0f
			};
			const std::string uiLayer = "999999";

			if (GameObject* popup = scene.SpawnStaticSprite(
				"../assets/UI/tutorial_complete.png",
				center,
				glm::vec2(1152.0f, 648.0f),
				uiLayer)) {
				completionPopupIDs_.push_back(popup->GetID());
			}

			if (GameObject* menu = scene.SpawnStaticSprite(
				FilePaths::Textures::BTN_RETURN,
				glm::vec3(center.x, center.y + 160.0f, 0.0f),
				glm::vec2(350.0f, 100.0f),
				uiLayer)) {
				completionMenuButtonID_ = menu->GetID();
				completionPopupIDs_.push_back(completionMenuButtonID_);
				scene.SetObjectTexturePath(completionMenuButtonID_, FilePaths::Textures::BTN_RETURN);
				completionMenuHovered_ = false;
				// Do NOT attach MenuButtonLogic here.
			}
		}
	};

	static TutorialFlow gTutorialFlow;
	/************************************************************************/
	/*!
	\brief
		Assigns engine-level role IDs and authored velocities to objects
		based on their tag. Called by the engine during level loading for
		every object that has a non-empty tag.
	\param scene   The active Scene.
	\param id      Object ID being configured.
	\param tag     The object's tag string from JSON.
	\param speedX  Authored horizontal speed from JSON.
	\param speedY  Authored vertical speed from JSON.
	*/
	/************************************************************************/
	void ApplyTagRules(Scene& scene, int id, const std::string& tag, float speedX, float speedY) {
		if (tag == "player") {
			scene.SetPlayerID(id);
		}
		else if (tag == "npc1") {
			scene.SetNPC1ID(id);
			scene.SetNPCVelocity(id, speedX, speedY);
		}
		else if (tag == "npc2") {
			scene.SetNPC2ID(id);
			scene.SetNPCVelocity(id, speedX, speedY);
		}
		else if (tag == "dino") {
			scene.SetDinoID(id);
			scene.SetNPCVelocity(id, speedX, speedY);
		}
	}

	/************************************************************************/
	/*!
	\brief
		Attaches the correct sprite-sheet animation set to an object based
		on its tag or texture path. Called by the engine's RuntimeLevel
		builder for every object marked as animated in JSON.
	\param scene       The active Scene.
	\param id          Object ID.
	\param tag         Object tag from JSON.
	\param texturePath Texture file path (used as fallback for dino detection).
	\param animated    Whether the JSON entry has "animated": true.
	\param animName    Optional initial animation name override from JSON.
	\param speedX      Authored speed X (unused here, forwarded by engine).
	\param speedY      Authored speed Y (unused here, forwarded by engine).
	*/
	/************************************************************************/
	void ApplyRuntimeObjectSetup(Scene& scene, int id, const std::string& tag, const std::string& texturePath, bool animated, const std::string& animName, float speedX, float speedY) {
		(void)speedX;
		(void)speedY;

		if (!animated) {
			return;
		}

		if (tag == "menu_anim") {
			scene.AttachMenuAnimations(id);
			scene.SetAnimation(id, animName.empty() ? "FULL" : animName);
		}
		else if (tag == "customer_template") {
			scene.AttachCustomersAnimations(id, texturePath);
			scene.SetAnimation(id, animName.empty() ? "IDLE_FRONT" : animName);
		}
		else if (texturePath.find("dino") != std::string::npos || tag == "dino") {
			scene.AttachDinoAnimations(id);
			scene.SetAnimation(id, animName.empty() ? "IDLE" : animName);
		}
	}

	static bool HasAnyUnresolvedCustomer(Scene& scene) {
		LogicManager& logic = scene.GetLogicManager();

		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) continue;

			auto* table = logic.GetLogicForObject<CustomerTableLogic>(obj->GetID());
			if (!table || !table->HasSeatedCustomer()) continue;

			for (int customerId : table->GetSeatedCustomerIDs()) {
				if (customerId < 0) continue;

				auto* npc = logic.GetLogicForObject<SimpleNpcLogic>(customerId);
				if (!npc) continue;

				// Still unresolved until payment has been collected
				if (!npc->HasPaid()) {
					return true;
				}
			}
		}

		return false;
	}

	/************************************************************************/
	/*!
	\brief
		Per-frame simulation hook. Ticks the economy timer, checks for
		win/lose conditions, and triggers countdown sound effects at
		key time thresholds.
	\param dt     Frame delta time in seconds.
	\param scene  The active Scene (used to reach AudioManager).
	*/
	/************************************************************************/
	void UpdateSimulationPolicy(float dt, Scene& scene) {
		if (IsDayClearLevelLoaded(scene)) {
			return;
		}

		float prevTime = Economy::gTimeRemaining;
		Economy::Update(dt, scene);

		if (AudioManager* audioManager = scene.GetAudioManager()) {
			float currentTime = Economy::gTimeRemaining;

			if (!Economy::gPlayed10SecWarning && prevTime > 10.0f && currentTime <= 10.0f) {
				Economy::gPlayed10SecWarning = true;
				if (audioManager->HasSound("sfx_clock_ticking_10secs")) {
					audioManager->PlaySound("sfx_clock_ticking_10secs", audioManager->GetVfxVolume() * 1.2f, false);
				}
			}

			if (!Economy::gPlayed3SecBeep && prevTime > 3.0f && currentTime <= 3.0f) {
				Economy::gPlayed3SecBeep = true;
				if (audioManager->HasSound("sfx_beep")) {
					audioManager->PlaySound("sfx_beep", audioManager->GetVfxVolume(), false);
				}
			}

			if (!Economy::gPlayed2SecBeep && prevTime > 2.0f && currentTime <= 2.0f) {
				Economy::gPlayed2SecBeep = true;
				if (audioManager->HasSound("sfx_beep")) {
					audioManager->PlaySound("sfx_beep", audioManager->GetVfxVolume(), false);
				}
			}

			if (!Economy::gPlayed1SecBeep && prevTime > 1.0f && currentTime <= 1.0f) {
				Economy::gPlayed1SecBeep = true;
				if (audioManager->HasSound("sfx_beep")) {
					audioManager->PlaySound("sfx_beep", audioManager->GetVfxVolume(), false);
				}
			}

			if (!Economy::gPlayedTimeUp && currentTime <= 0.0f) {
				Economy::gPlayedTimeUp = true;
				if (audioManager->HasSound("sfx_time_up")) {
					audioManager->PlaySound("sfx_time_up", audioManager->GetVfxVolume(), false);
				}
			}
		}
	}

	/************************************************************************/
	/*!
	\brief
		Sets the default scene background texture. Called by the engine
		when a scene is first created before any level JSON is loaded.
	\param scene  The Scene to configure.
	*/
	/************************************************************************/
	void ApplyDefaultSceneSetup(Scene& scene) {
		scene.SetSceneBackground(MyoonchiPaths::Textures::BACKGROUND);
	}

	/************************************************************************/
	/*!
	\brief
		Post-level-load hook. Enables the main-menu UI layer when the
		simulation is inactive (menu state) and starts the appropriate
		BGM for the loaded level. Release-only.
	\param scene            The Scene that just finished loading.
	\param simulationActive True if gameplay is active, false for menus.
	*/
	/************************************************************************/
	void OnPostLevelLoaded(Scene& scene, bool simulationActive, CustomerManagerSystem& customerManager) {
		const bool isTutorial = simulationActive && IsTutorialLevelLoaded(scene);
		gTutorialFlow.Reset(scene, isTutorial);
		Economy::BindUIScene(scene);
		gQuitPopup.Clear(scene);

		if (isTutorial) {
			customerManager.SetMaxCustomers(2);
			customerManager.SetTotalSpawnLimit(2);
			customerManager.SetSpawnedCustomersInfinitePatience(true);

			// Tutorial timer: 20 minutes
			Economy::SetTimeLimitSeconds(20.0f * 60.0f);
			Economy::gTimeRemaining = Economy::kTimeLimitSeconds;

			// Prevent normal quota win cutscene during tutorial.
			Economy::SetQuota(999);
			Economy::gQuotaReached = false;
			Economy::SyncUI(&scene);
		}
		else {
			customerManager.SetMaxCustomers(4);
			customerManager.ClearTotalSpawnLimit();
			customerManager.SetSpawnedCustomersInfinitePatience(false);
		}

		if (AudioManager* audioManager = scene.GetAudioManager()) {
			for (GameObject* obj : scene.GetAllObjectsRaw()) {
				if (!obj) continue;
				if (scene.GetObjectTag(obj->GetID()) != "btn_play") continue;
				if (auto* logic = scene.GetLogicManager().GetLogicForObject<StartGamePromptLogic>(obj->GetID())) {
					logic->SetAudioManager(audioManager);
				}
			}
		}

		if (!scene.ShouldUseRuntimeParityMode()) {
			return;
		}

		if (!simulationActive || IsDayClearLevelLoaded(scene)) {
			if (Layer* menuLayer = scene.GetLayer("10")) {
				menuLayer->SetVisible(true);
				menuLayer->SetEnabled(true);
				menuLayer->SetCollidable(false);
			}
		}

		if (AudioManager* audioManager = scene.GetAudioManager()) {
			const std::string levelPath = scene.GetCurrentLevelPath();
			const bool isLevel2Loaded = levelPath.find("kitchen02") != std::string::npos;
			const char* levelAmbienceKey = isLevel2Loaded
				? MyoonchiPaths::Audio::BGM_FOREST_AMBIENCE
				: MyoonchiPaths::Audio::BGM_KITCHEN_AMBIENCE;
			const bool shouldRestartMenuBgm =
				levelPath == FilePaths::Levels::MAIN_MENU ||
				IsDayClearLevelPath(levelPath);

			scene.SetPauseOverlayAudioChannels(MyoonchiPaths::Audio::BGM_LEVEL_THEME, levelAmbienceKey);

			const bool useMenuBgm = !simulationActive || IsDayClearLevelLoaded(scene);
			if (useMenuBgm) {
				audioManager->StopSound(MyoonchiPaths::Audio::BGM_LEVEL_THEME);
				audioManager->StopSound(MyoonchiPaths::Audio::BGM_KITCHEN_AMBIENCE);
				audioManager->StopSound(MyoonchiPaths::Audio::BGM_FOREST_AMBIENCE);
				audioManager->StopSound(MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE);
				audioManager->StopSound(MyoonchiPaths::Audio::BGM_WIN_CUTSCENE);
				if (shouldRestartMenuBgm) {
					audioManager->StopSound(MyoonchiPaths::Audio::BGM_MAIN_MENU);
					audioManager->PlaySound(MyoonchiPaths::Audio::BGM_MAIN_MENU, audioManager->GetBgmVolume(), false);
				}
				else if (!audioManager->IsSoundPlaying(MyoonchiPaths::Audio::BGM_MAIN_MENU)) {
					audioManager->PlaySound(MyoonchiPaths::Audio::BGM_MAIN_MENU, audioManager->GetBgmVolume(), false);
				}
				else {
					// Menu-to-menu transitions can leave the shared menu BGM faded out.
					// Reapply the current BGM mix here without replaying the track.
					audioManager->SetBgmVolume(audioManager->GetBgmVolume());
				}
			}
			else {
				const float fadeIn = 1.0f;
				audioManager->PlaySound(MyoonchiPaths::Audio::BGM_LEVEL_THEME, 0.0f, false);
				audioManager->FadeChannel(MyoonchiPaths::Audio::BGM_LEVEL_THEME, audioManager->GetBgmVolume(), fadeIn);

				audioManager->StopSound(MyoonchiPaths::Audio::BGM_KITCHEN_AMBIENCE);
				audioManager->StopSound(MyoonchiPaths::Audio::BGM_FOREST_AMBIENCE);
				audioManager->PlaySound(levelAmbienceKey, 0.0f, false);
				audioManager->FadeChannel(levelAmbienceKey, audioManager->GetBgmVolume() * 0.5f, fadeIn);
			}
		}
	}

	/************************************************************************/
	/*!
	\brief
		Cutscene fade-out audio hook. Fades the intro cutscene BGM to
		silence over the given duration when a cutscene begins to exit.
	\param scene       The active Scene.
	\param outSeconds  Duration of the fade-out in seconds.
	*/
	/************************************************************************/
	void OnCutsceneFadeOut(Scene& scene, float outSeconds) {
		if (!scene.ShouldUseRuntimeParityMode()) {
			return;
		}

		if (AudioManager* audioManager = scene.GetAudioManager()) {
			audioManager->FadeChannel(MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE, 0.0f, outSeconds);
			audioManager->StopSound(MyoonchiPaths::Audio::SFX_INTRO_CUTSCENE);
		}
	}

	/************************************************************************/
	/*!
	\brief
		Cutscene first-frame hook. Starts the win cutscene BGM when the
		first displayed image belongs to the win sequence.
	\param scene       The active Scene.
	\param firstImage  File path of the first cutscene frame being shown.
	*/
	/************************************************************************/
	void OnCutsceneFirstFrame(Scene& scene, const std::string& firstImage) {
		if (!scene.ShouldUseRuntimeParityMode()) {
			return;
		}

		if (AudioManager* audioManager = scene.GetAudioManager()) {
			if (firstImage.find("Win") != std::string::npos || firstImage.find("daychange") != std::string::npos) {
				if (audioManager->HasSound(MyoonchiPaths::Audio::BGM_WIN_CUTSCENE)) {
					audioManager->PlaySound(MyoonchiPaths::Audio::BGM_WIN_CUTSCENE, audioManager->GetBgmVolume(), false);
				}
			}
		}
	}

	/************************************************************************/
	/*!
	\brief
		Cutscene pre-final-load hook. Cleans up all cutscene audio by stopping
		or fading them out before the target level is loaded.
	\param scene       The active Scene.
	\param outSeconds  Fade-out duration in seconds for lingering channels.
	*/
	/************************************************************************/
	void OnCutsceneBeforeFinalLoad(Scene& scene, float outSeconds) {
		if (!scene.ShouldUseRuntimeParityMode()) {
			return;
		}

		if (AudioManager* audioManager = scene.GetAudioManager()) {
			audioManager->StopSound(MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE);
			audioManager->StopSound(MyoonchiPaths::Audio::SFX_INTRO_CUTSCENE);
			if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_GAMEOVER)) {
				audioManager->FadeChannel(MyoonchiPaths::Audio::SFX_GAMEOVER, 0.0f, outSeconds);
			}
			if (audioManager->HasSound(MyoonchiPaths::Audio::BGM_WIN_CUTSCENE)) {
				audioManager->FadeChannel(MyoonchiPaths::Audio::BGM_WIN_CUTSCENE, 0.0f, outSeconds);
			}
		}
	}

	using TagHandler = std::function<void(Scene&, int)>;

	/************************************************************************/
	/*!
	\brief
		Returns the static tag-to-handler dispatch table. Each entry maps
		a JSON tag string to a lambda that attaches the corresponding
		game logic and/or animation to the object. Uses a static local
		so the table is built once and reused on every call (O(1) lookup).
	\return
		Const reference to the dispatch table.
	*/
	/************************************************************************/
	const std::unordered_map<std::string, TagHandler>& GetTagDispatchTable() {
		// Very scalable and easy to maintain as more tags are added.
		static const std::unordered_map<std::string, TagHandler> table = {
			{ "player", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<PlayerLogic>(id);
				scene.SetPlayerID(id);
				scene.AttachPlayerAnimations(id);
			}},
			{ "customer_template", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<SimpleNpcLogic>(id);
			}},
			{ "work_vfx_cut", [](Scene& scene, int id) {
				scene.AttachWorkVfxCutAnimations(id);
				scene.SetAnimation(id, "LOOP");
			}},
			{ "work_vfx_grill", [](Scene& scene, int id) {
				scene.AttachWorkVfxGrillAnimations(id);
				scene.SetAnimation(id, "LOOP");
			}},
			{ "work_vfx_stove", [](Scene& scene, int id) {
				scene.AttachWorkVfxStoveAnimations(id);
				scene.SetAnimation(id, "LOOP");
			}},
			{ "table", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<TableLogic>(id);
			}},
			{ "work_table", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<WorkTableLogic>(id);
			}},
			{ "customer_table", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<CustomerTableLogic>(id);
			}},
			{ "ingredient_box", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<IngredientBoxLogic>(id);
			}},
			{ "plate_box", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<IngredientBoxLogic>(id);
			}},
			{ "exit_gate", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<ExitGateLogic>(id);
				scene.RegisterExitGate(id);
			}},
			{ "trash_box", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<TrashCanLogic>(id);
				scene.RegisterExitGate(id);
			}},
			{ "order_ui_logic", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<OrderUILogic>(id);
			}},
			{ "btn_play", [](Scene& scene, int id) {
				auto* logic = scene.GetLogicManager().AddLogic<StartGamePromptLogic>(id, MyoonchiPaths::Levels::TUTORIAL, MyoonchiPaths::Levels::KITCHEN_01, true);
				if (logic && scene.GetAudioManager()) {
					logic->SetAudioManager(scene.GetAudioManager());
				}
			}},
			{ "btn_howtoplay", [](Scene& scene, int id) {
				auto* logic = scene.GetLogicManager().AddLogic<HowToPlayButtonLogic>(id);
				if (logic && scene.GetAudioManager()) {
					logic->SetAudioManager(scene.GetAudioManager());
				}
			}},
			{ "settings_how_visual", [](Scene& scene, int id) {
				auto* logic = scene.GetLogicManager().AddLogic<HowToPlayButtonLogic>(id);
				if (logic && scene.GetAudioManager()) {
					logic->SetAudioManager(scene.GetAudioManager());
				}
			}},
			{ "btn_settings", [](Scene& scene, int id) {
				auto* logic = scene.GetLogicManager().AddLogic<MenuButtonLogic>(id, FilePaths::Levels::SETTINGS, false);
				if (logic && scene.GetAudioManager()) {
					logic->SetAudioManager(scene.GetAudioManager());
				}
			}},
			{ "btn_credits", [](Scene& scene, int id) {
				// Temporary wiring: route to main menu until a dedicated credits flow exists.
				auto* logic = scene.GetLogicManager().AddLogic<MenuButtonLogic>(id, FilePaths::Levels::CREDITS, false);
				if (logic && scene.GetAudioManager()) {
					logic->SetAudioManager(scene.GetAudioManager());
				}
			}},
			{ "btn_quit", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<QuitPopupOpenButtonLogic>(id, QuitPopupYesAction::QuitApplication);
			}},
			{ "btn_next_level", [](Scene& scene, int id) {
				const bool goMainMenu = Economy::gWinScreenNextGoesToMainMenu;
				const char* targetLevel = goMainMenu ? FilePaths::Levels::MAIN_MENU : FilePaths::Levels::KITCHEN_02;
				const bool activateSimulation = !goMainMenu;
				auto* logic = scene.GetLogicManager().AddLogic<MenuButtonLogic>(id, targetLevel, activateSimulation);
				if (logic && scene.GetAudioManager()) {
					logic->SetAudioManager(scene.GetAudioManager());
				}
			}},
			{ "btn_retry_level", [](Scene& scene, int id) {
				auto* logic = scene.GetLogicManager().AddLogic<MenuButtonLogic>(id, scene.GetCurrentLevelPath(), true);
				if (logic && scene.GetAudioManager()) {
					logic->SetAudioManager(scene.GetAudioManager());
				}
			}},
			{ "btn_main_menu", [](Scene& scene, int id) {
				auto* logic = scene.GetLogicManager().AddLogic<MenuButtonLogic>(id, FilePaths::Levels::MAIN_MENU, false);
				if (logic && scene.GetAudioManager()) {
					logic->SetAudioManager(scene.GetAudioManager());
				}
			}},
			{ "btn_pause", [](Scene& scene, int id) {
			   scene.GetLogicManager().AddLogic<InGamePauseTriggerLogic>(id);
			}},
			{ "settings_ui_logic", [](Scene& scene, int id) {
				auto* logic = scene.GetLogicManager().AddLogic<SettingsMenuLogic>(id);
				if (logic && scene.GetAudioManager()) {
					logic->SetAudioManager(scene.GetAudioManager());
				}
			}},
		};
		return table;
	}

	/************************************************************************/
	/*!
	\brief
		Looks up the given tag in the dispatch table and, if found,
		executes the associated handler to attach game logic and/or
		animations to the object. Unrecognized tags are silently ignored.
	\param scene  The active Scene.
	\param id     Object ID to bind logic to.
	\param tag    The object's tag string from JSON.
	*/
	/************************************************************************/
	void AttachTagLogic(Scene& scene, int id, const std::string& tag) {
		const auto& table = GetTagDispatchTable();
		auto it = table.find(tag);
		if (it != table.end()) {
			it->second(scene, id);
		}
	}

	/************************************************************************/
	/*!
	\brief
		Attaches the correct button logic to a pause-overlay button
		based on its action string.
	\param scene   The active Scene.
	\param id      Object ID of the button.
	\param action  Action identifier authored in the overlay layout.
	*/
	/************************************************************************/
	void AttachPauseOverlayButton(Scene& scene, int id, const std::string& action) {
		LogicManager& logicManager = scene.GetLogicManager();

		if (action == "resume") {
			logicManager.AddLogic<PauseButtonLogic>(id, PauseAction::Resume);
		}
		else if (action == "howtoplay") {
			auto* logic = logicManager.AddLogic<HowToPlayButtonLogic>(id);
			if (logic && scene.GetAudioManager()) {
				logic->SetAudioManager(scene.GetAudioManager());
			}
		}
		else if (action == "quit") {
			logicManager.AddLogic<QuitPopupOpenButtonLogic>(id, QuitPopupYesAction::ReturnToMainMenu);
		}
	}

	/************************************************************************/
	/*!
	\brief
		Applies level-specific gameplay tuning for customer flow and
		economy progression based on the currently loaded kitchen level.

	\param scene
		The active Scene used to identify the current level.

	\param customerManager
		The CustomerManagerSystem to configure with level-specific
		spawn cooldown and customer capacity values.
	*/
	/************************************************************************/
	void ConfigureLevelGameplayTuning(Scene& scene, CustomerManagerSystem& customerManager) {
		const std::string levelPath = scene.GetCurrentLevelPath();
		const bool isLevel1 = levelPath.find("kitchen01") != std::string::npos;
		const bool isLevel2 = levelPath.find("kitchen02") != std::string::npos;

		if (isLevel2) {
			customerManager.ConfigureSpawnCurve(
				20.0f,  // opening grace time before first customer
				17.0f,  // first repeat cooldown
				9.0f,   // rush-hour minimum cooldown
				1.0f,   // drop by 1 second per successful spawn
				180.0f  // stop ramping after 2 minutes
			);

			customerManager.SetMaxCustomers(12);
			Economy::SetTimeLimitSeconds(240.0f);
			Economy::SetQuota(450);
		}
		else if (isLevel1) {
			customerManager.SetSpawnCooldown(20.0f);
			customerManager.SetMaxCustomers(4);
			Economy::SetTimeLimitSeconds(180.0f);
			Economy::SetQuota(200);
		}
	}

	/************************************************************************/
	/*!
	\brief
		Builds a list of AABB blockers for the pathfinding system by
		collecting every table-tagged object that has a valid collider
		on an enabled, collidable layer. The mover's own object is
		excluded to prevent self-blocking.
	\param scene          The active Scene.
	\param moverObjectID  Object ID of the entity requesting navigation.
	\param outBoxes       Output vector filled with blocker AABBs.
	*/
	/************************************************************************/
	void CollectNavigationBlockersForGame(Scene& scene, int moverObjectID, std::vector<collision::AABB>& outBoxes) {
		outBoxes.clear();

		LogicManager& logicMgr = scene.GetLogicManager();

		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) continue;
			if (obj->GetID() == moverObjectID) continue;

			TableLogic* table = logicMgr.GetLogicForObject<TableLogic>(obj->GetID());
			if (!table) continue;

			const Math::Vector2D colSize = obj->GetColliderSize();
			if (colSize.x <= 0.0f || colSize.y <= 0.0f) continue;

			const std::string layerName = scene.GetObjectLayer(obj->GetID());
			Layer* layer = scene.GetLayer(layerName);
			if (layer && (!layer->IsEnabled() || !layer->IsCollidable())) {
				continue;
			}

			const glm::vec3 p = obj->GetPositionGLM();
			outBoxes.push_back(
				physics::MakeColliderBox(obj, Math::Vector3D(p.x, p.y, p.z))
			);
		}
	}
}

/************************************************************************/
/*!
\brief
	Hook registration entry point.

\details
	Order of registration:
	  1) Customer hooks (update/reset)
	  2) Runtime/tag setup hooks
	  3) Simulation and scene lifecycle hooks
	  4) Cutscene and pause/audio hooks
	  5) Logic binders and navigation collector

	This function should remain side-effect free beyond hook wiring.
*/
/************************************************************************/
void RegisterMyoonchiDinerBindings(Scene& scene) {
	// Customer management system (shared across hooks)
	auto customerManager = std::make_shared<CustomerManagerSystem>();
	auto ambientVfx = std::make_shared<AmbientVfxController>();
	auto applyLevelGameplayTuning = [customerManager](Scene& s) {
		ConfigureLevelGameplayTuning(s, *customerManager);
		};

	scene.SetCustomerUpdateHook([customerManager, ambientVfx](float dt, Scene& s) {
		// Stop new customer spawns near endgame.
		// This requires CustomerManagerSystem to support spawn enable/disable cleanly.
		const bool shouldStopSpawning =
			(Economy::gTimeRemaining <= Economy::kStopSpawningThresholdSeconds) ||
			Economy::gAwaitingFinalCustomerClear;

		customerManager->SetSpawningEnabled(!shouldStopSpawning);
		customerManager->Update(dt, s);

		ambientVfx->Update(dt, s);

		// If time is over and no unresolved customers remain, finish the round.
		// Final state depends on whether quota was reached by then.
		if (Economy::gAwaitingFinalCustomerClear && !HasAnyUnresolvedCustomer(s)) {
			if (Economy::gQuotaReached) {
				Economy::OnQuotaReached(s);
			}
			else {
				Economy::OnTimeUp(s);
			}
		}

		gTutorialFlow.Update(s, dt);
		});

	scene.SetCustomerResetHook([customerManager, ambientVfx, applyLevelGameplayTuning](Scene& s) {
		customerManager->Reset();
		ambientVfx->Reset(s);
		applyLevelGameplayTuning(s);
		Economy::Reset();
		});

	scene.SetRuntimeObjectSetupHook(ApplyRuntimeObjectSetup);
	scene.SetTagRuleHook(ApplyTagRules);

	// Per-frame simulation hook (economy timer + countdown SFX)
	scene.SetSimulationUpdateHook(UpdateSimulationPolicy);

	// Scene lifecycle hooks
	scene.SetDefaultSceneSetupHook(ApplyDefaultSceneSetup);
	scene.SetPostLevelLoadHook([customerManager, ambientVfx, applyLevelGameplayTuning](Scene& s, bool simulationActive) {
		ambientVfx->Reset(s);
		OnPostLevelLoaded(s, simulationActive, *customerManager);
		// Apply level-authored quota/timer tuning for both editor loads and live gameplay so
		// the HUD matches the selected kitchen immediately after Load, Play, and Stop.
		applyLevelGameplayTuning(s);

		// Prevent timer carry-over (from tutorial -> kitchen01).
		// Ensures remaining time always matches the configured level time limit after load.
		Economy::gTimeRemaining = Economy::kTimeLimitSeconds;
		Economy::SyncUI(&s);
		});

	// Cutscene audio hooks
	scene.SetCutsceneFadeOutHook(OnCutsceneFadeOut);
	scene.SetCutsceneFirstFrameHook(OnCutsceneFirstFrame);
	scene.SetCutsceneBeforeFinalLoadHook(OnCutsceneBeforeFinalLoad);

	// Pause overlay: tell engine which audio channels to fade on pause
	scene.SetPauseOverlayAudioChannels(MyoonchiPaths::Audio::BGM_LEVEL_THEME, MyoonchiPaths::Audio::BGM_KITCHEN_AMBIENCE);
	scene.SetPauseOverlayAdditionalAudioChannels({ "sfx_grilling_sizzle", "sfx_boiling_sound", "sfx_chopping" });
	scene.SetPauseSuppressedRuntimeTextNames({ "MoneyText", "QuotaText", "QuotaLabelText", "QuotaValueText", "TimerText", "TutorialText" });
	scene.SetEditorPreservedRuntimeTextNames({ "MoneyText", "QuotaText", "QuotaLabelText", "QuotaValueText", "TimerText", "TutorialText" });

	// Logic / UI binders
	scene.SetTagLogicBinder(AttachTagLogic);
	scene.SetPauseOverlayButtonBinder(AttachPauseOverlayButton);
	scene.SetNavigationBlockerCollector(CollectNavigationBlockersForGame);

	// Skip-cutscene audio: stop cutscene BGMs and play skip SFX when player skips
	scene.SetSkipCutsceneAudioHook([](Scene& s, float outSeconds) {
		if (!s.ShouldUseRuntimeParityMode()) {
			return;
		}

		if (AudioManager* audioManager = s.GetAudioManager()) {
			// Fade bgm
			if (audioManager->HasSound(MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE)) {
				audioManager->FadeChannel(MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE, 0.0f, outSeconds);
			}
			audioManager->StopSound(MyoonchiPaths::Audio::SFX_INTRO_CUTSCENE);

			// Stop intro stinger immediately on skip so it does not continue under the transition.
			if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_INTRO_CUTSCENE)) {
				audioManager->StopSound(MyoonchiPaths::Audio::SFX_INTRO_CUTSCENE);
			}

			if (audioManager->HasSound(MyoonchiPaths::Audio::BGM_WIN_CUTSCENE)) {
				audioManager->FadeChannel(MyoonchiPaths::Audio::BGM_WIN_CUTSCENE, 0.0f, outSeconds);
			}

			// Fade out game-over SFX if it is active.
			if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_GAMEOVER)) {
				audioManager->FadeChannel(MyoonchiPaths::Audio::SFX_GAMEOVER, 0.0f, outSeconds);
			}

			// Play skip cutscene SFX as 2D UI sound to preserve its initial transient.
			if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_SKIP_INTRO_CUTSCENE)) {
				audioManager->PlaySound(MyoonchiPaths::Audio::SFX_SKIP_INTRO_CUTSCENE,
					audioManager->GetVfxVolume(),
					false);
			}
		}
		});

	// Tag velocity hook: tells engine which tags use authored velocity
	scene.SetTagUsesVelocityHook([](const std::string& tag) {
		return (tag == "npc1" || tag == "npc2" || tag == "dino");
		});
}
