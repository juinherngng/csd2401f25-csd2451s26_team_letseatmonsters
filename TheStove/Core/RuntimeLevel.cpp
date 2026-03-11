/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			RuntimeLevel.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Implements RuntimeLevel utilities to parse LevelData JSON, spawn animated/static GameObjects with
					proper layers/tags/colliders/animations, set scene backgrounds, rebuild colliders, and store object
					metadata for runtime level loading. (For use outside of editor only.)

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Core/LevelEditorPanelFonts.hpp"
#include "../Graphics/GameObject.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/SceneManager.hpp"

#include "LevelSerializer.hpp"
#include "RuntimeLevel.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <iostream>
#include <unordered_set>
#include <vector>

namespace RuntimeLevel {

	struct LevelManifest {
		std::vector<std::string> textures;
	};

	LevelManifest BuildLevelManifest(const LevelData& data) {
		LevelManifest manifest;
		manifest.textures.reserve(data.objects.size() + 2);

		std::unordered_set<std::string> seenTexturePaths;
		seenTexturePaths.reserve(data.objects.size() + 2);

		if (!data.background.empty() && seenTexturePaths.insert(data.background).second) {
			manifest.textures.push_back(data.background);
		}

		if (!data.backgroundOverlay.empty() && seenTexturePaths.insert(data.backgroundOverlay).second) {
			manifest.textures.push_back(data.backgroundOverlay);
		}

		for (const auto& obj : data.objects) {
			if (obj.texture.empty()) {
				continue;
			}

			if (seenTexturePaths.insert(obj.texture).second) {
				manifest.textures.push_back(obj.texture);
			}
		}

		return manifest;
	}

	void BuildSceneFromLevel(const LevelData& levelIn, Scene& scene) {
		static const std::string kDefaultLayer = "1";
		static const std::vector<glm::vec4> kFullFrame = { glm::vec4(0.f, 0.f, 1.f, 1.f) };
		for (const auto& obj : levelIn.objects) {
			GameObject* g = nullptr;
			const std::string& layerName = obj.layer.empty() ? kDefaultLayer : obj.layer;

			if (obj.animated) {
				g = scene.SpawnAnimatedSprite(
					obj.texture,
					{ obj.x, obj.y, 0.0f },
					{ obj.w, obj.h },
					kFullFrame,
					0.25f, // initial frame duration; real duration comes from SetFrames
					true,
					layerName
				);

				if (!g) {
					std::cerr << "[RuntimeLevel] Spawn failed: " << obj.texture << std::endl;
					continue;
				}

			}
			else {
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
			}
			else {
				// Explicitly clear collider when disabled
				g->SetColliderSize({ 0.f, 0.f });
				g->SetColliderOffset({ 0.f, 0.f });
			}

			// Track texture path for editor/runtime
			scene.SetObjectTexturePath(g->GetID(), obj.texture);

			scene.ApplyTagRules(g->GetID(), obj.tag, obj.speedX, obj.speedY);
			scene.ApplyRuntimeObjectSetup(g->GetID(), obj.tag, obj.texture, obj.animated, obj.animName, obj.speedX, obj.speedY);

			if (g) {
				// Default shadow off unless specified
				const bool shadowOn = obj.shadow; // new JSON bool
				g->EnableShadow(shadowOn);

				// You can keep sizing/offset consistent; they won’t render unless enabled.
				g->SetShadowSize(glm::vec2(obj.w * 0.8f, obj.h * 0.33f));
				g->SetShadowOffset(glm::vec2(0.0f, 55.0f));
				g->SetShadowOpacity(0.65f);
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
			defs.hasApproachOffset2 = obj.hasApproachOffset2;
			defs.approachOffset2 = { obj.approachOffset2X, obj.approachOffset2Y };
			defs.hasCustomerSeatOffset = obj.hasCustomerSeatOffset;
			defs.customerSeatOffset = { obj.customerSeatOffsetX, obj.customerSeatOffsetY };
			defs.visible = obj.visible;

			scene.SetDefaults(g->GetID(), defs);
			scene.AttachLogicForTag(g->GetID(), obj.tag);

			// Allow immediate hide via alpha tint if invisible for visual consistency
			if (!obj.visible) {
				scene.SetObjectVisible(g->GetID(), false);
				// keep object spawned but invisible; alpha tint avoids popping in debug
				g->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
			}

			// Only clamp objects that have colliders
			if (obj.hasCollider) {
				scene.ClampToWalkArea(g);
			}
		}
	}

	bool LoadAndBuild(const std::string& path, Scene& scene) {
		const auto loadStart = std::chrono::steady_clock::now();
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

		if (!data.backgroundOverlay.empty()) {
			std::cout << "[RuntimeLevel] Background overlay set: " << data.backgroundOverlay << std::endl;
			scene.SetSceneBackgroundOverlay(data.backgroundOverlay);
		}
		else {
			scene.ClearSceneBackgroundOverlay();
		}

		const LevelManifest manifest = BuildLevelManifest(data);

		const auto preloadStart = std::chrono::steady_clock::now();
		ResourceManager::Instance().PreloadTextures(manifest.textures);
		const double preloadMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - preloadStart).count();
#ifdef NDEBUG
		(void)preloadMs;
#endif

		const auto buildStart = std::chrono::steady_clock::now();
		BuildSceneFromLevel(data, scene);
		scene.RebuildColliders();

		std::vector<LEPANELFONTS::TextObjectData> parsedTexts;
		parsedTexts.reserve(data.textObjects.size());

		for (const auto& t : data.textObjects) {
			auto& d = parsedTexts.emplace_back();
			d.name = t.name;
			d.fontName = t.fontName;
			d.text = t.text;

			d.x = t.x;
			d.y = t.y;
			d.scale = t.scale;

			d.rotation = t.rotation;
			d.useBlockRotation = t.useBlockRotation;

			d.colorR = t.colorR;
			d.colorG = t.colorG;
			d.colorB = t.colorB;
			d.colorA = t.colorA;

			d.layer = t.layer;
		}

		LEPANELFONTS::SetTextObjectsWithScene(parsedTexts, scene);

#ifndef NDEBUG
		const double buildMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - buildStart).count();
		const double totalMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - loadStart).count();
		std::cout << "[RuntimeLevel] LoadAndBuild '" << path << "': textures=" << manifest.textures.size()
			<< ", preload=" << preloadMs << " ms, build=" << buildMs << " ms, total=" << totalMs << " ms" << std::endl;
#endif

#if 0
		// Create text for menu buttons if this is a menu level
		if (path.find("main_menu") != std::string::npos) {
			scene.CreateMenuButtonTexts();
		}
#endif

		return true;
	}
}
