#include "../Core/OrderUILogic.hpp"
#include "../Graphics/SceneManager.hpp"
#include "../Graphics/GameObject.hpp"
#include "../Core/LogicManager.hpp"
#include "../Core/CustomerTableLogic.hpp"
#include "../Core/SimpleNpcLogic.hpp"
#include "../Graphics/ResourceManager.hpp"

#include <algorithm> // sort (optional)

static void DespawnIfAlive(Scene& scene, int& id)
{
    if (id >= 0) { scene.DespawnByID(id); id = -1; }
}

const char* OrderUILogic::DishToIconPath(DishType t) const
{
    switch (t)
    {
    case DishType::VegDish:  return "../assets/Salad.png";
    case DishType::MeatDish: return "../assets/Meat.png";
    case DishType::SoupDish: return "../assets/Soup.png";
    case DishType::PoopDish: return "../assets/PoopDish.png";
    default:                 return "../assets/PoopDish.png";
    }
}

void OrderUILogic::Start(Scene& /*scene*/)
{
    slots_.clear();
    slots_.resize(kMaxOrders); //vector fixed-size = 4 slots
}

void OrderUILogic::OnDestroy(Scene& scene)
{
    for (auto& slot : slots_)
        ClearSlot(scene, slot);
}

glm::vec2 OrderUILogic::SlotTargetPos(int slotIndex) const
{
    return glm::vec2(
        panelTargetPos_.x + slotIndex * (panelSize_.y + ticketGapY_),
        panelTargetPos_.y
    );
}

void OrderUILogic::ClearSlot(Scene& scene, OrderSlot& slot)
{
    DespawnIfAlive(scene, slot.iconId);
    DespawnIfAlive(scene, slot.panelId);

    slot.customerId = -1;
    slot.panelSpawned = false;
    slot.lastDish = DishType::PoopDish;
    slot.hasLastDish = false;
}

void OrderUILogic::EnsurePanel(Scene& scene, int slotIndex, OrderSlot& slot)
{
    if (slot.panelSpawned && slot.panelId >= 0 && scene.GetGameObjectByID(slot.panelId))
        return;

    slot.panelId = scene.TriggerOrderUiSlideIn(
        SlotTargetPos(slotIndex),
        panelSize_,
        panelLayer_,
        panelTex_,
        slideDuration_
    );

    slot.panelSpawned = (slot.panelId >= 0);
}

void OrderUILogic::EnsureIcon(Scene& scene, OrderSlot& slot)
{
    if (slot.iconId >= 0 && scene.GetGameObjectByID(slot.iconId))
        return;

    GameObject* panel = scene.GetGameObjectByID(slot.panelId);
    if (!panel) return;

    glm::vec3 p = panel->GetPositionGLM();

    if (GameObject* icon = scene.SpawnStaticSprite(
        invisTex_,
        { p.x + iconOffset_.x, p.y + iconOffset_.y, p.z },
        iconSize_,
        iconLayer_))
    {
        slot.iconId = icon->GetID();
        icon->SetColliderSize(Math::Vector2D(0.f, 0.f));
        scene.SetObjectTexturePath(slot.iconId, invisTex_);
    }
}

void OrderUILogic::FollowPanel(Scene& scene, OrderSlot& slot)
{
    if (slot.panelId < 0 || slot.iconId < 0) return;

    GameObject* panel = scene.GetGameObjectByID(slot.panelId);
    GameObject* icon = scene.GetGameObjectByID(slot.iconId);
    if (!panel || !icon) return;

    glm::vec3 p = panel->GetPositionGLM();
    icon->SetPosition(Math::Vector3D(p.x + iconOffset_.x, p.y + iconOffset_.y, p.z));
}

void OrderUILogic::SetIconTexture(Scene& scene, int iconId, const char* texPath)
{
    GameObject* icon = scene.GetGameObjectByID(iconId);
    if (!icon) return;

    icon->SetTexture(ResourceManager::Instance().LoadTexture(texPath, texPath));
    scene.SetObjectTexturePath(iconId, texPath);

    Scene::Defaults d = scene.GetDefaults(iconId);
    d.texture = texPath;
    scene.SetDefaults(iconId, d);
}

