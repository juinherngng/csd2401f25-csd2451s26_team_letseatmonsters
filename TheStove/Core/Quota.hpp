#pragma once

class Scene;

namespace Economy
{
    // Global money the player currently has
    inline int gPlayerMoney = 0;

    // Win quota
    inline constexpr int kQuota = 300;

    // How much a correct dish pays (tweak anytime)
    inline constexpr int kCorrectDishPay = 50;

    // Prevent triggering win multiple times
    inline bool gQuotaReached = false;

    inline void Reset()
    {
        gPlayerMoney = 0;
        gQuotaReached = false;
    }

    // TODO: you will implement this later (switch scene, show win UI, etc.)
    inline void OnQuotaReached(Scene& scene)
    {
        (void)scene;
        // Example later:
        // scene.RequestStateChange(WIN_STATE_ID);
    }

    inline void AddMoney(Scene& scene, int amount)
    {
        if (amount <= 0)
            return;

        gPlayerMoney += amount;

        if (!gQuotaReached && gPlayerMoney >= kQuota)
        {
            gQuotaReached = true;
            OnQuotaReached(scene);
        }
    }
}
