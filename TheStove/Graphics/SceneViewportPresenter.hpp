#pragma once

#include "imgui.h"

class SceneViewportPresenter {
public:
	void BeginDockspaceFrame(ImGuiID& dockspaceId) const;
	void DrawSceneWindow(unsigned int sceneColorTexture,
		int referenceWidth,
		int referenceHeight,
		ImGuiID dockspaceId,
		ImVec2& outSceneImagePos,
		ImVec2& outSceneImageSize) const;
	void ComputeSceneImageRect(const ImVec2& cachedSceneImagePos,
		const ImVec2& cachedSceneImageSize,
		int referenceWidth,
		int referenceHeight,
		ImVec2& outPos,
		ImVec2& outSize) const;

private:
	static ImVec2 FitRectToAspect(const ImVec2& available, float targetAspect);
};