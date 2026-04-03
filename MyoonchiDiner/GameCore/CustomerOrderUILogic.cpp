/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CustomerOrderUILogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Defines the CustomerOrderUILogic component, responsible for displaying
					and updating customer order UI elements such as the order bubble,
					patience bar, and payment result feedback attached to a customer.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <array>
#include <cmath>

#include "EngineCore/LogicManager.hpp"
#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/CustomerOrderUILogic.hpp"
#include "GameCore/OrderUILogic.hpp"
#include "GameCore/SimpleNpcLogic.hpp"

namespace {
	constexpr float kLowPatienceThreshold = 0.30f;
	constexpr float kLowPatienceGlowPulseSpeed = 9.0f;
	constexpr float kLowPatienceGlowMinAlpha = 0.32f;
	constexpr float kLowPatienceGlowMaxAlpha = 0.92f;
	constexpr float kLowPatienceGlowOffset = 2.5f;
	constexpr int kLowPatienceGlowSortOrder = 200;
	constexpr int kLowPatienceGlowMaskSortOrder = 201;
	const glm::vec4 kLowPatienceGlowTint(1.0f, 0.16f, 0.16f, kLowPatienceGlowMaxAlpha);
}

 // small utility
static void DespawnIfAlive(Scene& scene, int& id) {
	if (id >= 0) {
		scene.DespawnByID(id);
		id = -1;
	}
}

static float Clamp01(float v) {
	if (v < 0.f) return 0.f;
	if (v > 1.f) return 1.f;
	return v;
}

static float EaseOutCubic01(float x) {
	x = Clamp01(x);
	float inv = 1.0f - x;
	return 1.0f - inv * inv * inv;
}

void CustomerOrderUILogic::Start(Scene& /*scene*/) {
	lastIconPath_.clear();
	prevBehaviourState_ = -1;
	lowPatienceGlow_ = {};
	lowPatienceGlowTimer_ = 0.0f;

	payVFX_ID_ = -1;
	payVFXTimer_ = 0.0f;
	payVFXMode_ = PayVfxMode::FloatUp;
	payVFXStartPos_ = { 0.f, 0.f };
	payVFXTargetPos_ = { 0.f, 0.f };
	payVFXQueueStamp_ = false;
}

void CustomerOrderUILogic::OnDestroy(Scene& scene) {
	// When the customer despawns, clean up their attached UI.
	DestroyBubble(scene);
	DestroyPatienceBar(scene);
	DestroyPaymentVFX(scene);
}

void CustomerOrderUILogic::DestroyBubble(Scene& scene) {
	DespawnIfAlive(scene, bubbleDish_ID_);
	DespawnIfAlive(scene, bubbleBG_ID_);
}

void CustomerOrderUILogic::DestroyPatienceBar(Scene& scene) {
	DestroyLowPatienceGlow(scene);
	DespawnIfAlive(scene, barFill_ID_);
	DespawnIfAlive(scene, barBG_ID_);
}

void CustomerOrderUILogic::UpdateIconTexture(Scene& scene, const char* iconPath) {
	GameObject* icon = scene.GetGameObjectByID(bubbleDish_ID_);
	if (!icon) return;

	icon->SetTexture(ResourceManager::Instance().LoadTexture(iconPath, iconPath));
	scene.SetObjectTexturePath(bubbleDish_ID_, iconPath);

	Scene::Defaults d = scene.GetDefaults(bubbleDish_ID_);
	d.texture = iconPath;
	scene.SetDefaults(bubbleDish_ID_, d);

	//set size depending on icon type
	glm::vec2 iconSize = GetIconSizeForPath(iconPath);
	icon->SetScale(glm::vec3(iconSize.x, iconSize.y, 1.0f));

	lastIconPath_ = iconPath;
}

