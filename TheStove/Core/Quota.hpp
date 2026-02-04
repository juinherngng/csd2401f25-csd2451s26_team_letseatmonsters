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

    // Total time allowed (seconds). Adjust as you like.
    inline constexpr float kTimeLimitSeconds = 120.0f;

    // Remaining time (seconds)
    inline float gTimeRemaining = kTimeLimitSeconds;

    // Prevent triggering lose multiple times
    inline bool gTimeUp = false;

    // Optional: pause timer (e.g., in win/lose screen)
    inline bool gTimerPaused = false;

    inline void Reset()
    {
        gPlayerMoney = 0;
        gQuotaReached = false;

        gTimeRemaining = kTimeLimitSeconds;
        gTimeUp = false;
        gTimerPaused = false;
    }

    // TODO: you will implement this later (switch scene, show win UI, etc.)
    inline void OnQuotaReached(Scene& scene)
    {
        (void)scene;
        // Example later:
        // scene.RequestStateChange(WIN_STATE_ID);
    }

    // TODO: implement this later (switch scene to lose screen, show lose UI, etc.)
    inline void OnTimeUp(Scene& scene)
    {
        (void)scene;
        // Example later:
        // scene.RequestStateChange(LOSE_STATE_ID);
        //
        // Or whatever your engine uses for switching scene/state.
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

    inline void Update(float dt, Scene& scene)
    {
        if (gQuotaReached) return;      // optional: stop timer if win
        if (gTimeUp) return;            // already time-up
        if (gTimerPaused) return;       // paused

        if (dt <= 0.0f) return;

        gTimeRemaining -= dt;

        if (gTimeRemaining <= 0.0f)
        {
            gTimeRemaining = 0.0f;
            gTimeUp = true;
            OnTimeUp(scene);
        }
    }

    // Helpers (optional)
    inline float GetTimeRemaining() { return gTimeRemaining; }
}
