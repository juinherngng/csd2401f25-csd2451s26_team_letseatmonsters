/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CustomerOrderUILogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (95%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (5%)

 DESCRIPTION:       Defines the CustomerOrderUILogic component, responsible for displaying
					and updating customer order UI elements such as the order bubble,
					patience bar, and payment result feedback attached to a customer.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "EngineCore/LogicManager.hpp"
#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/CustomerOrderUILogic.hpp"
#include "GameCore/OrderUILogic.hpp"
#include "GameCore/SimpleNpcLogic.hpp"
#include "MyoonchiDiner/GamePaths.hpp"

namespace {
	constexpr int kAmbientVfxRows = 14;
	constexpr int kAmbientVfxCols = 6;
	constexpr float kLowPatienceThreshold = 0.30f;
	constexpr float kLowPatienceGlowPulseSpeed = 7.8f;
	constexpr float kLowPatienceGlowMinAlpha = 0.72f;
	constexpr float kLowPatienceGlowMaxAlpha = 1.0f;
	constexpr float kLowPatienceBarPulseSpeed = 8.6f;
	constexpr float kLowPatienceBarPulseAmount = 0.22f;
	constexpr float kLowPatienceBarMinAlpha = 0.64f;
	constexpr float kLowPatienceBarMaxAlpha = 1.0f;
	constexpr float kLowPatienceShakeAmplitudeX = 3.2f;
	constexpr float kLowPatienceShakeAmplitudeY = 2.1f;
	constexpr float kLowPatienceShakeSpeedX = 8.5f;
	constexpr float kLowPatienceShakeSpeedY = 11.8f;
	constexpr float kLowPatienceGlowOffset = 5.5f;
	constexpr int kLowPatienceGlowSortOrder = 200;
	constexpr int kLowPatienceGlowMaskSortOrder = 201;
	const glm::vec4 kLowPatienceGlowTint(1.0f, 0.16f, 0.16f, kLowPatienceGlowMaxAlpha);

	/**
	 * @brief Builds UV frames from a top-indexed row of the ambient VFX sheet.
	 * @param topRowOneBased One-based row index counted from the top of the sheet.
	 * @param startCol First column to include.
	 * @param endCol Last column to include.
	 * @return UV rectangles for the requested frame range.
	 */
	std::vector<glm::vec4> CreateAmbientVfxFramesFromTopRow(int topRowOneBased, int startCol, int endCol) {
		// Return no frames when the requested row falls outside the authored sheet.
		std::vector<glm::vec4> frames;
		if (topRowOneBased < 1 || topRowOneBased > kAmbientVfxRows) {
			return frames;
		}

		startCol = std::max(0, std::min(startCol, kAmbientVfxCols - 1));
		endCol = std::max(0, std::min(endCol, kAmbientVfxCols - 1));
		if (startCol > endCol) {
			std::swap(startCol, endCol);
		}

		const int engineRow = kAmbientVfxRows - topRowOneBased;
		const float frameW = 1.0f / static_cast<float>(kAmbientVfxCols);
		const float frameH = 1.0f / static_cast<float>(kAmbientVfxRows);
		const float v = engineRow * frameH;

		for (int col = startCol; col <= endCol; ++col) {
			const float u = col * frameW;
			frames.emplace_back(u, v, frameW, frameH);
		}

		return frames;
	}
}

/**
 * @brief Despawns an object when its runtime ID is still valid.
 * @param scene Active scene containing the object.
 * @param id Runtime object ID to despawn and clear.
 */
static void DespawnIfAlive(Scene& scene, int& id) {
	// Clear the cached ID after despawn so follow-up checks stay safe.
	if (id >= 0) {
		scene.DespawnByID(id);
		id = -1;
	}
}

/**
 * @brief Clamps a scalar to the normalized range `[0, 1]`.
 * @param v Value to clamp.
 * @return Clamped normalized value.
 */
static float Clamp01(float v) {
	// Normalize helper values before they drive alpha, easing, or timing math.
	if (v < 0.f) return 0.f;
	if (v > 1.f) return 1.f;
	return v;
}

/**
 * @brief Applies an ease-out cubic curve to a normalized value.
 * @param x Input value in the range `[0, 1]`.
 * @return Eased output in the range `[0, 1]`.
 */
static float EaseOutCubic01(float x) {
	// Clamp first so overshoot never distorts the easing curve.
	x = Clamp01(x);
	float inv = 1.0f - x;
	return 1.0f - inv * inv * inv;
}

/**
 * @brief Produces a smooth interpolation weight for value-noise blending.
 * @param t Local interpolation factor.
 * @return Smoothed interpolation factor.
 */
static float SmoothNoiseBlend(float t) {
	// Use a smoothstep-style curve so adjacent noise cells blend without hard edges.
	t = Clamp01(t);
	return t * t * (3.0f - 2.0f * t);
}

/**
 * @brief Hashes an integer bit pattern into a pseudo-random value.
 * @param x Input bits to hash.
 * @return Mixed hash bits.
 */
static uint32_t HashNoiseBits(uint32_t x) {
	// Mix nearby inputs aggressively so the shake channels do not repeat obvious patterns.
	x ^= x >> 16;
	x *= 0x7feb352dU;
	x ^= x >> 15;
	x *= 0x846ca68bU;
	x ^= x >> 16;
	return x;
}

/**
 * @brief Converts a noise cell and seed into a signed pseudo-random scalar.
 * @param cell Integer noise cell.
 * @param seed Seed offset used to decorrelate channels.
 * @return Signed noise value in the range `[-1, 1]`.
 */
static float HashSignedNoise(int cell, int seed) {
	// Combine the cell and seed before hashing so each channel gets a distinct pattern.
	const uint32_t cellBits = static_cast<uint32_t>(cell);
	const uint32_t seedBits = static_cast<uint32_t>(seed) * 0x9e3779b9U;
	const uint32_t hashed = HashNoiseBits(cellBits ^ seedBits);
	const float normalized = static_cast<float>(hashed) / static_cast<float>(UINT32_MAX);
	return normalized * 2.0f - 1.0f;
}

/**
 * @brief Samples one octave of smoothed 1D value noise.
 * @param x Continuous sample position.
 * @param seed Seed offset used to decorrelate channels.
 * @return Signed noise sample.
 */
static float SampleValueNoise1D(float x, int seed) {
	// Interpolate between the neighboring cell values that bound the sample point.
	const int cell0 = static_cast<int>(std::floor(x));
	const int cell1 = cell0 + 1;
	const float localT = x - static_cast<float>(cell0);
	const float blend = SmoothNoiseBlend(localT);
	const float value0 = HashSignedNoise(cell0, seed);
	const float value1 = HashSignedNoise(cell1, seed);
	return value0 + (value1 - value0) * blend;
}

/**
 * @brief Samples a short fractal stack of 1D noise.
 * @param x Continuous sample position.
 * @param seed Seed offset used to decorrelate channels.
 * @return Weighted multi-octave noise sample.
 */
static float SampleFractalNoise1D(float x, int seed) {
	// Blend several octaves together so the low-patience shake feels organic instead of robotic.
	float sum = 0.0f;
	float totalWeight = 0.0f;
	float amplitude = 1.0f;
	float frequency = 1.0f;

	for (int octave = 0; octave < 3; ++octave) {
		sum += SampleValueNoise1D(x * frequency, seed + octave * 131) * amplitude;
		totalWeight += amplitude;
		amplitude *= 0.5f;
		frequency *= 2.17f;
	}

	if (totalWeight <= 0.0f) {
		return 0.0f;
	}

	return sum / totalWeight;
}

/**
 * @brief Raises the magnitude of a value while preserving its sign.
 * @param value Signed input value.
 * @param exponent Exponent applied to the magnitude.
 * @return Sign-preserving powered result.
 */
static float SignedPow(float value, float exponent) {
	// Shape noise intensity without losing the original movement direction.
	const float magnitude = std::pow(std::abs(value), exponent);
	return std::copysign(magnitude, value);
}

/**
 * @brief Initializes transient UI and VFX state for the customer.
 * @param scene Unused active scene reference.
 */
void CustomerOrderUILogic::Start(Scene& /*scene*/) {
	// Reset every transient field so a reused logic instance starts from a clean slate.
	lastIconPath_.clear();
	prevBehaviourState_ = -1;
	lowPatienceGlow_ = {};
	lowPatienceGlowTimer_ = 0.0f;
	lowPatienceGlowAlpha_ = 0.0f;
	lowPatienceBarScaleMul_ = 1.0f;
	lowPatienceBarAlpha_ = 1.0f;
	lowPatienceBarOffset_ = { 0.0f, 0.0f };

	payVFX_ID_ = -1;
	payVFXTimer_ = 0.0f;
	payVFXMode_ = PayVfxMode::FloatUp;
	payVFXStartPos_ = { 0.f, 0.f };
	payVFXTargetPos_ = { 0.f, 0.f };
	payVFXQueueStamp_ = false;
	eatingVFX_ID_ = -1;
}

/**
 * @brief Destroys every spawned helper object owned by this customer UI.
 * @param scene Active scene containing the helper objects.
 */
void CustomerOrderUILogic::OnDestroy(Scene& scene) {
	// When the customer despawns, clean up their attached UI.
	DestroyBubble(scene);
	DestroyPatienceBar(scene);
	DestroyPaymentVFX(scene);
	DestroyEatingVFX(scene);
}

/**
 * @brief Destroys the currently spawned order-bubble sprites.
 * @param scene Active scene containing the bubble sprites.
 */
void CustomerOrderUILogic::DestroyBubble(Scene& scene) {
	// Remove the icon before the background so the bubble fully tears down in one pass.
	DespawnIfAlive(scene, bubbleDish_ID_);
	DespawnIfAlive(scene, bubbleBG_ID_);
}

/**
 * @brief Destroys the patience-bar sprites and warning glow.
 * @param scene Active scene containing the patience-bar objects.
 */
void CustomerOrderUILogic::DestroyPatienceBar(Scene& scene) {
	// Tear down the glow first because it mirrors the bar background's transform.
	DestroyLowPatienceGlow(scene);
	DespawnIfAlive(scene, barFill_ID_);
	DespawnIfAlive(scene, barBG_ID_);
}

/**
 * @brief Replaces the current bubble icon texture and refreshes its size.
 * @param scene Active scene containing the icon sprite.
 * @param iconPath Texture path for the replacement icon.
 */
void CustomerOrderUILogic::UpdateIconTexture(Scene& scene, const char* iconPath) {
	GameObject* icon = scene.GetGameObjectByID(bubbleDish_ID_);
	if (!icon) return;

	// Keep both the runtime texture and serialized defaults aligned with the latest prompt icon.
	icon->SetTexture(ResourceManager::Instance().LoadTexture(iconPath, iconPath));
	scene.SetObjectTexturePath(bubbleDish_ID_, iconPath);

	Scene::Defaults d = scene.GetDefaults(bubbleDish_ID_);
	d.texture = iconPath;
	scene.SetDefaults(bubbleDish_ID_, d);

	// Resize the icon so payment prompts and food prompts keep their authored proportions.
	glm::vec2 iconSize = GetIconSizeForPath(iconPath);
	icon->SetScale(glm::vec3(iconSize.x, iconSize.y, 1.0f));

	lastIconPath_ = iconPath;
}

/**
 * @brief Ensures the patience bar exists for the owning customer.
 * @param scene Active scene containing the customer.
 */
void CustomerOrderUILogic::EnsurePatienceBar(Scene& scene) {
	GameObject* me = scene.GetGameObjectByID(GetOwnerID());
	if (!me) return;

	// Spawn any missing patience widgets at the customer's current anchor position.
	glm::vec3 p = me->GetPositionGLM();

	if (barBG_ID_ < 0) {
		if (GameObject* bg = scene.SpawnStaticSprite(
			patienceBGPath_,
			{ p.x + barOffset_.x, p.y + barOffset_.y, p.z },
			barBGSize_,
			uiLayerBG_)) {
			barBG_ID_ = bg->GetID();
			bg->SetColliderSize(Math::Vector2D(0.f, 0.f));
			scene.SetObjectTexturePath(barBG_ID_, patienceBGPath_);
		}
	}

	if (barFill_ID_ < 0) {
		if (GameObject* fill = scene.SpawnStaticSprite(
			patienceFillPath_,
			{ p.x + barOffset_.x, p.y + barOffset_.y, p.z },
			barFillSize_,
			uiLayerTop_)) {
			barFill_ID_ = fill->GetID();
			fill->SetColliderSize(Math::Vector2D(0.f, 0.f));
			scene.SetObjectTexturePath(barFill_ID_, patienceFillPath_);
		}
	}
}

/**
 * @brief Keeps all spawned UI and VFX aligned to the customer.
 * @param scene Active scene containing the customer and helper objects.
 */
void CustomerOrderUILogic::FollowCustomer(Scene& scene) {
	GameObject* me = scene.GetGameObjectByID(GetOwnerID());
	if (!me) return;

	// Re-anchor every helper object from the customer's latest world position.
	glm::vec3 p = me->GetPositionGLM();

	if (bubbleBG_ID_ >= 0) {
		if (GameObject* bg = scene.GetGameObjectByID(bubbleBG_ID_)) {
			bg->SetPosition(Math::Vector3D(p.x + bubbleOffset_.x, p.y + bubbleOffset_.y, p.z));
		}
	}

	if (bubbleDish_ID_ >= 0) {
		if (GameObject* icon = scene.GetGameObjectByID(bubbleDish_ID_)) {
			icon->SetPosition(Math::Vector3D(p.x + dishOffset_.x, p.y + dishOffset_.y, p.z));
		}
	}

	if (barBG_ID_ >= 0) {
		if (GameObject* bg = scene.GetGameObjectByID(barBG_ID_)) {
			bg->SetPosition(Math::Vector3D(
				p.x + barOffset_.x + lowPatienceBarOffset_.x,
				p.y + barOffset_.y + lowPatienceBarOffset_.y,
				p.z));
			bg->SetScale(glm::vec3(
				barBGSize_.x * lowPatienceBarScaleMul_,
				barBGSize_.y * lowPatienceBarScaleMul_,
				1.0f));
			bg->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, lowPatienceBarAlpha_));
		}
	}

	if (lowPatienceGlow_.sourceID >= 0) {
		SyncLowPatienceGlow(scene, lowPatienceGlowAlpha_);
	}

	if (eatingVFX_ID_ >= 0) {
		if (GameObject* eatingVfx = scene.GetGameObjectByID(eatingVFX_ID_)) {
			const glm::vec2 eatingOffset = GetEatingVfxOffset(scene);
			eatingVfx->SetPosition(Math::Vector3D(
				p.x + eatingOffset.x,
				p.y + eatingOffset.y,
				p.z + 0.001f));
		}
	}

	// Leave the fill bar to UpdatePatienceFill() so its left edge stays pinned correctly.
}

