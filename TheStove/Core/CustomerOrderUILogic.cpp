#include "CustomerOrderUILogic.hpp"

#include "../Graphics/SceneManager.hpp"
#include "../Core/LogicManager.hpp"
#include "../Core/SimpleNpcLogic.hpp"
#include "../Graphics/GameObject.hpp"

// small utility
static void DespawnIfAlive(Scene& scene, int& id)
{
    if (id >= 0) {
        scene.DespawnByID(id);
        id = -1;
    }
}

static float Clamp01(float v)
{
    if (v < 0.f) return 0.f;
    if (v > 1.f) return 1.f;
    return v;
}

const char* CustomerOrderUILogic::DishToIconPath(DishType t) const
{
    // Using same textures as your plate visuals (works immediately).
    // If you have separate UI icons, swap paths here.
    switch (t)
    {
    case DishType::VegDish:  return "../assets/Salad.png";
    case DishType::MeatDish: return "../assets/Meat.png";
    case DishType::SoupDish: return "../assets/Soup.png";
    case DishType::PoopDish: return "../assets/PoopDish.png";
    default:                 return "../assets/PoopDish.png";
    }
}

void CustomerOrderUILogic::Start(Scene& /*scene*/)
{
    lastIconPath_.clear();
    prevBehaviourState_ = -1;

    payVFX_ID_ = -1;
    payVFXTimer_ = 0.0f;
}

void CustomerOrderUILogic::OnDestroy(Scene& scene)
{
    // When the customer despawns, clean up their attached UI.
    DestroyBubble(scene);
    DestroyPatienceBar(scene);
    DestroyPaymentVFX(scene);
}

void CustomerOrderUILogic::DestroyBubble(Scene& scene)
{
    DespawnIfAlive(scene, bubbleDish_ID_);
    DespawnIfAlive(scene, bubbleBG_ID_);
}

void CustomerOrderUILogic::DestroyPatienceBar(Scene& scene)
{
    DespawnIfAlive(scene, barFill_ID_);
    DespawnIfAlive(scene, barBG_ID_);
}

void CustomerOrderUILogic::EnsureBubble(Scene& scene, DishType dish)
{
    const char* iconPath = DishToIconPath(dish);
    EnsureBubbleIcon(scene, iconPath);
}

void CustomerOrderUILogic::UpdateIconTexture(Scene& scene, const char* iconPath)
{
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

void CustomerOrderUILogic::EnsurePatienceBar(Scene& scene)
{
    GameObject* me = scene.GetGameObjectByID(GetOwnerID());
    if (!me) return;

    glm::vec3 p = me->GetPositionGLM();

    if (barBG_ID_ < 0) {
        if (GameObject* bg = scene.SpawnStaticSprite(
            patienceBGPath_,
            { p.x + barOffset_.x, p.y + barOffset_.y, p.z },
            barBGSize_,
            uiLayerBG_))
        {
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
            uiLayerTop_))
        {
            barFill_ID_ = fill->GetID();
            fill->SetColliderSize(Math::Vector2D(0.f, 0.f));
            scene.SetObjectTexturePath(barFill_ID_, patienceFillPath_);
        }
    }
}