void CustomerOrderUILogic::EnsurePatienceBar(Scene& scene) {
	GameObject* me = scene.GetGameObjectByID(GetOwnerID());
	if (!me) return;

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

void CustomerOrderUILogic::FollowCustomer(Scene& scene) {
	GameObject* me = scene.GetGameObjectByID(GetOwnerID());
	if (!me) return;

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
			bg->SetPosition(Math::Vector3D(p.x + barOffset_.x, p.y + barOffset_.y, p.z));
		}
	}

	// barFill position is handled in UpdatePatienceFill() so it pivots correctly
}

void CustomerOrderUILogic::UpdatePatienceFill(Scene& scene, float ratio01) {
	if (barBG_ID_ < 0 || barFill_ID_ < 0) return;

	GameObject* bg = scene.GetGameObjectByID(barBG_ID_);
	GameObject* fill = scene.GetGameObjectByID(barFill_ID_);
	if (!bg || !fill) return;

	ratio01 = Clamp01(ratio01);

	glm::vec3 bgPos = bg->GetPositionGLM();

	// We want fill to shrink RIGHT -> LEFT while LEFT edge stays fixed.
	// Assuming sprite position is CENTER-based (typical), we:
	//  1) compute left edge world X
	//  2) set new width
	//  3) set fill center to left + newWidth/2
	const float fullW = barFillSize_.x;
	const float fullH = barFillSize_.y;

	float newW = fullW * ratio01;
	if (newW < 0.f) newW = 0.f;

	float leftX = bgPos.x - (fullW * 0.5f);
	float centerX = leftX + (newW * 0.5f);

	fill->SetScale({ newW, fullH, 1.0f });
	fill->SetPosition(Math::Vector3D(centerX, bgPos.y, bgPos.z));
}

void CustomerOrderUILogic::DestroyLowPatienceGlow(Scene& scene) {
	for (int& id : lowPatienceGlow_.ids) {
		DespawnIfAlive(scene, id);
	}
	lowPatienceGlow_.sourceID = -1;
}

void CustomerOrderUILogic::EnsureLowPatienceGlow(Scene& scene) {
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

	const std::string layer = scene.GetObjectLayer(barBG_ID_);
	const glm::vec3 sourcePos = source->GetPositionGLM();
	const glm::vec3 sourceScale = source->GetScaleGLM();
	const float sourceRotation = source->GetRotation();

	const std::array<glm::vec2, 4> offsets{
		glm::vec2(-kLowPatienceGlowOffset, 0.0f),
		glm::vec2(kLowPatienceGlowOffset, 0.0f),
		glm::vec2(0.0f, -kLowPatienceGlowOffset),
		glm::vec2(0.0f,  kLowPatienceGlowOffset)
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
		lowPatienceGlow_.ids[4] = mask->GetID();
	}

	lowPatienceGlow_.sourceID = barBG_ID_;
}