/**
 * @brief Rescales the patience fill bar using the supplied normalized ratio.
 * @param scene Active scene containing the patience-bar objects.
 * @param ratio01 Normalized patience value in the range `[0, 1]`.
 */
void CustomerOrderUILogic::UpdatePatienceFill(Scene& scene, float ratio01) {
	if (barBG_ID_ < 0 || barFill_ID_ < 0) return;

	GameObject* bg = scene.GetGameObjectByID(barBG_ID_);
	GameObject* fill = scene.GetGameObjectByID(barFill_ID_);
	if (!bg || !fill) return;

	ratio01 = Clamp01(ratio01);

	glm::vec3 bgPos = bg->GetPositionGLM();

	// Recompute the fill from its left edge so depletion always collapses toward the right.
	const float fullW = barFillSize_.x * lowPatienceBarScaleMul_;
	const float fullH = barFillSize_.y * lowPatienceBarScaleMul_;

	float newW = fullW * ratio01;
	if (newW < 0.f) newW = 0.f;

	float leftX = bgPos.x - (fullW * 0.5f);
	float centerX = leftX + (newW * 0.5f);

	fill->SetScale({ newW, fullH, 1.0f });
	fill->SetPosition(Math::Vector3D(centerX, bgPos.y, bgPos.z));
	fill->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, lowPatienceBarAlpha_));
}

