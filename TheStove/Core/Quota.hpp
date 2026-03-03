/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         Quota.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu   (90%)
 CO-AUTHOR:         Ng Juin Herng, juinherng.ng@digipen.edu (10%)

 DESCRIPTION:       Defines the Economy namespace, which tracks player money,
                    win quota, and remaining time, and synchronizes these values
                    with the game UI and win/lose conditions.

         All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "LevelEditorPanelFonts.hpp"

class Scene;
class AudioManager;

namespace Economy
{
    // Global money the player currently has
    inline int gPlayerMoney = 0;

    // Win quota
    inline constexpr int kQuota = 200;

    // How much a correct dish pays (tweak anytime)
    inline constexpr int kCorrectDishPay = 50;

    // Prevent triggering win multiple times
    inline bool gQuotaReached = false;

    // Total time allowed (seconds). Adjust as you like.
    inline constexpr float kTimeLimitSeconds = 180.0f;

    // Remaining time (seconds)
    inline float gTimeRemaining = kTimeLimitSeconds;

    // Prevent triggering lose multiple times
    inline bool gTimeUp = false;

    // Optional: pause timer (e.g., in win/lose screen)
    inline bool gTimerPaused = false;

    // Track which time-based sounds have been played
    inline bool gPlayed10SecWarning = false;
    inline bool gPlayed3SecBeep = false;
    inline bool gPlayed2SecBeep = false;
    inline bool gPlayed1SecBeep = false;
    inline bool gPlayedTimeUp = false;

    inline void SetTextColorAndScaleByName(const std::string& name,
        float r, float g, float b, float a,
        float scale) {
        std::vector<LEPANELFONTS::TextObjectData>& textObjects = LEPANELFONTS::GetMutableTextObjects();
        for (LEPANELFONTS::TextObjectData& textObj : textObjects) {
            if (textObj.name == name) {
                textObj.colorR = r;
                textObj.colorG = g;
                textObj.colorB = b;
                textObj.colorA = a;
                textObj.scale = scale;
                return;
            }
        }
    }

    // Keep UI objective and urgency feedback synchronized with gameplay values.
    inline void SyncUI()
    {
        // Money
        LEPANELFONTS::SetTextByName("MoneyText", "$" + std::to_string(gPlayerMoney));

        // Quota: "current / target"
        LEPANELFONTS::SetTextByName("QuotaText", "$" + std::to_string(gPlayerMoney) + " / $" + std::to_string(kQuota));

        // Optional objective hint text (only updates if this text object exists in scene)
        const int moneyLeftToGoal = std::max(0, kQuota - gPlayerMoney);
        if (moneyLeftToGoal > 0) {
            LEPANELFONTS::SetTextByName("ObjectiveText", "Serve dishes to earn $" + std::to_string(moneyLeftToGoal) + " more!");
        }
        else {
            LEPANELFONTS::SetTextByName("ObjectiveText", "Quota reached! Complete day transition...");
        }

        // Timer: format mm:ss
        int total = static_cast<int>(gTimeRemaining + 0.999f); // ceil-ish
        int mm = total / 60;
        int ss = total % 60;

        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << mm
            << ":" << std::setw(2) << std::setfill('0') << ss;

        LEPANELFONTS::SetTextByName("TimerText", oss.str());

        // Escalating timer feedback for urgency.
        if (gTimeRemaining <= 10.0f) {
            SetTextColorAndScaleByName("TimerText", 1.0f, 0.30f, 0.30f, 1.0f, 1.2f);
        }
        else if (gTimeRemaining <= 60.0f) {
            SetTextColorAndScaleByName("TimerText", 1.0f, 0.85f, 0.25f, 1.0f, 1.0f);
        }
        else {
            SetTextColorAndScaleByName("TimerText", 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    inline void Reset()
    {
        gPlayerMoney = 0;
        gQuotaReached = false;

        gTimeRemaining = kTimeLimitSeconds;
        gTimeUp = false;
        gTimerPaused = false;

        // Reset sound effect flags
        gPlayed10SecWarning = false;
        gPlayed3SecBeep = false;
        gPlayed2SecBeep = false;
        gPlayed1SecBeep = false;
        gPlayedTimeUp = false;

        SyncUI();
    }

    void OnQuotaReached(Scene& scene);
    void OnTimeUp(Scene& scene);

    inline void AddMoney(Scene& scene, int amount)
    {
        if (amount <= 0)
            return;

        gPlayerMoney += amount;

        SyncUI(); // <--- update UI immediately

        if (!gQuotaReached && gPlayerMoney >= kQuota)
        {
            gQuotaReached = true;
            OnQuotaReached(scene);
        }
    }

    inline void Update(float dt, Scene& scene)
    {
        if (gQuotaReached) return;
        if (gTimeUp) return;
        if (gTimerPaused) return;
        if (dt <= 0.0f) return;

        gTimeRemaining -= dt;

        if (gTimeRemaining <= 0.0f)
        {
            gTimeRemaining = 0.0f;
            gTimeUp = true;
            OnTimeUp(scene);
        }

        SyncUI(); // <--- update timer every frame (and quota/money too)
    }

    // Helpers (optional)
    inline float GetTimeRemaining() { return gTimeRemaining; }
}