void CustomerOrderUILogic::SyncLowPatienceGlow(Scene& scene, float alpha) {
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

	const std::array<glm::vec2, 4> offsets{
		glm::vec2(-kLowPatienceGlowOffset, 0.0f),
		glm::vec2(kLowPatienceGlowOffset, 0.0f),
		glm::vec2(0.0f, -kLowPatienceGlowOffset),
		glm::vec2(0.0f,  kLowPatienceGlowOffset)
	};

	for (std::size_t i = 0; i < 4; ++i) {
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

	const int maskID = lowPatienceGlow_.ids[4];
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

void CustomerOrderUILogic::UpdateLowPatienceWarning(Scene& scene, float dt, float ratio01, bool showBar) {
	const bool shouldWarn = showBar && ratio01 <= kLowPatienceThreshold;
	if (!shouldWarn) {
		lowPatienceGlowTimer_ = 0.0f;
		DestroyLowPatienceGlow(scene);
		return;
	}

	lowPatienceGlowTimer_ += dt;
	EnsureLowPatienceGlow(scene);

	const float pulse01 = 0.5f + 0.5f * std::sin(lowPatienceGlowTimer_ * kLowPatienceGlowPulseSpeed);
	const float alpha = kLowPatienceGlowMinAlpha +
		(kLowPatienceGlowMaxAlpha - kLowPatienceGlowMinAlpha) * pulse01;

	SyncLowPatienceGlow(scene, alpha);
}

/**
 * @brief Updates the order bubble, patience bar, and reaction VFX for the owning customer.
 * @param dt Delta time for the frame.
 * @param scene Active scene containing the customer and UI objects.
 * @param input Unused input manager reference.
 */
void CustomerOrderUILogic::Update(float dt, Scene& scene, InputManager& /*input*/) {
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

	// Detect state transitions so one-shot success/failure reactions only trigger once.
	const bool enteredLeaving = (prevBehaviourState_ != leavingInt && curStateInt == leavingInt);
	const bool enteredEating = (prevBehaviourState_ != eatingInt && curStateInt == eatingInt);

	// correct dish just got completed -> launch happy face from customer to order card
	if (enteredEating && !npcLogic->WillPayZero() && npcLogic->HasDishServed()) {
		TriggerOrderCompleteSuccess(scene);
	}

	// only keep sad face for failure cases
	if (enteredLeaving && npcLogic->WillPayZero()) {
		SpawnPaymentVFX(scene, sadFacePath_);
	}

	// Bubble shown only in WaitingForFood or Paying
	const bool showBubble =
		(state == SimpleNpcLogic::BehaviourState::WaitingForFood) ||
		(state == SimpleNpcLogic::BehaviourState::Paying && !npcLogic->WillPayZero());

	const bool showBar =
		(state == SimpleNpcLogic::BehaviourState::WaitingForFood);

	if (showBubble) {
		if (state == SimpleNpcLogic::BehaviourState::Paying) {
			// Always coin prompt during Paying (interaction hint)
			EnsureBubbleIcon(scene, coinIconPath_);
		}
		else {
			// WaitingForFood shows requested dish
			DishType wanted = npcLogic->GetDesiredDishType();
			EnsureBubbleIcon(scene, DishToIconPath(wanted));
		}
	}
	else {
		DestroyBubble(scene);
	}

	if (showBar) {
		EnsurePatienceBar(scene);
	}
	else {
		DestroyPatienceBar(scene);
	}

	// keep UI following the customer every frame
	FollowCustomer(scene);

	if (showBar) {
		const float ratio = npcLogic->GetPatienceRatio01();
		UpdatePatienceFill(scene, ratio);
		UpdateLowPatienceWarning(scene, dt, ratio, showBar);
	}
	else {
		UpdateLowPatienceWarning(scene, dt, 1.0f, false);
	}

	// Update the payment VFX lifetime / motion
	UpdatePaymentVFX(scene, dt);

	// store previous state for transition detection
	prevBehaviourState_ = curStateInt;
}

void CustomerOrderUILogic::EnsureBubbleIcon(Scene& scene, const char* iconPath) {
	GameObject* me = scene.GetGameObjectByID(GetOwnerID());
	if (!me) return;

	glm::vec3 p = me->GetPositionGLM();

	// Ensure bubble BG exists
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

	// Ensure icon exists
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


	// If icon path changed (dish <-> coin), update texture
	if (bubbleDish_ID_ >= 0 && lastIconPath_ != iconPath) {
		UpdateIconTexture(scene, iconPath);
	}
}

void CustomerOrderUILogic::DestroyPaymentVFX(Scene& scene) {
	DespawnIfAlive(scene, payVFX_ID_);
	payVFXTimer_ = 0.0f;
	payVFXMode_ = PayVfxMode::FloatUp;
	payVFXStartPos_ = { 0.f, 0.f };
	payVFXTargetPos_ = { 0.f, 0.f };
	payVFXQueueStamp_ = false;
}

void CustomerOrderUILogic::SpawnPaymentVFX(Scene& scene, const char* path) {
	DestroyPaymentVFX(scene);

	GameObject* me = scene.GetGameObjectByID(GetOwnerID());
	if (!me) return;

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

		payVFXMode_ = PayVfxMode::FloatUp;
		payVFXQueueStamp_ = false;

		glm::vec3 start = vfx->GetPositionGLM();
		payVFXStartPos_ = { start.x, start.y };
		payVFXTargetPos_ = payVFXStartPos_;
	}
}

void CustomerOrderUILogic::TriggerOrderCompleteSuccess(Scene& scene) {
	glm::vec2 targetPanelPos{};
	if (!OrderUILogic::TryGetPanelCenterForCustomer(GetOwnerID(), targetPanelPos)) {
		return;
	}

	SpawnPaymentVFX(scene, happyFacePath_);

	GameObject* vfx = scene.GetGameObjectByID(payVFX_ID_);
	if (!vfx) {
		return;
	}

	glm::vec3 start = vfx->GetPositionGLM();
	payVFXStartPos_ = { start.x, start.y };
	payVFXTargetPos_ = targetPanelPos;
	payVFXTimer_ = 0.0f;
	payVFXMode_ = PayVfxMode::FlyToOrder;
	payVFXQueueStamp_ = true;
}

void CustomerOrderUILogic::UpdatePaymentVFX(Scene& scene, float dt) {
	if (payVFX_ID_ < 0) return;

	GameObject* vfx = scene.GetGameObjectByID(payVFX_ID_);
	if (!vfx) {
		payVFX_ID_ = -1;
		return;
	}

	payVFXTimer_ += dt;

	if (payVFXMode_ == PayVfxMode::FlyToOrder) {
		const float t = Clamp01(payVFXTimer_ / std::max(0.001f, payVFXFlyDuration_));
		const float eased = EaseOutCubic01(t);

		glm::vec2 pos = payVFXStartPos_ + (payVFXTargetPos_ - payVFXStartPos_) * eased;

		// little arc upward during flight
		pos.y -= std::sin(t * 3.14159265f) * 26.0f;

		vfx->SetPosition(Math::Vector3D(pos.x, pos.y, vfx->GetPositionGLM().z));

		const float scaleMul = 0.90f + 0.10f * std::sin(t * 3.14159265f * 0.5f);
		vfx->SetScale(glm::vec3(
			payVFXSize_.x * scaleMul,
			payVFXSize_.y * scaleMul,
			1.0f));

		if (t >= 1.0f) {
			if (payVFXQueueStamp_) {
				OrderUILogic::RequestCompletionStampForCustomer(GetOwnerID());
			}
			DestroyPaymentVFX(scene);
		}
		return;
	}

	// old float-up behavior
	glm::vec3 pos = vfx->GetPositionGLM();
	pos.y -= payVFXRiseSpeed_ * dt;
	vfx->SetPosition(Math::Vector3D(pos.x, pos.y, pos.z));

	if (payVFXTimer_ >= payVFXDuration_) {
		DestroyPaymentVFX(scene);
	}
}

const char* CustomerOrderUILogic::DishToIconPath(DishType dish) const {
	switch (dish) {
	case DishType::VegDish:  return "../assets/Food/Salad.png";
	case DishType::MeatDish: return "../assets/Food/Meat.png";
	case DishType::SoupDish: return "../assets/Food/Soup.png";
	case DishType::SkewerDish: return "../assets/Food/Food_Meat_n_carrot.png";
	case DishType::CarrotSaladDish: return "../assets/Food/Food_Salad_n_carrot.png";
	case DishType::PoopDish:
	default: return "../assets/Food/poop.png";
	}
}

glm::vec2 CustomerOrderUILogic::GetIconSizeForPath(const char* iconPath) const {
	if (!iconPath) return dishIconSize_;
	// coin should be small, everything else (dish icons) big
	if (std::string(iconPath) == coinIconPath_) return coinIconSize_;
	return dishIconSize_;
}