/**
 * @brief Destroys the active eating VFX object.
 * @param scene Active scene containing the VFX object.
 */
void CustomerOrderUILogic::DestroyEatingVFX(Scene& scene) {
	// Collapse the helper state to "inactive" by despawning and clearing the cached ID.
	DespawnIfAlive(scene, eatingVFX_ID_);
}

/**
 * @brief Returns the best eating-VFX offset for the customer's current animation.
 * @param scene Active scene used to query the current animation.
 * @return Offset to apply to the eating VFX.
 */
glm::vec2 CustomerOrderUILogic::GetEatingVfxOffset(Scene& scene) const {
	// Match the particle placement to the current eating-facing animation when possible.
	const std::string animName = scene.GetCurrentAnimationName(GetOwnerID());
	if (animName == "EAT_LEFT") {
		return eatingVFXOffsetLeft_;
	}
	if (animName == "EAT_RIGHT") {
		return eatingVFXOffsetRight_;
	}
	return eatingVFXFallbackOffset_;
}

/**
 * @brief Ensures the looping eating VFX exists while the customer is eating.
 * @param scene Active scene containing the customer and VFX objects.
 */
void CustomerOrderUILogic::EnsureEatingVFX(Scene& scene) {
	// Reuse the current VFX object when it is still alive instead of respawning it every frame.
	if (eatingVFX_ID_ >= 0 && scene.GetGameObjectByID(eatingVFX_ID_)) {
		return;
	}

	GameObject* me = scene.GetGameObjectByID(GetOwnerID());
	if (!me) {
		return;
	}

	// Pull the authored ambient-bite frames from the shared VFX sheet.
	std::vector<glm::vec4> frames = CreateAmbientVfxFramesFromTopRow(5, 0, 4);
	if (frames.empty()) {
		return;
	}

	const glm::vec3 p = me->GetPositionGLM();
	const glm::vec2 eatingOffset = GetEatingVfxOffset(scene);
	const std::string layer = scene.GetObjectLayer(GetOwnerID()).empty()
		? std::string("10")
		: scene.GetObjectLayer(GetOwnerID());

	GameObject* fx = scene.SpawnAnimatedSprite(
		MyoonchiPaths::Textures::AMBIENT_VFX_SHEET,
		glm::vec3(p.x + eatingOffset.x, p.y + eatingOffset.y, p.z + 0.001f),
		eatingVFXSize_,
		frames,
		0.10f,
		true,
		layer);

	if (!fx) {
		return;
	}

	eatingVFX_ID_ = fx->GetID();
	// Configure the VFX as a pure visual child with no gameplay collision or physics behavior.
	fx->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
	fx->SetColliderOffset(Math::Vector2D(0.0f, 0.0f));
	fx->SetMovableByPhysics(false);
	fx->EnableShadow(false);
	fx->SetRenderSortOrder(std::max(me->GetRenderSortOrder() + 1, 5));
	scene.SetObjectTag(eatingVFX_ID_, "customer_eating_vfx");
}

