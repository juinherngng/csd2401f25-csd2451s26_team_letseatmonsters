/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         SceneViewportPresenter.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implementation of SceneViewportPresenter, a helper class responsible.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "imgui_internal.h"
#include "SceneViewportPresenter.hpp"

#include <cstdint>

 // Helper to fit a rect of given aspect ratio within available space, preserving aspect and centering.
ImVec2 SceneViewportPresenter::FitRectToAspect(const ImVec2& available, float targetAspect) {
	float width = available.x;
	float height = available.y;
	if (width <= 0.0f || height <= 0.0f || targetAspect <= 0.0f) {
		return ImVec2(0.0f, 0.0f);
	}

	const float currentAspect = width / height;

	if (currentAspect > targetAspect) {
		width = height * targetAspect;
	}
	else {
		height = width / targetAspect;
	}

	return ImVec2(width, height);
}

// Begin the ImGui frame for the main dockspace and get the dockspace ID for docking other windows.
void SceneViewportPresenter::BeginDockspaceFrame(ImGuiID& dockspaceId) const {
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags hostFlags =
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	if (ImGui::Begin("###DockSpaceHost", nullptr, hostFlags)) {
		dockspaceId = ImGui::GetID("MainDockSpace");
		ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), 0);
	}

	ImGui::End();
	ImGui::PopStyleVar(2);
}

// Draw the scene viewport window with the given scene color texture and reference dimensions for aspect ratio.
void SceneViewportPresenter::DrawSceneWindow(unsigned int sceneColorTexture,
	int referenceWidth,
	int referenceHeight,
	ImGuiID dockspaceId,
	ImVec2& outSceneImagePos,
	ImVec2& outSceneImageSize) const {
	ImGui::SetNextWindowDockID(dockspaceId, ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("Scene###SceneWindow")) {
		ImGui::End();
		return;
	}

	const ImVec2 available = ImGui::GetContentRegionAvail();
	const float targetAspect = static_cast<float>(referenceWidth) / static_cast<float>(referenceHeight);
	const ImVec2 imageSize = FitRectToAspect(available, targetAspect);

	const ImVec2 cursor = ImGui::GetCursorPos();
	const ImVec2 centeredCursor(
		cursor.x + (available.x - imageSize.x) * 0.5f,
		cursor.y + (available.y - imageSize.y) * 0.5f
	);
	ImGui::SetCursorPos(centeredCursor);

	outSceneImagePos = ImGui::GetCursorScreenPos();
	outSceneImageSize = imageSize;

	ImGui::Image(
		(ImTextureID)(intptr_t)sceneColorTexture,
		imageSize,
		ImVec2(0, 1),
		ImVec2(1, 0)
	);

	if (outSceneImageSize.x > 1.0f && outSceneImageSize.y > 1.0f) {
		ImGui::SetCursorScreenPos(outSceneImagePos);
		ImGui::InvisibleButton("##SceneImageBtn", outSceneImageSize);
	}

	ImGui::End();
}

// Compute the screen-space rect of the scene image based on cached position/size and current window dimensions, for mouse picking and UI alignment.
void SceneViewportPresenter::ComputeSceneImageRect(const ImVec2& cachedSceneImagePos,
	const ImVec2& cachedSceneImageSize,
	int referenceWidth,
	int referenceHeight,
	ImVec2& outPos,
	ImVec2& outSize) const {
	outPos = cachedSceneImagePos;
	outSize = cachedSceneImageSize;
	if (outSize.x > 1.0f && outSize.y > 1.0f) {
		return;
	}

	// If the cached size is invalid (e.g. first frame), compute a fitting rect based on current window dimensions and reference aspect ratio.
	const float targetAspect = static_cast<float>(referenceWidth) / static_cast<float>(referenceHeight);
	if (ImGuiWindow* sceneWindow = ImGui::FindWindowByName("Scene###SceneWindow")) {
		const ImRect contentRect = sceneWindow->InnerRect;
		const ImVec2 fitted = FitRectToAspect(ImVec2(contentRect.GetWidth(), contentRect.GetHeight()), targetAspect);
		outPos = ImVec2(
			contentRect.Min.x + (contentRect.GetWidth() - fitted.x) * 0.5f,
			contentRect.Min.y + (contentRect.GetHeight() - fitted.y) * 0.5f
		);
		outSize = fitted;
		return;
	}

	// As a fallback, fit within the main viewport's work area.
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 fitted = FitRectToAspect(viewport->WorkSize, targetAspect);
	outPos = ImVec2(
		viewport->WorkPos.x + (viewport->WorkSize.x - fitted.x) * 0.5f,
		viewport->WorkPos.y + (viewport->WorkSize.y - fitted.y) * 0.5f
	);
	outSize = fitted;
}
