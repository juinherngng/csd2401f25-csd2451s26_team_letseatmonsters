/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         SceneViewportPresenter.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Helper class responsible for presenting the scene viewport within an ImGui dockspace.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "imgui.h"

class SceneViewportPresenter {
public:

	/**
	 * @brief Begins dockspace frame.
	 * @param dockspaceId ImGui dockspace identifier.
	 */
	void BeginDockspaceFrame(ImGuiID& dockspaceId) const;

	/**
	 * @brief Draws scene window.
	 * @param sceneColorTexture Parameter for scene color texture.
	 * @param referenceWidth Reference width in pixels.
	 * @param referenceHeight Reference height in pixels.
	 * @param dockspaceId ImGui dockspace identifier.
	 * @param outSceneImagePos Output value for out scene image pos.
	 * @param outSceneImageSize Output value for out scene image size.
	 */
	void DrawSceneWindow(unsigned int sceneColorTexture,
		int referenceWidth,
		int referenceHeight,
		ImGuiID dockspaceId,
		ImVec2& outSceneImagePos,
		ImVec2& outSceneImageSize) const;

	/**
	 * @brief Computes scene image rect.
	 * @param cachedSceneImagePos Parameter for cached scene image pos.
	 * @param cachedSceneImageSize Parameter for cached scene image size.
	 * @param referenceWidth Reference width in pixels.
	 * @param referenceHeight Reference height in pixels.
	 * @param outPos Output value for out pos.
	 * @param outSize Output value for out size.
	 */
	void ComputeSceneImageRect(const ImVec2& cachedSceneImagePos,
		const ImVec2& cachedSceneImageSize,
		int referenceWidth,
		int referenceHeight,
		ImVec2& outPos,
		ImVec2& outSize) const;

private:

	/**
	 * @brief Fits rect to aspect.
	 * @param available Parameter for available.
	 * @param targetAspect Parameter for target aspect.
	 * @return Result produced by this operation.
	 */
	static ImVec2 FitRectToAspect(const ImVec2& available, float targetAspect);
};