/**
 * @brief Updates the active eating VFX position, scale, and layer.
 * @param scene Active scene containing the customer and VFX objects.
 */
void CustomerOrderUILogic::UpdateEatingVFX(Scene& scene) {
	// If either object vanished unexpectedly, tear the helper down cleanly.
	if (eatingVFX_ID_ < 0) {
		return;
	}

	GameObject* me = scene.GetGameObjectByID(GetOwnerID());
	GameObject* fx = scene.GetGameObjectByID(eatingVFX_ID_);
	if (!me || !fx) {
		DestroyEatingVFX(scene);
		return;
	}

	// Keep the VFX visually attached to the customer and above the customer sprite.
	const glm::vec3 p = me->GetPositionGLM();
	const glm::vec2 eatingOffset = GetEatingVfxOffset(scene);
	const std::string layer = scene.GetObjectLayer(GetOwnerID()).empty()
		? std::string("10")
		: scene.GetObjectLayer(GetOwnerID());

	fx->SetPosition(glm::vec3(
		p.x + eatingOffset.x,
		p.y + eatingOffset.y,
		p.z + 0.001f));
	fx->SetScale(glm::vec3(eatingVFXSize_.x, eatingVFXSize_.y, 1.0f));
	fx->SetRenderSortOrder(std::max(me->GetRenderSortOrder() + 1, 5));
	scene.AssignObjectToLayer(eatingVFX_ID_, layer);
}

/**
 * @brief Destroys the low-patience glow sprites and resets the glow source.
 * @param scene Active scene containing the glow sprites.
 */
void CustomerOrderUILogic::DestroyLowPatienceGlow(Scene& scene) {
	// Remove every outline sprite and the center mask in one pass.
	for (int& id : lowPatienceGlow_.ids) {
		DespawnIfAlive(scene, id);
	}
	lowPatienceGlow_.sourceID = -1;
}

/**
 * @brief Ensures the low-patience glow overlay exists around the patience bar.
 * @param scene Active scene containing the bar sprites.
 */
