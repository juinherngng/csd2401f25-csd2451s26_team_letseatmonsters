/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			RuntimeLevel.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (50%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(30%)
					Vu Phan Hung, phanhung.vu@digipen.edu   (20%)

 DESCRIPTION:		Implements RuntimeLevel utilities to parse LevelData JSON, spawn animated/static GameObjects with
					proper layers/tags/colliders/animations, set scene backgrounds, rebuild colliders, and store object
					metadata for runtime level loading. (For use outside of editor only.)

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <chrono>
#include <cmath>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <optional>
#include <unordered_set>
#include <vector>

#include "EngineCore/AudioLoading.hpp"
#include "EngineCore/FilePaths.hpp"
#include "EngineCore/LevelSerializer.hpp"
#include "EngineCore/Logger.hpp"
#include "EngineCore/RuntimeLevel.hpp"
#include "EngineCore/RuntimeTextData.hpp"
#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"

namespace {
	namespace fs = std::filesystem;

	/**
	 * @brief Resolves freshest level path.
	 * @param path Path to process.
	 * @return Result produced by this operation.
	 */
	std::optional<fs::path> ResolveFreshestLevelPath(const std::string& path) {
		std::vector<fs::path> candidates;
		candidates.emplace_back(path);

		const fs::path inputPath(path);
		const fs::path fileName = inputPath.filename();
		if (!fileName.empty()) {
			candidates.emplace_back(fs::path("../levels") / fileName);
			candidates.emplace_back(fs::path("../../levels") / fileName);
			candidates.emplace_back(fs::path("levels") / fileName);
		}

		std::unordered_set<std::string> seen;
		std::optional<fs::path> freshest;
		fs::file_time_type freshestTime{};

		for (const fs::path& candidate : candidates) {
			std::error_code ec;
			const fs::path normalized = candidate.lexically_normal();
			const std::string key = normalized.string();
			if (!seen.insert(key).second) {
				continue;
			}

			if (!fs::exists(normalized, ec) || ec) {
				continue;
			}

			const fs::file_time_type modified = fs::last_write_time(normalized, ec);
			if (ec) {
				continue;
			}

			if (!freshest || modified >= freshestTime) {
				freshest = normalized;
				freshestTime = modified;
			}
		}

		return freshest;
	}

	/**
	 * @brief Determines whether the input is finite.
	 * @param value Parameter for value.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsFinite(float value) {
		return std::isfinite(value);
	}

	/**
	 * @brief Resolves the default runtime font path used by authored HUD text.
	 * @return Best-effort filesystem path to the bundled default font.
	 */
	std::string ResolveDefaultRuntimeFontPath() {
		const std::vector<std::string> candidates = {
			FilePaths::Fonts::AGENCYB,
			"assets/Font/AGENCYB.ttf",
			std::string(FilePaths::Dirs::FONTS) + "AGENCYB.ttf"
		};

		for (const std::string& candidate : candidates) {
			if (!candidate.empty() && fs::exists(candidate)) {
				return candidate;
			}
		}

		return {};
	}

	/**
	 * @brief Ensures commonly-authored runtime fonts are loaded before scene text is rendered.
	 * @param fontName Logical font name referenced by the level data.
	 */
	void EnsureRuntimeFontLoaded(const std::string& fontName) {
		if (fontName.empty() || ResourceManager::Instance().GetFont(fontName)) {
			return;
		}

		if (fontName == "font1" || fontName == "font2") {
			const std::string resolvedPath = ResolveDefaultRuntimeFontPath();
			if (!resolvedPath.empty()) {
				ResourceManager::Instance().LoadFont(fontName, resolvedPath, 48);
			}
		}
	}

	/**
	 * @brief Appends warning.
	 * @param report Parameter for report.
	 * @param warning Parameter for warning.
	 */
	void AddWarning(RuntimeLevel::LevelValidationReport& report, std::string warning) {
		report.warnings.push_back(std::move(warning));
	}

	/**
	 * @brief Resolves existing path.
	 * @param rawPath Path to process.
	 * @param levelPath Parameter for level path.
	 * @param fallbackDir Parameter for fallback dir.
	 * @return Result produced by this operation.
	 */
	std::optional<fs::path> ResolveExistingPath(const std::string& rawPath, const fs::path& levelPath, const fs::path& fallbackDir = {}) {
		if (rawPath.empty()) {
			return std::nullopt;
		}

		std::vector<fs::path> candidates;
		candidates.emplace_back(rawPath);

		if (!levelPath.empty()) {
			candidates.push_back(levelPath.parent_path() / rawPath);
		}

		if (!fallbackDir.empty()) {
			candidates.push_back(fallbackDir / rawPath);
			candidates.push_back(fallbackDir / fs::path(rawPath).filename());
		}

		std::unordered_set<std::string> seen;
		for (const fs::path& candidate : candidates) {
			std::error_code ec;
			const fs::path normalized = candidate.lexically_normal();
			const std::string key = normalized.string();
			if (!seen.insert(key).second) {
				continue;
			}

			if (fs::exists(normalized, ec) && !ec) {
				return normalized;
			}
		}

		return std::nullopt;
	}

	/**
	 * @brief Validates referenced path.
	 * @param report Parameter for report.
	 * @param levelPath Parameter for level path.
	 * @param rawPath Path to process.
	 * @param fieldName Parameter for field name.
	 * @param owner Parameter for owner.
	 * @param fallbackDir Parameter for fallback dir.
	 */
	void ValidateReferencedPath(RuntimeLevel::LevelValidationReport& report,
		const fs::path& levelPath,
		const std::string& rawPath,
		const char* fieldName,
		const std::string& owner,
		const fs::path& fallbackDir = {}) {
		if (rawPath.empty()) {
			return;
		}

		if (!ResolveExistingPath(rawPath, levelPath, fallbackDir)) {
			AddWarning(report, owner + ": missing " + fieldName + " path '" + rawPath + "'.");
		}
	}

	/**
	 * @brief Validates audio key.
	 * @param report Parameter for report.
	 * @param audioKey Parameter for audio key.
	 * @param fieldName Parameter for field name.
	 * @param owner Parameter for owner.
	 */
	void ValidateAudioKey(RuntimeLevel::LevelValidationReport& report,
		const std::string& audioKey,
		const char* fieldName,
		const std::string& owner) {
		if (audioKey.empty()) {
			return;
		}

		if (!Audio::AudioCatalog::GetAudioAsset(audioKey)) {
			AddWarning(report, owner + ": unknown " + fieldName + " audio key '" + audioKey + "'.");
		}
	}

	/**
	 * @brief Validates level data.
	 * @param levelPath Parameter for level path.
	 * @param data Parameter for data.
	 * @return Result produced by this operation.
	 */
	RuntimeLevel::LevelValidationReport ValidateResolvedLevelData(const fs::path& levelPath, const LevelData& data) {
		RuntimeLevel::LevelValidationReport report;

		ValidateReferencedPath(report, levelPath, data.background, "background", "Level", "../assets");
		ValidateReferencedPath(report, levelPath, data.backgroundOverlay, "background overlay", "Level", "../assets");

		for (size_t index = 0; index < data.objects.size(); ++index) {
			const LevelObject& obj = data.objects[index];
			const std::string owner = "Object[" + std::to_string(index) + "]";

			if (obj.texture.empty() && obj.prefabPath.empty()) {
				AddWarning(report, owner + ": has neither a texture nor a prefab path.");
			}

			ValidateReferencedPath(report, levelPath, obj.texture, "texture", owner, "../assets");
			ValidateReferencedPath(report, levelPath, obj.prefabPath, "prefab", owner, "../prefabs");

			if (!IsFinite(obj.x) || !IsFinite(obj.y) || !IsFinite(obj.z) || !IsFinite(obj.w) || !IsFinite(obj.h) || !IsFinite(obj.rotation)) {
				AddWarning(report, owner + ": contains non-finite transform values.");
			}
			else if (obj.w <= 0.0f || obj.h <= 0.0f) {
				AddWarning(report, owner + ": has non-positive size (" + std::to_string(obj.w) + ", " + std::to_string(obj.h) + ").");
			}

			if (!IsFinite(obj.speedX) || !IsFinite(obj.speedY)) {
				AddWarning(report, owner + ": contains non-finite velocity values.");
			}

			if (obj.hasCollider) {
				if (!IsFinite(obj.colWidth) || !IsFinite(obj.colHeight) || !IsFinite(obj.colOffsetX) || !IsFinite(obj.colOffsetY)) {
					AddWarning(report, owner + ": contains non-finite collider values.");
				}
				else if (obj.colWidth <= 0.0f || obj.colHeight <= 0.0f) {
					AddWarning(report, owner + ": collider is enabled but has non-positive size (" + std::to_string(obj.colWidth) + ", " + std::to_string(obj.colHeight) + ").");
				}
			}

			ValidateAudioKey(report, obj.audioOnSpawn, "spawn", owner);
			ValidateAudioKey(report, obj.audioOnInteract, "interact", owner);
			ValidateAudioKey(report, obj.audioOnDestroy, "destroy", owner);
			ValidateAudioKey(report, obj.audioOnProcessing, "processing", owner);
		}

		std::unordered_set<std::string> seenTextNames;
		for (size_t index = 0; index < data.textObjects.size(); ++index) {
			const LevelTextObject& text = data.textObjects[index];
			const std::string owner = "TextObject[" + std::to_string(index) + "]";

			if (text.name.empty()) {
				AddWarning(report, owner + ": is missing a name.");
			}
			else if (!seenTextNames.insert(text.name).second) {
				AddWarning(report, owner + ": duplicates text object name '" + text.name + "'.");
			}

			if (text.text.empty()) {
				AddWarning(report, owner + ": has empty text content.");
			}

			if (text.fontName.empty()) {
				AddWarning(report, owner + ": is missing a font name.");
			}
			else if (text.fontName != "font1" && text.fontName != "font2" && !ResourceManager::Instance().GetFont(text.fontName)) {
				AddWarning(report, owner + ": font '" + text.fontName + "' is not loaded and may not render in runtime.");
			}

			if (!IsFinite(text.x) || !IsFinite(text.y) || !IsFinite(text.scale) || !IsFinite(text.rotation)) {
				AddWarning(report, owner + ": contains non-finite transform values.");
			}
			else if (text.scale <= 0.0f) {
				AddWarning(report, owner + ": has non-positive scale (" + std::to_string(text.scale) + ").");
			}

			if (!IsFinite(text.colorR) || !IsFinite(text.colorG) || !IsFinite(text.colorB) || !IsFinite(text.colorA)) {
				AddWarning(report, owner + ": contains non-finite color values.");
			}
			else if (text.colorR < 0.0f || text.colorR > 1.0f ||
				text.colorG < 0.0f || text.colorG > 1.0f ||
				text.colorB < 0.0f || text.colorB > 1.0f ||
				text.colorA < 0.0f || text.colorA > 1.0f) {
				AddWarning(report, owner + ": has color values outside the expected 0..1 range.");
			}
		}

		return report;
	}
}

namespace RuntimeLevel {

	/**
	 * @brief Validates level data.
	 * @param path Path to process.
	 * @param data Parameter for data.
	 * @return Result produced by this operation.
	 */
	LevelValidationReport ValidateLevelData(const std::string& path, const LevelData& data) {
		const std::optional<fs::path> resolvedLevelPath = ResolveFreshestLevelPath(path);
		return ValidateResolvedLevelData(resolvedLevelPath.value_or(fs::path(path)), data);
	}

	struct LevelManifest {
		std::vector<std::string> textures;
	};

	/**
	 * @brief Builds level manifest.
	 * @param data Parameter for data.
	 * @return Result produced by this operation.
	 */
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

	/**
	 * @brief Builds scene from level.
	 * @param levelIn Parameter for level in.
	 * @param scene Scene being processed.
	 */
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
					TS_LOG_ERROR("[RuntimeLevel] Spawn failed: " << obj.texture);
					continue;
				}

			}
			else {
				g = scene.SpawnStaticSprite(obj.texture, { obj.x, obj.y, 0.0f }, { obj.w, obj.h }, layerName);
				if (!g) {
					TS_LOG_ERROR("[RuntimeLevel] Spawn failed: " << obj.texture);
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

				// You can keep sizing/offset consistent; they wont render unless enabled.
				g->SetShadowSize(glm::vec2(obj.w * 0.8f, obj.h * 0.33f));
				g->SetShadowOffset(glm::vec2(0.0f, 55.0f));
				g->SetShadowOpacity(0.65f);
			}

			// Default metadata
			scene.SetTransformFromLevel(g->GetID(), { obj.x, obj.y, 0.0f }, { obj.w, obj.h, 1.0f }, obj.rotation);

			Scene::Defaults defs{};
			defs.pos = { obj.x, obj.y, 0.0f };
			defs.size = { obj.w, obj.h, 1.0f };
			defs.rot = obj.rotation;
			defs.colSize = { obj.colWidth, obj.colHeight };
			defs.colOff = { obj.colOffsetX, obj.colOffsetY };
			defs.vel = { obj.speedX, obj.speedY };
			defs.texture = obj.texture;
			defs.tag = obj.tag;
			// Persist the effective layer, including the fallback base layer for blank authoring values.
			defs.layer = layerName;
			defs.approachOffset = { obj.approachOffsetX, obj.approachOffsetY };
			defs.hasApproachOffset2 = obj.hasApproachOffset2;
			defs.approachOffset2 = { obj.approachOffset2X, obj.approachOffset2Y };
			defs.hasCustomerSeatOffset = obj.hasCustomerSeatOffset;
			defs.customerSeatOffset = { obj.customerSeatOffsetX, obj.customerSeatOffsetY };
			defs.customerSeatCapacity = obj.customerSeatCapacity;
			defs.hasCustomerSeatOffset2 = obj.hasCustomerSeatOffset2;
			defs.customerSeatOffset2 = { obj.customerSeatOffset2X, obj.customerSeatOffset2Y };
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

	/**
	 * @brief Loads and build.
	 * @param path Path to process.
	 * @param scene Scene being processed.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool LoadAndBuild(const std::string& path, Scene& scene) {
		const auto loadStart = std::chrono::steady_clock::now();
		LevelData data{};
		if (!LevelSerializer::Load(path, data)) {
			TS_LOG_ERROR("[RuntimeLevel] Failed to load level JSON: " << path);
			return false;
		}

		const LevelValidationReport validation = ValidateLevelData(path, data);
		if (validation.HasWarnings()) {
			TS_LOG_WARN("[RuntimeLevel] Validation warnings for '" << path << "' (" << validation.warnings.size() << "):");
			for (const std::string& warning : validation.warnings) {
				TS_LOG_WARN("  - " << warning);
			}
		}

		scene.ClearAll();

		// Set background if provided, otherwise keep current
		if (!data.background.empty()) {
			TS_LOG_DEBUG("[RuntimeLevel] Background set: " << data.background);
			scene.SetSceneBackground(data.background);
		}

		if (!data.backgroundOverlay.empty()) {
			TS_LOG_DEBUG("[RuntimeLevel] Background overlay set: " << data.backgroundOverlay);
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

		std::vector<RuntimeTextData> parsedTexts;
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
			d.visible = t.visible;

			// Runtime text owns its own font loading path and no longer depends on editor panel state.
			EnsureRuntimeFontLoaded(d.fontName);
		}

		scene.SetRuntimeTextObjects(parsedTexts);

#ifndef NDEBUG
		const double buildMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - buildStart).count();
		const double totalMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - loadStart).count();
		TS_LOG_DEBUG("[RuntimeLevel] LoadAndBuild '" << path << "': textures=" << manifest.textures.size()
			<< ", preload=" << preloadMs << " ms, build=" << buildMs << " ms, total=" << totalMs << " ms");
#endif

		return true;
	}
}
