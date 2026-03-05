/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         SceneViewportPresenter.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Helper class responsible for presenting the scene viewport within an ImGui dockspace.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "imgui.h"

class SceneViewportPresenter {
public:
	// Begin the ImGui frame for the main dockspace and get the dockspace ID for docking other windows.
	void BeginDockspaceFrame(ImGuiID& dockspaceId) const;

	// Draw the scene viewport window with the given scene color texture and reference dimensions for aspect ratio.
	void DrawSceneWindow(unsigned int sceneColorTexture,
		int referenceWidth,
		int referenceHeight,
		ImGuiID dockspaceId,
		ImVec2& outSceneImagePos,
		ImVec2& outSceneImageSize) const;

	// Compute the screen-space rect of the scene image based on cached position/size and current window dimensions, for mouse picking and UI alignment.
	void ComputeSceneImageRect(const ImVec2& cachedSceneImagePos,
		const ImVec2& cachedSceneImageSize,
		int referenceWidth,
		int referenceHeight,
		ImVec2& outPos,
		ImVec2& outSize) const;

private:
	// Helper to fit a rect of given aspect ratio within available space, preserving aspect and centering.
	static ImVec2 FitRectToAspect(const ImVec2& available, float targetAspect);
};