void CustomerOrderUILogic::EnsureLowPatienceGlow(Scene& scene) {
	// Abort if the source bar does not exist, because the glow simply mirrors that sprite.
	if (barBG_ID_ < 0) {
		DestroyLowPatienceGlow(scene);
		return;
	}

	GameObject* source = scene.GetGameObjectByID(barBG_ID_);
	if (!source) {
		DestroyLowPatienceGlow(scene);
		return;
	}

	const std::string texturePath = scene.GetObjectTexturePath(barBG_ID_);
	if (texturePath.empty()) {
		DestroyLowPatienceGlow(scene);
		return;
	}

	if (lowPatienceGlow_.sourceID == barBG_ID_) {
		return;
	}

	DestroyLowPatienceGlow(scene);

	// Spawn outline copies around the bar to simulate a pulsing warning glow.
	const std::string layer = scene.GetObjectLayer(barBG_ID_);
	const glm::vec3 sourcePos = source->GetPositionGLM();
	const glm::vec3 sourceScale = source->GetScaleGLM();
	const float sourceRotation = source->GetRotation();

	const float diagonalOffset = kLowPatienceGlowOffset * 0.72f;
	const std::array<glm::vec2, 8> offsets{
		glm::vec2(-kLowPatienceGlowOffset, 0.0f),
		glm::vec2(kLowPatienceGlowOffset, 0.0f),
		glm::vec2(0.0f, -kLowPatienceGlowOffset),
		glm::vec2(0.0f,  kLowPatienceGlowOffset),
		glm::vec2(-diagonalOffset, -diagonalOffset),
		glm::vec2(diagonalOffset, -diagonalOffset),
		glm::vec2(-diagonalOffset, diagonalOffset),
		glm::vec2(diagonalOffset, diagonalOffset)
	};

	for (std::size_t i = 0; i < offsets.size(); ++i) {
		const glm::vec2& offset = offsets[i];
		GameObject* outline = scene.SpawnStaticSprite(
			texturePath,
			glm::vec3(sourcePos.x + offset.x, sourcePos.y + offset.y, sourcePos.z),
			glm::vec2(std::abs(sourceScale.x), std::abs(sourceScale.y)),
			layer);

		if (!outline) {
			continue;
		}

		// Reuse the hover-outline shader so the bar gets a crisp additive warning edge.
		outline->SetColorTint(kLowPatienceGlowTint);
		outline->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
		outline->SetMovableByPhysics(false);
		outline->EnableShadow(false);
		outline->SetRenderSortOrder(kLowPatienceGlowSortOrder);
		outline->SetRotation(sourceRotation, glm::vec3(0.0f, 0.0f, 1.0f));

		if (Shader* outlineShader = ResourceManager::Instance().GetShader("hover_outline")) {
			outline->SetShader(outlineShader);
		}

		lowPatienceGlow_.ids[i] = outline->GetID();
	}

	// Spawn a center mask so the warning reads as an outline rather than a solid duplicate.
	GameObject* mask = scene.SpawnStaticSprite(
		texturePath,
		sourcePos,
		glm::vec2(std::abs(sourceScale.x), std::abs(sourceScale.y)),
		layer);

	if (mask) {
		mask->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		mask->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
		mask->SetMovableByPhysics(false);
		mask->EnableShadow(false);
		mask->SetRenderSortOrder(kLowPatienceGlowMaskSortOrder);
		mask->SetRotation(sourceRotation, glm::vec3(0.0f, 0.0f, 1.0f));
		lowPatienceGlow_.ids[8] = mask->GetID();
	}

	lowPatienceGlow_.sourceID = barBG_ID_;
}

/**
 * @brief Synchronizes the low-patience glow sprites to the current bar transform.
 * @param scene Active scene containing the glow and bar sprites.
 * @param alpha Alpha value to apply to the outline sprites.
 */
void CustomerOrderUILogic::SyncLowPatienceGlow(Scene& scene, float alpha) {
	// Follow the source bar exactly so pulse and shake remain visually locked together.
	if (lowPatienceGlow_.sourceID < 0) {
		return;
	}

	GameObject* source = scene.GetGameObjectByID(lowPatienceGlow_.sourceID);
	if (!source) {
		DestroyLowPatienceGlow(scene);
		return;
	}

	const std::string layer = scene.GetObjectLayer(lowPatienceGlow_.sourceID);
	const glm::vec3 sourcePos = source->GetPositionGLM();
	const glm::vec3 sourceScale = source->GetScaleGLM();
	const float sourceRotation = source->GetRotation();
	const float clampedAlpha = Clamp01(alpha);

	const float diagonalOffset = kLowPatienceGlowOffset * 0.72f;
	const std::array<glm::vec2, 8> offsets{
		glm::vec2(-kLowPatienceGlowOffset, 0.0f),
		glm::vec2(kLowPatienceGlowOffset, 0.0f),
		glm::vec2(0.0f, -kLowPatienceGlowOffset),
		glm::vec2(0.0f,  kLowPatienceGlowOffset),
		glm::vec2(-diagonalOffset, -diagonalOffset),
		glm::vec2(diagonalOffset, -diagonalOffset),
		glm::vec2(-diagonalOffset, diagonalOffset),
		glm::vec2(diagonalOffset, diagonalOffset)
	};

	// Update every outline copy around the source bar.
	for (std::size_t i = 0; i < offsets.size(); ++i) {
		const int id = lowPatienceGlow_.ids[i];
		if (id < 0) {
			continue;
		}

		GameObject* outline = scene.GetGameObjectByID(id);
		if (!outline) {
			continue;
		}

		const glm::vec2& offset = offsets[i];
		outline->SetPosition(glm::vec3(sourcePos.x + offset.x, sourcePos.y + offset.y, sourcePos.z));
		outline->SetScale(glm::vec3(std::abs(sourceScale.x), std::abs(sourceScale.y), 1.0f));
		outline->SetRotation(sourceRotation, glm::vec3(0.0f, 0.0f, 1.0f));
		outline->SetColorTint(glm::vec4(
			kLowPatienceGlowTint.r,
			kLowPatienceGlowTint.g,
			kLowPatienceGlowTint.b,
			clampedAlpha));
		outline->SetRenderSortOrder(kLowPatienceGlowSortOrder);
		scene.AssignObjectToLayer(id, layer);
	}

	// Keep the center mask aligned so the glow appears as an outer ring only.
	const int maskID = lowPatienceGlow_.ids[8];
	if (maskID >= 0) {
		if (GameObject* mask = scene.GetGameObjectByID(maskID)) {
			mask->SetPosition(sourcePos);
			mask->SetScale(glm::vec3(std::abs(sourceScale.x), std::abs(sourceScale.y), 1.0f));
			mask->SetRotation(sourceRotation, glm::vec3(0.0f, 0.0f, 1.0f));
			mask->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			mask->SetRenderSortOrder(kLowPatienceGlowMaskSortOrder);
			scene.AssignObjectToLayer(maskID, layer);
		}
	}
}

