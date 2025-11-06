#pragma once

#include "Animator.hpp"  // This includes Animator2D
#include <unordered_map>
#include <string>
#include <vector>
#include <glm/glm.hpp>

class EntityManager;  // Forward declaration

/**
 * @brief Manages animations for all game objects
 *
 * Handles animation registration, playback, and frame updates.
 * Works with EntityManager to apply animations to GameObjects.
 */
class AnimationManager {
public:
    AnimationManager() = default;
    ~AnimationManager() = default;

    // Core functionality
    void Update(float deltaTime, EntityManager& entityManager);
    void Clear();

    // Play/Pause control
    void Play();
    void Pause();
    void Stop();
    bool IsPlaying() const { return isPlaying; }

    // Animation registration for specific entity types
    void AttachDinoAnimations(int objectID);
    void AttachPlayerAnimations(int objectID);
    void AttachNPCAnimations(int objectID);

    // Animation control
    void SetAnimation(int objectID, const std::string& animName);
    std::string GetCurrentAnimation(int objectID) const;

    // Query
    bool HasAnimator(int objectID) const;

private:
    // Animator storage - each object has ONE animator with multiple named animation sets
    std::unordered_map<int, Animator2D> animators_;  // objectID -> Animator2D

    // Track which animation is active for each object
    std::unordered_map<int, std::string> currentAnimations_;

    // Store all animation sets for each object
    struct AnimationSet {
        std::vector<glm::vec4> frames;
        float frameDuration;
        bool loop;
    };
    std::unordered_map<int, std::unordered_map<std::string, AnimationSet>> animationSets_;

    bool isPlaying= false;  // Start paused by default

    // Helper: Create standard frame sequences
    std::vector<glm::vec4> CreateFrameSequence(int startFrame, int endFrame, int totalFrames);
};
