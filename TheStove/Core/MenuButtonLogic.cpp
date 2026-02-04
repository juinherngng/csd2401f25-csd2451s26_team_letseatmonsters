/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			MenuButtonLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Ng Juin Herng, juinherng.ng@digipen.edu

 DESCRIPTION:		 Implements MenuButtonLogic::Update for release builds only, handling mouse click registering logic
					 against button AABB, hover texture transitions using ResourceManager, and triggering a cutscene
					 followed by JSON level load after left-click.

		 All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "MenuButtonLogic.hpp"
#include "../Graphics/GraphicsEngine.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/SceneManager.hpp"
#include "AudioManager.hpp"

#include <string>
#include <vector>
#include <filesystem>
#include <array>
#include <cmath>

#ifndef _DEBUG
namespace {
    static std::string MakeHoverPath(const std::string& path) {
        if (path.empty()) return path;
        const size_t dot = path.find_last_of('.');
        const std::string ext = (dot != std::string::npos) ? path.substr(dot) : std::string();
        const std::string base = (dot != std::string::npos) ? path.substr(0, dot) : path;
        if (base.size() >= 2 && base.substr(base.size() - 2) == "_h") {
            return dot != std::string::npos ? path : (base + ext);
        }
        if (base.size() >= 2 && base.substr(base.size() - 2) == "_s") {
            return base.substr(0, base.size() - 2) + "_h" + ext;
        }
        return base + "_h" + ext;
    }

    static void TrySetTexture(GameObject* owner, const std::string& texPath) {
        if (!owner || texPath.empty()) return;
        std::string cacheName = "staticsprite_" + texPath;
        if (Texture* tex = ResourceManager::Instance().LoadTexture(cacheName, texPath)) {
            owner->SetTexture(tex);
        }
    }

    // Discover per-chapter frames; fallback to a single frame if none found
    static std::array<std::vector<std::string>, 6> CollectChapterFrames() {
        namespace fs = std::filesystem;
        std::array<std::vector<std::string>, 6> chapters{};
        for (int ch = 1; ch <= 6; ++ch) {
            auto& list = chapters[ch - 1];
            bool any = false;
            for (int f = 1; f <= 300; ++f) {
                std::string path = "../assets/Cutscenes/Cutscene_starting_" + std::to_string(ch) + "." + std::to_string(f) + ".png";
                if (fs::exists(path)) {
                    list.push_back(path);
                    any = true;
                } else {
                    break;
                }
            }
            if (!any) {
                list.push_back("../assets/Cutscenes/Cutscene_starting_" + std::to_string(ch) + ".png");
            }
        }
        return chapters;
    }

    // Build frames so each chapter lasts exactly chapterHoldSeconds at fps.
    // - Repeats (or truncates) per-chapter frames to exactly targetFrames (round(chapterHoldSeconds * fps)).
    // - Marks only the first frame of each chapter as a boundary.
    // - Computes the zero-based flattened index of the first frame of chapter 6.
    static void BuildChapterTimedFramesAndBoundaries(std::vector<std::string>& outFrames,
                                                     std::vector<bool>& outFlags,
                                                     int& firstFrameIndexChapter6,
                                                     float chapterHoldSeconds,
                                                     float fps) {
        outFrames.clear();
        outFlags.clear();

        const auto chapters = CollectChapterFrames();
        const int targetFramesPerChapter = std::max(1, static_cast<int>(std::round(chapterHoldSeconds * fps)));

        firstFrameIndexChapter6 = -1;
        int runningIndex = 0;

        for (int ch = 1; ch <= 6; ++ch) {
            const auto& raw = chapters[ch - 1];
            if (raw.empty()) continue;

            // Build timed sequence for this chapter
            std::vector<std::string> timed;
            timed.reserve(targetFramesPerChapter);

            if (raw.size() == 1) {
                // Only one frame: repeat to fill duration
                for (int i = 0; i < targetFramesPerChapter; ++i) {
                    timed.push_back(raw[0]);
                }
            } else {
                // Repeat the raw sequence until reaching targetFramesPerChapter
                int produced = 0;
                while (produced < targetFramesPerChapter) {
                    for (const auto& frame : raw) {
                        timed.push_back(frame);
                        ++produced;
                        if (produced >= targetFramesPerChapter) break;
                    }
                }
            }

            // Append to flattened timeline; boundary true only on the first frame of this chapter
            for (size_t i = 0; i < timed.size(); ++i) {
                outFrames.push_back(timed[i]);
                outFlags.push_back(i == 0);

                if (ch == 6 && i == 0) {
                    firstFrameIndexChapter6 = runningIndex; // first frame of chapter 6 in the flattened timeline
                }
                ++runningIndex;
            }
        }
    }
}
#endif // _DEBUG