/**
 * @brief Drives the pulsing low-patience warning animation.
 * @param scene Active scene containing the bar and glow sprites.
 * @param dt Delta time for the frame.
 * @param ratio01 Normalized patience value in the range `[0, 1]`.
 * @param showBar True when the patience bar is visible this frame.
 */
void CustomerOrderUILogic::UpdateLowPatienceWarning(Scene& scene, float dt, float ratio01, bool showBar) {
	// Reset all warning state immediately when the bar is hidden or patience is healthy.
	const bool shouldWarn = showBar && ratio01 <= kLowPatienceThreshold;
	if (!shouldWarn) {
		lowPatienceGlowTimer_ = 0.0f;
		lowPatienceGlowAlpha_ = 0.0f;
		lowPatienceBarScaleMul_ = 1.0f;
		lowPatienceBarAlpha_ = 1.0f;
		lowPatienceBarOffset_ = { 0.0f, 0.0f };
		DestroyLowPatienceGlow(scene);
		return;
	}

	// Build pulse, alpha, and shake parameters from how deep into the warning zone the customer is.
	lowPatienceGlowTimer_ += dt;
	EnsureLowPatienceGlow(scene);

	const float warning01 = Clamp01((kLowPatienceThreshold - ratio01) / kLowPatienceThreshold);
	const float trauma = 0.42f + 0.58f * warning01;
	const float shakeStrength = trauma * trauma;

	const float pulse01 = 0.5f + 0.5f * std::sin(lowPatienceGlowTimer_ * kLowPatienceGlowPulseSpeed + warning01 * 0.85f);
	lowPatienceGlowAlpha_ = kLowPatienceGlowMinAlpha +
		(kLowPatienceGlowMaxAlpha - kLowPatienceGlowMinAlpha) * pulse01;

	const float barPulse01 = 0.25f + 0.75f * std::pow(
		0.5f + 0.5f * std::sin(lowPatienceGlowTimer_ * kLowPatienceBarPulseSpeed + 0.55f),
		1.85f);
	lowPatienceBarScaleMul_ = 1.0f +
		(barPulse01 * kLowPatienceBarPulseAmount * (0.45f + 0.55f * shakeStrength));
	lowPatienceBarAlpha_ = kLowPatienceBarMinAlpha +
		(kLowPatienceBarMaxAlpha - kLowPatienceBarMinAlpha) * barPulse01;

	const int seedBase = GetOwnerID() * 97 + 131;
	const float noiseX = SignedPow(
		SampleFractalNoise1D(lowPatienceGlowTimer_ * kLowPatienceShakeSpeedX, seedBase + 11),
		0.78f);
	const float noiseY = SignedPow(
		SampleFractalNoise1D(lowPatienceGlowTimer_ * kLowPatienceShakeSpeedY, seedBase + 53),
		1.12f);
	const float burstNoise = std::max(
		0.0f,
		SampleFractalNoise1D(lowPatienceGlowTimer_ * 4.6f, seedBase + 97));
	const float xAmp = kLowPatienceShakeAmplitudeX * (0.55f + shakeStrength + burstNoise * 0.35f);
	const float yAmp = kLowPatienceShakeAmplitudeY * (0.45f + shakeStrength * 0.95f);
	lowPatienceBarOffset_ = glm::vec2(
		noiseX * xAmp * (noiseX >= 0.0f ? 1.0f : 0.72f),
		noiseY * yAmp * (noiseY >= 0.0f ? 0.78f : 1.18f));
}

/**
 * @brief Updates the order bubble, patience bar, and reaction VFX for the owning customer.
 * @param dt Delta time for the frame.
 * @param scene Active scene containing the customer and UI objects.
 * @param input Unused input manager reference.
 */
