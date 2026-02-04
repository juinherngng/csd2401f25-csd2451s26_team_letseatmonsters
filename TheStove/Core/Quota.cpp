#include "../Core/Quota.hpp"
#include "../Graphics/SceneManager.hpp"
#include "FilePaths.hpp"

#include <string>
#include <vector>
#include <array>
#include <filesystem>
#include <cmath>
#include <algorithm>

namespace Economy
{
    namespace
    {
        // Supports BOTH patterns:
        //   A) baseFolder/prefix + chapter + "." + frame + ".png"   (e.g. Cutscene_gameover1.1.png)
        //   B) baseFolder/prefix + chapter + ".png"               (e.g. Cutscene_gameover1.png)
        //
        // Example inputs:
        //   baseFolder = "../assets/Lose"
        //   prefix     = "Cutscene_gameover"   -> Cutscene_gameover1.1.png or Cutscene_gameover1.png
        static std::array<std::vector<std::string>, 6>
            CollectChapterFrames(const std::string& baseFolder, const std::string& prefix)
        {
            namespace fs = std::filesystem;

            std::array<std::vector<std::string>, 6> chapters{};
            for (int ch = 1; ch <= 6; ++ch) {
                auto& list = chapters[ch - 1];
                bool any = false;

                // Try multi-frame: prefix + chapter + "." + frame
                for (int f = 1; f <= 300; ++f) {
                    std::string path =
                        baseFolder + "/" + prefix + std::to_string(ch) + "." + std::to_string(f) + ".png";

                    if (fs::exists(path)) {
                        list.push_back(path);
                        any = true;
                    }
                    else {
                        break;
                    }
                }

                // Fallback single image: prefix + chapter
                if (!any) {
                    std::string fallback =
                        baseFolder + "/" + prefix + std::to_string(ch) + ".png";
                    if (fs::exists(fallback)) list.push_back(fallback);
                }
            }
            return chapters;
        }

        static void BuildTimedFramesAndBoundaries(std::vector<std::string>& outFrames,
            std::vector<bool>& outFlags,
            float chapterHoldSeconds,
            float fps,
            const std::string& baseFolder,
            const std::string& prefix)
        {
            outFrames.clear();
            outFlags.clear();

            const auto chapters = CollectChapterFrames(baseFolder, prefix);
            const int targetFramesPerChapter =
                std::max(1, static_cast<int>(std::round(chapterHoldSeconds * fps)));

            for (int ch = 1; ch <= 6; ++ch) {
                const auto& raw = chapters[ch - 1];
                if (raw.empty()) continue;

                int produced = 0;
                while (produced < targetFramesPerChapter) {
                    for (const auto& frame : raw) {
                        outFrames.push_back(frame);
                        outFlags.push_back(produced == 0); // boundary at first frame of the chapter
                        ++produced;
                        if (produced >= targetFramesPerChapter) break;
                    }
                }
            }
        }
    }

    void OnQuotaReached(Scene& scene)
    {
        if (gTimerPaused) return;
        gTimerPaused = true;

        std::vector<std::string> frames;
        std::vector<bool> boundaries;

        const float fps = 4.0f;
        const float chapterHoldSeconds = 1.5f;

        BuildTimedFramesAndBoundaries(
            frames, boundaries,
            chapterHoldSeconds, fps,
            "../assets/Win",
            "Cutscene_daychange_"
        );

        // If no frames, go straight to MAIN MENU
        if (frames.empty()) {
            scene.StartLevelTransition(FilePaths::Levels::MAIN_MENU, false, 0.35f, 0.35f);
            return;
        }

        scene.StartCutsceneTransitionedBounded(
            frames,
            boundaries,
            FilePaths::Levels::MAIN_MENU,   // <-- changed from WIN to MAIN_MENU
            true,
            0.35f,
            0.35f,
            1.0f / fps,
            -1,
            0.0f
        );
    }

    void OnTimeUp(Scene& scene)
    {
        if (gTimerPaused) return;
        gTimerPaused = true;

        std::vector<std::string> frames;
        std::vector<bool> boundaries;

        const float fps = 4.0f;
        const float chapterHoldSeconds = 1.5f;

        BuildTimedFramesAndBoundaries(
            frames, boundaries,
            chapterHoldSeconds, fps,
            "../assets/Lose",
            "Cutscene_gameover"
        );

        // If no frames, go straight to MAIN MENU
        if (frames.empty()) {
            scene.StartLevelTransition(FilePaths::Levels::MAIN_MENU, false, 0.35f, 0.35f);
            return;
        }

        scene.StartCutsceneTransitionedBounded(
            frames,
            boundaries,
            FilePaths::Levels::MAIN_MENU,   // <-- changed from LOSE to MAIN_MENU
            true,
            0.35f,
            0.35f,
            1.0f / fps,
            -1,
            0.0f
        );
    }


}