void CustomerOrderUILogic::FollowCustomer(Scene& scene)
{
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

void CustomerOrderUILogic::UpdatePatienceFill(Scene& scene, float ratio01)
{
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

void CustomerOrderUILogic::Update(float dt, Scene& scene, InputManager& /*input*/)
{
    if (!scene.IsSimulationActive())
        return;

    LogicManager& logicMgr = scene.GetLogicManager();
    auto* npcLogic = logicMgr.GetLogicForObject<SimpleNpcLogic>(GetOwnerID());
    if (!npcLogic)
        return;

    auto state = npcLogic->GetBehaviourState();
    const int curStateInt = (int)state;

    // Spawn result VFX exactly when payment is taken (Paying -> Leaving)
    if (prevBehaviourState_ == (int)SimpleNpcLogic::BehaviourState::Paying &&
        curStateInt == (int)SimpleNpcLogic::BehaviourState::Leaving)
    {
        // pay $0 => sad, pay money => happy
        const char* face = npcLogic->WillPayZero() ? sadFacePath_ : happyFacePath_;
        SpawnPaymentVFX(scene, face);
    }

    // Bubble shown only in WaitingForFood or Paying
    const bool showBubble =
        (state == SimpleNpcLogic::BehaviourState::WaitingForFood ||
            state == SimpleNpcLogic::BehaviourState::Paying);

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
            EnsureBubble(scene, wanted);
        }
    }
    else {
        DestroyBubble(scene);
    }

    if (showBar) {
        EnsurePatienceBar(scene);
        float ratio = npcLogic->GetPatienceRatio01();
        UpdatePatienceFill(scene, ratio);
    }
    else {
        DestroyPatienceBar(scene);
    }

    // Update the payment VFX lifetime / motion
    UpdatePaymentVFX(scene, dt);

    // keep UI following the customer every frame
    FollowCustomer(scene);

    // store previous state for transition detection
    prevBehaviourState_ = curStateInt;
}


void CustomerOrderUILogic::EnsureBubbleIcon(Scene& scene, const char* iconPath)
{
    GameObject* me = scene.GetGameObjectByID(GetOwnerID());
    if (!me) return;

    glm::vec3 p = me->GetPositionGLM();

    // Ensure bubble BG exists
    if (bubbleBG_ID_ < 0) {
        if (GameObject* bg = scene.SpawnStaticSprite(
            bubbleBGPath_,
            { p.x + bubbleOffset_.x, p.y + bubbleOffset_.y, p.z },
            bubbleSize_,
            uiLayerBG_))
        {
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
            uiLayerTop_))
        {
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


void CustomerOrderUILogic::DestroyPaymentVFX(Scene& scene)
{
    DespawnIfAlive(scene, payVFX_ID_);
    payVFXTimer_ = 0.0f;
}

void CustomerOrderUILogic::SpawnPaymentVFX(Scene& scene, const char* path)
{
    // If one is still alive, replace it
    DestroyPaymentVFX(scene);

    GameObject* me = scene.GetGameObjectByID(GetOwnerID());
    if (!me) return;

    glm::vec3 p = me->GetPositionGLM();

    if (GameObject* vfx = scene.SpawnStaticSprite(
        path,
        { p.x + payVFXOffset_.x, p.y + payVFXOffset_.y, p.z },
        payVFXSize_,
        uiLayerTop_))
    {
        payVFX_ID_ = vfx->GetID();
        vfx->SetColliderSize(Math::Vector2D(0.f, 0.f));
        scene.SetObjectTexturePath(payVFX_ID_, path);
        payVFXTimer_ = 0.0f;
    }
}

void CustomerOrderUILogic::UpdatePaymentVFX(Scene& scene, float dt)
{
    if (payVFX_ID_ < 0) return;

    GameObject* vfx = scene.GetGameObjectByID(payVFX_ID_);
    if (!vfx) {
        payVFX_ID_ = -1;
        return;
    }

    payVFXTimer_ += dt;

    // float upward a bit
    glm::vec3 pos = vfx->GetPositionGLM();
    pos.y -= payVFXRiseSpeed_ * dt; // y negative = up in your project
    vfx->SetPosition(Math::Vector3D(pos.x, pos.y, pos.z));

    if (payVFXTimer_ >= payVFXDuration_) {
        DestroyPaymentVFX(scene);
    }
}

glm::vec2 CustomerOrderUILogic::GetIconSizeForPath(const char* iconPath) const
{
    if (!iconPath) return dishIconSize_;
    // coin should be small, everything else (dish icons) big
    if (std::string(iconPath) == coinIconPath_) return coinIconSize_;
    return dishIconSize_;
}