void CustomerOrderUILogic::Update(float dt, Scene& scene, InputManager& /*input*/) {
	// The customer UI only advances while gameplay simulation is actively running.
	if (!scene.IsSimulationActive())
		return;

	LogicManager& logicMgr = scene.GetLogicManager();
	auto* npcLogic = logicMgr.GetLogicForObject<SimpleNpcLogic>(GetOwnerID());
	if (!npcLogic)
		return;

	auto state = npcLogic->GetBehaviourState();
	const int curStateInt = (int)state;

	const int leavingInt = (int)SimpleNpcLogic::BehaviourState::Leaving;
	const int eatingInt = (int)SimpleNpcLogic::BehaviourState::Eating;

	// Detect state transitions so one-shot success and failure reactions only fire once.
	const bool enteredLeaving = (prevBehaviourState_ != leavingInt && curStateInt == leavingInt);
	const bool enteredEating = (prevBehaviourState_ != eatingInt && curStateInt == eatingInt);

	// Launch the success stamp animation when the customer starts eating a correctly served dish.
	if (enteredEating && !npcLogic->WillPayZero() && npcLogic->HasDishServed()) {
		TriggerOrderCompleteSuccess(scene);
	}

	// Reserve the sad face for failed service cases that end in a zero payment.
	if (enteredLeaving && npcLogic->WillPayZero()) {
		SpawnPaymentVFX(scene, sadFacePath_);
	}

	// Decide which UI elements should be active for the customer's current behavior state.
	const bool showBubble =
		(state == SimpleNpcLogic::BehaviourState::WaitingForFood) ||
		(state == SimpleNpcLogic::BehaviourState::Paying && !npcLogic->WillPayZero());

	const bool showBar =
		(state == SimpleNpcLogic::BehaviourState::WaitingForFood);
	const bool showEatingVfx =
		(state == SimpleNpcLogic::BehaviourState::Eating);
	float patienceRatio = 1.0f;

	if (showBubble) {
		if (state == SimpleNpcLogic::BehaviourState::Paying) {
			// Swap to the payment icon once the customer is ready to settle the bill.
			EnsureBubbleIcon(scene, coinIconPath_);
		}
		else {
			// While waiting for food, show the dish icon the customer still wants.
			DishType wanted = npcLogic->GetDesiredDishType();
			EnsureBubbleIcon(scene, DishToIconPath(wanted));
		}
	}
	else {
		DestroyBubble(scene);
	}

	if (showEatingVfx) {
		EnsureEatingVFX(scene);
	}
	else {
		DestroyEatingVFX(scene);
	}

	if (showBar) {
		EnsurePatienceBar(scene);
		patienceRatio = npcLogic->GetPatienceRatio01();
		UpdateLowPatienceWarning(scene, dt, patienceRatio, true);
	}
	else {
		DestroyPatienceBar(scene);
		UpdateLowPatienceWarning(scene, dt, 1.0f, false);
	}

	// Re-anchor all helper objects after the visibility decisions above.
	FollowCustomer(scene);

	if (showBar) {
		UpdatePatienceFill(scene, patienceRatio);
	}

	if (showEatingVfx) {
		UpdateEatingVFX(scene);
	}

	// Advance payment reaction motion after the main state update.
	UpdatePaymentVFX(scene, dt);

	// Remember the latest state so next frame can detect fresh transitions cleanly.
	prevBehaviourState_ = curStateInt;
}

/**
 * @brief Ensures the order bubble background and icon exist with the requested texture.
 * @param scene Active scene containing the customer and bubble sprites.
 * @param iconPath Texture path for the desired icon.
 */
void CustomerOrderUILogic::EnsureBubbleIcon(Scene& scene, const char* iconPath) {
	GameObject* me = scene.GetGameObjectByID(GetOwnerID());
	if (!me) return;

	// Spawn the bubble widgets lazily at the customer's current position.
	glm::vec3 p = me->GetPositionGLM();

	// Create the background bubble first so the icon has a backing card.
	if (bubbleBG_ID_ < 0) {
		if (GameObject* bg = scene.SpawnStaticSprite(
			bubbleBGPath_,
			{ p.x + bubbleOffset_.x, p.y + bubbleOffset_.y, p.z },
			bubbleSize_,
			uiLayerBG_)) {
			bubbleBG_ID_ = bg->GetID();
			bg->SetColliderSize(Math::Vector2D(0.f, 0.f));
			scene.SetObjectTexturePath(bubbleBG_ID_, bubbleBGPath_);
		}
	}

	// Create the icon sprite when it does not exist yet.
	if (bubbleDish_ID_ < 0) {
		glm::vec2 iconSize = GetIconSizeForPath(iconPath);

		if (GameObject* icon = scene.SpawnStaticSprite(
			iconPath,
			{ p.x + dishOffset_.x, p.y + dishOffset_.y, p.z },
			iconSize,
			uiLayerTop_)) {
			bubbleDish_ID_ = icon->GetID();
			icon->SetColliderSize(Math::Vector2D(0.f, 0.f));
			scene.SetObjectTexturePath(bubbleDish_ID_, iconPath);
			lastIconPath_ = iconPath;
		}
	}


	// Refresh the icon only when the desired prompt changed between frames.
	if (bubbleDish_ID_ >= 0 && lastIconPath_ != iconPath) {
		UpdateIconTexture(scene, iconPath);
	}
}

/**
 * @brief Destroys the current payment VFX and resets its runtime state.
 * @param scene Active scene containing the payment VFX object.
 */
void CustomerOrderUILogic::DestroyPaymentVFX(Scene& scene) {
	// Reset both the object ID and the flight state so the next spawn starts fresh.
	DespawnIfAlive(scene, payVFX_ID_);
	payVFXTimer_ = 0.0f;
	payVFXMode_ = PayVfxMode::FloatUp;
	payVFXStartPos_ = { 0.f, 0.f };
	payVFXTargetPos_ = { 0.f, 0.f };
	payVFXQueueStamp_ = false;
}

/**
 * @brief Spawns a temporary payment reaction sprite above the customer.
 * @param scene Active scene containing the customer.
 * @param path Texture path for the VFX sprite to spawn.
 */
void CustomerOrderUILogic::SpawnPaymentVFX(Scene& scene, const char* path) {
	// Replace any existing payment reaction before spawning the new one.
	DestroyPaymentVFX(scene);

	GameObject* me = scene.GetGameObjectByID(GetOwnerID());
	if (!me) return;

	// Spawn the reaction at the authored offset above the customer's head.
	glm::vec3 p = me->GetPositionGLM();

	if (GameObject* vfx = scene.SpawnStaticSprite(
		path,
		{ p.x + payVFXOffset_.x, p.y + payVFXOffset_.y, p.z },
		payVFXSize_,
		uiLayerTop_)) {
		payVFX_ID_ = vfx->GetID();
		vfx->SetColliderSize(Math::Vector2D(0.f, 0.f));
		scene.SetObjectTexturePath(payVFX_ID_, path);
		payVFXTimer_ = 0.0f;

		// Default to the simpler float-up animation unless the caller upgrades it later.
		payVFXMode_ = PayVfxMode::FloatUp;
		payVFXQueueStamp_ = false;

		glm::vec3 start = vfx->GetPositionGLM();
		payVFXStartPos_ = { start.x, start.y };
		payVFXTargetPos_ = payVFXStartPos_;
	}
}

