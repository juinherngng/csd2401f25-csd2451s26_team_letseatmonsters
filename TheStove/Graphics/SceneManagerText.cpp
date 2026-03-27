/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManagerText.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements Scene text and authored UI rendering helpers to keep
					the core scene implementation smaller and easier to navigate.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Core/LevelEditorPanelFonts.hpp"

#include "GraphicsEngine.hpp"
#include "ResourceManager.hpp"
#include "SceneManager.hpp"

#include <unordered_set>

 /**
  * @brief Renders fpstext.
  * @return Result produced by this operation.
  */
void Scene::RenderFPSText() {
#ifndef _DEBUG
	if (!showFPS_) {
		return;
	}

	// Use the current scene projection so the overlay aligns with the active render target.
	glm::mat4 projection = graphicsEngine.GetProjection();

	// Save current GL viewport so we can restore after drawing.
	GLint prevViewport[4];
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	// Match the viewport to the current render target before drawing text.
	GLint boundFBO = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &boundFBO);
	if (boundFBO != 0) {
		glViewport(0, 0, graphicsEngine.GetSceneWidth(), graphicsEngine.GetSceneHeight());
	}
	else {
		graphicsEngine.ApplyViewport();
	}

	// Text always renders with alpha blending and without depth testing.
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	FontSystem::TextRenderer::Instance().RenderText(fpsText_, projection);

	glDisable(GL_BLEND);
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
#endif
}

/**
 * @brief Renders authored level text objects and runtime floating text overlays.
 */
void Scene::RenderLevelTextObjects() {
	// Skip the pass entirely when there are no authored text objects to draw.
	const auto& objs = LEPANELFONTS::GetTextObjects();
	if (objs.empty()) {
		return;
	}

	// Cutscene-specific UI uses separate drawing paths, so regular authored text stays hidden here.
	const bool cutsceneActive = IsAnyCutsceneActive();
	if (cutsceneActive) {
		return;
	}

	const bool pauseActive = IsPauseOverlayActive();
	static const std::unordered_set<std::string> kHudTextNames = {
		"MoneyText", "QuotaText", "TimerText"
	};

	glm::mat4 projection = graphicsEngine.GetProjection();

	GLint prevViewport[4];
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	GLint boundFBO = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &boundFBO);
	if (boundFBO != 0) {
		glViewport(0, 0, graphicsEngine.GetSceneWidth(), graphicsEngine.GetSceneHeight());
	}
	else {
		graphicsEngine.ApplyViewport();
	}

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Render dynamic feedback text before authored labels so authored UI can remain on top.
	RenderFloatingWorldTextFx(projection, pauseActive, cutsceneActive);

	for (const auto& o : objs) {
		// Hide gameplay HUD text while cutscene or pause overlays own the top-level presentation.
		if ((cutsceneActive || pauseActive) && kHudTextNames.count(o.name)) {
			continue;
		}

		// Ignore incomplete text records that cannot produce visible output.
		if (o.text.empty() || o.fontName.empty() || o.colorA <= 0.001f) {
			continue;
		}

		// Respect layer visibility so editor-authored UI follows the same rules as sprites.
		if (!o.layer.empty()) {
			Layer* layer = GetLayer(o.layer);
			if (layer && (!layer->IsEnabled() || !layer->IsVisible())) {
				continue;
			}
		}

		FontSystem::Font* font = ResourceManager::Instance().GetFont(o.fontName);
		if (!font) {
			// Skip silently when the font could not be resolved for this frame.
			continue;
		}

		// Build a transient text object from the authored settings and render it immediately.
		FontSystem::Text text;
		text.SetFont(font);
		text.SetText(o.text);
		text.SetColor(glm::vec4(o.colorR, o.colorG, o.colorB, o.colorA));
		text.SetScale(o.scale);
		text.SetRotation(o.rotation);
		text.SetPosition(glm::vec2(o.x, o.y));
		text.SetRotationMode(o.useBlockRotation
			? FontSystem::Text::RotationMode::Block
			: FontSystem::Text::RotationMode::PerCharacter);

		FontSystem::TextRenderer::Instance().RenderText(text, projection);
	}

	glDisable(GL_BLEND);
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}