void MenuButtonLogic::Update(float /*dt*/, Scene& scene, InputManager& input) {
#ifdef _DEBUG
    (void)scene; (void)input;
#else
    if (scene.IsHowToPlayOverlayActive()) {
        return;
    }

    if (!initialized_) {
        normalTexturePath_ = scene.GetObjectTexturePath(GetOwnerID());
        hoverTexturePath_ = MakeHoverPath(normalTexturePath_);
        initialized_ = true;
    }

    glm::vec2 mouseWorld{};
    bool insideScene = GraphicsEngine::Instance().GetMouseWorldInScene(mouseWorld);
    if (!insideScene) {
        glm::vec3 w = input.ScreenToWorld(
            static_cast<float>(input.GetMousePosition().x),
            static_cast<float>(input.GetMousePosition().y));
        mouseWorld = glm::vec2(w.x, w.y);
    }

    GameObject* owner = GetOwner(scene);
    if (!owner) {
        return;
    }

    const glm::vec3 pos = owner->GetPositionGLM();
    const glm::vec3 sz = owner->GetScaleGLM();
    const float halfW = sz.x * 0.5f;
    const float halfH = sz.y * 0.5f;

    const bool over =
        mouseWorld.x >= (pos.x - halfW) && mouseWorld.x <= (pos.x + halfW) &&
        mouseWorld.y >= (pos.y - halfH) && mouseWorld.y <= (pos.y + halfH);

    if (over && !hovered_) {
        hovered_ = true;
        TrySetTexture(owner, hoverTexturePath_);
    }
    else if (!over && hovered_) {
        hovered_ = false;
        TrySetTexture(owner, normalTexturePath_);
    }

    if (over && input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        if (audioManager_) {
            audioManager_->PlayUIClickSound();
            
            // Fade out main menu BGM during the fade-to-black transition
            const float menuBgmFadeDuration = 0.35f; // Match the visual fade-out duration
            audioManager_->FadeChannel("bgm_MyoonchiDiner_MainMenu", 0.0f, menuBgmFadeDuration);
        }
        input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);

        // Build full timeline where each chapter lasts exactly 1.5 seconds at set fps
        std::vector<std::string> frames;
        std::vector<bool> boundaries;
        int firstFrameIndexChapter6 = -1;
        const float fps = 4.0f;                 // set fps here
        const float chapterHoldSeconds = 1.5f;  // set seconds to hold each cutscene here
        BuildChapterTimedFramesAndBoundaries(frames, boundaries, firstFrameIndexChapter6, chapterHoldSeconds, fps);

        // Each frame displays for 1/fps seconds
        const float holdSeconds = 1.0f / fps;

        // Chapter boundary fades; single crossfade when entering chapter 6
        const float fadeOutSeconds = 0.35f;
        const float fadeInSeconds = 0.35f;
        const int crossfadeToIndex = firstFrameIndexChapter6; // zero-based index in flattened frames
        const float crossfadeSeconds = 2.0f;

        // Start cutscene BGM with fade-in after the initial fade-to-black completes
        // The cutscene will handle playing and fading the intro cutscene music
        scene.StartCutsceneTransitionedBounded(
            frames,
            boundaries,
            targetJson_,
            activateSimulation_,
            fadeOutSeconds,
            fadeInSeconds,
            holdSeconds,
            crossfadeToIndex,
            crossfadeSeconds);
        
        // Start the intro cutscene BGM with fade-in
        // This plays after the fade-to-black, syncing with the cutscene start
        if (audioManager_) {
            const float cutsceneBgmFadeIn = 1.0f; // Fade in over 1 second
            // Play at volume 0, then fade up
            audioManager_->PlaySound("bgm_MyoonchiDiner_IntroCutscene", 0.0f, false);
            audioManager_->FadeChannel("bgm_MyoonchiDiner_IntroCutscene", audioManager_->GetBgmVolume(), cutsceneBgmFadeIn);
        }
    }
#endif // _DEBUG
}