/**
 * @brief Converts a successful order completion into the fly-to-panel reaction.
 * @param scene Active scene containing the customer and order panel.
 */
void CustomerOrderUILogic::TriggerOrderCompleteSuccess(Scene& scene) {
	// Only launch the fly-to-panel effect when the order UI can resolve a target panel.
	glm::vec2 targetPanelPos{};
	if (!OrderUILogic::TryGetPanelCenterForCustomer(GetOwnerID(), targetPanelPos)) {
		return;
	}

	SpawnPaymentVFX(scene, happyFacePath_);

	GameObject* vfx = scene.GetGameObjectByID(payVFX_ID_);
	if (!vfx) {
		return;
	}

	// Promote the spawned VFX into the fly-to-order animation mode.
	glm::vec3 start = vfx->GetPositionGLM();
	payVFXStartPos_ = { start.x, start.y };
	payVFXTargetPos_ = targetPanelPos;
	payVFXTimer_ = 0.0f;
	payVFXMode_ = PayVfxMode::FlyToOrder;
	payVFXQueueStamp_ = true;
}

/**
 * @brief Updates payment VFX motion and lifetime for the current frame.
 * @param scene Active scene containing the payment VFX object.
 * @param dt Delta time for the frame.
 */
void CustomerOrderUILogic::UpdatePaymentVFX(Scene& scene, float dt) {
	// Skip cleanly when there is no active payment VFX to animate.
	if (payVFX_ID_ < 0) return;

	GameObject* vfx = scene.GetGameObjectByID(payVFX_ID_);
	if (!vfx) {
		payVFX_ID_ = -1;
		return;
	}

	payVFXTimer_ += dt;

	if (payVFXMode_ == PayVfxMode::FlyToOrder) {
		// Ease the success icon toward the order panel with a lightweight upward arc.
		const float t = Clamp01(payVFXTimer_ / std::max(0.001f, payVFXFlyDuration_));
		const float eased = EaseOutCubic01(t);

		glm::vec2 pos = payVFXStartPos_ + (payVFXTargetPos_ - payVFXStartPos_) * eased;

		// Add a small hop so the success icon feels more celebratory than linear.
		pos.y -= std::sin(t * 3.14159265f) * 26.0f;

		vfx->SetPosition(Math::Vector3D(pos.x, pos.y, vfx->GetPositionGLM().z));

		const float scaleMul = 0.90f + 0.10f * std::sin(t * 3.14159265f * 0.5f);
		vfx->SetScale(glm::vec3(
			payVFXSize_.x * scaleMul,
			payVFXSize_.y * scaleMul,
			1.0f));

		if (t >= 1.0f) {
			// Request the completion stamp once the icon actually reaches the order panel.
			if (payVFXQueueStamp_) {
				OrderUILogic::RequestCompletionStampForCustomer(GetOwnerID());
			}
			DestroyPaymentVFX(scene);
		}
		return;
	}

	// Fall back to the original float-up reaction for failure and generic feedback.
	glm::vec3 pos = vfx->GetPositionGLM();
	pos.y -= payVFXRiseSpeed_ * dt;
	vfx->SetPosition(Math::Vector3D(pos.x, pos.y, pos.z));

	if (payVFXTimer_ >= payVFXDuration_) {
		DestroyPaymentVFX(scene);
	}
}

/**
 * @brief Maps a dish type to the icon texture used in the customer's thought bubble.
 * @param dish Dish type requested by the customer.
 * @return Texture path for the matching icon.
 */
const char* CustomerOrderUILogic::DishToIconPath(DishType dish) const {
	// Keep dish-to-icon mapping centralized so order prompts stay consistent across the UI.
	switch (dish) {
	case DishType::VegDish:  return "../assets/Food/Food_Salad.png";
	case DishType::MeatDish: return "../assets/Food/Food_Meat.png";
	case DishType::SoupDish: return "../assets/Food/Food_Mushroom.png";
	case DishType::SkewerDish: return "../assets/Food/Food_Meat_n_carrot.png";
	case DishType::CarrotSaladDish: return "../assets/Food/Food_Salad_n_carrot.png";
	case DishType::PoopDish:
	default: return "../assets/Food/poop.png";
	}
}

/**
 * @brief Returns the authored icon size for the requested prompt texture.
 * @param iconPath Texture path for the icon being displayed.
 * @return Icon size in world units.
 */
glm::vec2 CustomerOrderUILogic::GetIconSizeForPath(const char* iconPath) const {
	// Coins intentionally render smaller than dish icons to fit the payment prompt bubble.
	if (!iconPath) return dishIconSize_;
	const std::string path(iconPath);
	if (path == coinIconPath_) return coinIconSize_;
	if (path == "../assets/Food/Food_Salad.png" ||
		path == "../assets/Food/Food_Meat.png" ||
		path == "../assets/Food/Food_Mushroom.png") {
		// Match the same relative reduction used by the smaller order-ticket dish icons.
		const float reducedDishScale = 50.0f / 60.0f;
		return glm::vec2(dishIconSize_.x * reducedDishScale, dishIconSize_.y * reducedDishScale);
	}
	return dishIconSize_;
}