int OrderUILogic::FindOrderIndexByCustomer(const std::vector<WaitingOrder>& orders, int customerId) const
{
    for (int i = 0; i < (int)orders.size(); ++i)
    {
        if (orders[i].customerId == customerId)
            return i;
    }
    return -1;
}

void OrderUILogic::CollectWaitingOrders(Scene& scene, std::vector<WaitingOrder>& outOrders)
{
    outOrders.clear();

    LogicManager& logicMgr = scene.GetLogicManager();

    for (GameObject* obj : scene.GetAllObjectsRaw())
    {
        if (!obj) continue;
        const int tableId = obj->GetID();

        auto* table = logicMgr.GetLogicForObject<CustomerTableLogic>(tableId);
        if (!table) continue;
        if (!table->HasSeatedCustomer()) continue;

        const int custId = table->GetSeatedCustomerID();
        auto* npc = logicMgr.GetLogicForObject<SimpleNpcLogic>(custId);
        if (!npc) continue;

        if (npc->IsWaitingForFood() && npc->HasOrderBeenTaken() && !npc->HasDishServed())
        {
            outOrders.push_back({ tableId, custId, npc->GetDesiredDishType() });
        }
    }

    // Optional: stable ordering (so new fills are consistent)
    std::sort(outOrders.begin(), outOrders.end(),
        [](const WaitingOrder& a, const WaitingOrder& b) { return a.tableId < b.tableId; });
}

void OrderUILogic::Update(float /*dt*/, Scene& scene, InputManager& /*input*/)
{
    std::vector<WaitingOrder> orders;
    orders.reserve(kMaxOrders);
    CollectWaitingOrders(scene, orders);

    // Track which orders are already displayed
    std::vector<bool> used(orders.size(), false);

    // 1) Keep existing customers in their slots if they’re still waiting
    for (int s = 0; s < (int)slots_.size(); ++s)
    {
        OrderSlot& slot = slots_[s];
        if (slot.customerId < 0) continue;

        const int idx = FindOrderIndexByCustomer(orders, slot.customerId);
        if (idx < 0)
        {
            // customer is no longer waiting -> clear slot
            ClearSlot(scene, slot);
            continue;
        }

        used[idx] = true;

        // Make sure panel/icon exists
        EnsurePanel(scene, s, slot);
        EnsureIcon(scene, slot);

        // Update dish icon if changed
        if (!slot.hasLastDish || slot.lastDish != orders[idx].dish)
        {
            SetIconTexture(scene, slot.iconId, DishToIconPath(orders[idx].dish));
            slot.lastDish = orders[idx].dish;
            slot.hasLastDish = true;
        }

        FollowPanel(scene, slot);
    }

    // 2) Fill empty slots with new waiting customers (up to 4 total)
    for (int i = 0; i < (int)orders.size(); ++i)
    {
        if (used[i]) continue;

        // find an empty slot
        int emptySlot = -1;
        for (int s = 0; s < (int)slots_.size(); ++s)
        {
            if (slots_[s].customerId < 0)
            {
                emptySlot = s;
                break;
            }
        }

        if (emptySlot < 0)
            break; // all slots filled (max 4)

        OrderSlot& slot = slots_[emptySlot];
        slot.customerId = orders[i].customerId;
        slot.lastDish = DishType::PoopDish;
        slot.hasLastDish = false;
        slot.panelSpawned = false;

        EnsurePanel(scene, emptySlot, slot);
        EnsureIcon(scene, slot);

        SetIconTexture(scene, slot.iconId, DishToIconPath(orders[i].dish));
        slot.lastDish = orders[i].dish;
        slot.hasLastDish = true;

        FollowPanel(scene, slot);
    }

    // 3) Any remaining empty slots are already cleared, nothing else needed
}
