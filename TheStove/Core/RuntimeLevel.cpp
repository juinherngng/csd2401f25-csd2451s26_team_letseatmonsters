/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			RuntimeLevel.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu

 DESCRIPTION:		Implements RuntimeLevel utilities to parse LevelData JSON, spawn animated/static GameObjects with
					proper layers/tags/colliders/animations, set scene backgrounds, rebuild colliders, and store object
					metadata for runtime level loading. (For use outside of editor only.)

		 All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Graphics/SceneManager.hpp"
#include "../Graphics/GameObject.hpp"
#include "../Graphics/ResourceManager.hpp"

#include "LevelSerializer.hpp"
#include "RuntimeLevel.hpp"

namespace RuntimeLevel {
	void BuildSceneFromLevel(const LevelData& levelIn, Scene& scene) {
		for (const auto& obj : levelIn.objects) {
			GameObject* g = nullptr;
			std::string layerName = obj.layer.empty() ? "1" : obj.layer;

			if (obj.animated) {
				const std::vector<glm::vec4> fullFrame = { glm::vec4(0.f, 0.f, 1.f, 1.f) };
				g = scene.SpawnAnimatedSprite(
					obj.texture,
					{ obj.x, obj.y, 0.0f },
					{ obj.w, obj.h },
					fullFrame,
					0.25f, // initial frame duration; real duration comes from SetFrames
					true,
					layerName
				);

				if (!g) {
					std::cerr << "[RuntimeLevel] Spawn failed: " << obj.texture << std::endl;
					continue;
				}

				// Menu animation: 6x5 sheet, separate from dino/goat
				if (obj.tag == "menu_anim") {
					// Use AnimationManager via Scene helper APIs
					scene.AttachMenuAnimations(g->GetID());
					// Choose the full-sheet looping clip (or allow obj.animName to override)
					scene.SetAnimation(g->GetID(), obj.animName.empty() ? "FULL" : obj.animName);
				}
				// Existing cases (unchanged)
				else if (obj.texture.find("dino") != std::string::npos || obj.tag == "dino") {
					scene.AttachDinoAnimations(g->GetID());
					scene.SetAnimation(g->GetID(), obj.animName.empty() ? "IDLE" : obj.animName);
				}
			} else {
				g = scene.SpawnStaticSprite(obj.texture, { obj.x, obj.y, 0.0f }, { obj.w, obj.h }, layerName);
				if (!g) {
					std::cerr << "[RuntimeLevel] Spawn failed: " << obj.texture << std::endl;
					continue;
				}
			}

			// Rotation: degrees in JSON -> radians in engine
			g->SetRotation(glm::radians(obj.rotation), { 0, 0, 1 });

			// Collider setup honoring has_collider
			if (obj.hasCollider) {
				g->SetColliderSize({ obj.colWidth, obj.colHeight });
				g->SetColliderOffset({ obj.colOffsetX, obj.colOffsetY });

				// Fallback collider if invalid (kept only when collider is enabled)
				if (obj.colWidth <= 0.f || obj.colHeight <= 0.f) {
					const glm::vec3 s = g->GetScaleGLM();
					g->SetColliderSize({ s.x, s.y });
					g->SetColliderOffset({ 0.f, 0.f });
				}
			} else {
				// Explicitly clear collider when disabled
				g->SetColliderSize({ 0.f, 0.f });
				g->SetColliderOffset({ 0.f, 0.f });
			}

			// Track texture path for editor/runtime
			scene.SetObjectTexturePath(g->GetID(), obj.texture);

			// Tag setup + optional velocity
			if (obj.tag == "player") {
				scene.SetPlayerID(g->GetID());
			}
			else if (obj.tag == "npc1") {
				scene.SetNPC1ID(g->GetID());
				scene.SetNPCVelocity(g->GetID(), obj.speedX, obj.speedY);
			}
			else if (obj.tag == "npc2") {
				scene.SetNPC2ID(g->GetID());
				scene.SetNPCVelocity(g->GetID(), obj.speedX, obj.speedY);
			}
			else if (obj.tag == "dino") {
				scene.SetDinoID(g->GetID());
				scene.SetNPCVelocity(g->GetID(), obj.speedX, obj.speedY);
			}

			// Default metadata
			scene.SetTransformFromLevel(g->GetID(), { obj.x, obj.y, 0.0f }, { obj.w, obj.h, 1.0f }, obj.rotation);

			Scene::Defaults defs{};
			defs.pos = { obj.x, obj.y, 0.0f };
			defs.size = { obj.w, obj.h };
			defs.rot = obj.rotation;
			defs.colSize = { obj.colWidth, obj.colHeight };
			defs.colOff = { obj.colOffsetX, obj.colOffsetY };
			defs.vel = { obj.speedX, obj.speedY };
			defs.texture = obj.texture;
			defs.tag = obj.tag;
			defs.layer = obj.layer;
			defs.approachOffset = { obj.approachOffsetX, obj.approachOffsetY };

			scene.SetDefaults(g->GetID(), defs);
			scene.AttachLogicForTag(g->GetID(), obj.tag);

			// Only clamp objects that have colliders
			if (obj.hasCollider) {
				scene.ClampToWalkArea(g);
			}
		}
	}

	bool LoadAndBuild(const std::string& path, Scene& scene) {
		LevelData data{};
		if (!LevelSerializer::Load(path, data)) {
			std::cerr << "[RuntimeLevel] Failed to load level JSON: " << path << std::endl;
			return false;
		}

		scene.ClearAll();

		// Set background if provided, otherwise keep current
		if (!data.background.empty()) {
			std::cout << "[RuntimeLevel] Background set: " << data.background << std::endl;
			scene.SetSceneBackground(data.background);
		}

		BuildSceneFromLevel(data, scene);
		scene.RebuildColliders();

#if 0
		// Create text for menu buttons if this is a menu level
		if (path.find("main_menu") != std::string::npos) {
			scene.CreateMenuButtonTexts();
		}
#endif

		return true;
	}
}