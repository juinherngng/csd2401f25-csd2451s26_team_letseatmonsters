#pragma once
#include "../Core/GameObjectLogic.hpp"
#include "../Core/FoodTypes.hpp"
#include <glm/glm.hpp>
#include <string>
#include "../Core/Math.hpp"
#include <vector>

class Scene;
class InputManager;

class OrderUILogic : public GameObjectLogic
{
public:
    using GameObjectLogic::GameObjectLogic;

    void Start(Scene& scene) override;
    void Update(float dt, Scene& scene, InputManager& input) override;
    void OnDestroy(Scene& scene) override;

    std::string GetName() const override { return "OrderUILogic"; }

private:
    static constexpr int kMaxOrders = 4;

    struct WaitingOrder
    {
        int tableId = -1;
        int customerId = -1;
        DishType dish = DishType::PoopDish;
    };

    struct OrderSlot
    {
        int customerId = -1;

        int panelId = -1;
        int iconId = -1;

        bool panelSpawned = false;
        DishType lastDish = DishType::PoopDish;
        bool hasLastDish = false;
    };

    std::vector<OrderSlot> slots_;

    // Config
    const char* panelTex_ = "../assets/Order_UI.png";
    const char* invisTex_ = "../assets/invis.png";
    std::string panelLayer_ = "3";
    std::string iconLayer_ = "4";

    glm::vec2 panelTargetPos_ = { 460.f, 64.f };
    glm::vec2 panelSize_ = { 168.f, 124.f };
    float slideDuration_ = 0.45f;

    // You WILL need to tweak these to align the dish inside the panel
    glm::vec2 iconOffset_ = { 0.f, 0.f };
    glm::vec2 iconSize_ = { 64.f, 64.f };

    float ticketGapY_ = 120.f; // space between tickets

    // Collect all currently-waiting orders (not just one)
    void CollectWaitingOrders(Scene& scene, std::vector<WaitingOrder>& outOrders);

    // Slot helpers
    glm::vec2 SlotTargetPos(int slotIndex) const;
    void ClearSlot(Scene& scene, OrderSlot& slot);

    // Spawn/maintain UI
    void EnsurePanel(Scene& scene, int slotIndex, OrderSlot& slot);
    void EnsureIcon(Scene& scene, OrderSlot& slot);
    void FollowPanel(Scene& scene, OrderSlot& slot);

    // Texture updates
    const char* DishToIconPath(DishType t) const;
    void SetIconTexture(Scene& scene, int iconId, const char* texPath);

    // Utility
    int FindOrderIndexByCustomer(const std::vector<WaitingOrder>& orders, int customerId) const;
};
