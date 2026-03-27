/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelFonts.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Ng Juin Herng, juinherng.ng@digipen.edu (70%)
 CO-AUTHORS:		Vu Phan Hung, phanhung.vu@digipen.edu   (30%)

 DESCRIPTION:       Font management panel for the Level Editor.
					Allows loading and managing multiple fonts.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "../Core/FontSystem.hpp"
#include "../Core/RuntimeTextData.hpp"

#include <string>
#include <vector>

class LevelEditor;
class Scene;

namespace LEPANELFONTS {
	/**
	 * @brief Draws the font-management panel and related text-object controls.
	 * @param editor Shared level editor controller.
	 * @param scene Scene currently being edited.
	 */
	void DrawFontsPanel(LevelEditor& editor, Scene& scene);

	using TextObjectData = RuntimeTextData;

	/**
	 * @brief Returns the current editor-managed text object list.
	 * @return Read-only reference to the cached text object data.
	 */
	const std::vector<TextObjectData>& GetTextObjects();

	/**
	 * @brief Replaces the cached text object list.
	 * @param textObjects New text object collection to store.
	 */
	void SetTextObjects(const std::vector<TextObjectData>& textObjects);
	/**
	 * @brief Replaces the cached text object list and synchronizes it with a scene.
	 * @param textObjects New text object collection to store.
	 * @param scene Scene to update with the supplied text objects.
	 */
	void SetTextObjectsWithScene(const std::vector<TextObjectData>& textObjects, Scene& scene);
	/**
	 * @brief Updates the displayed text for a named text object.
	 * @param name Name of the text object to modify.
	 * @param newText Replacement text value.
	 * @return True when a matching text object was updated.
	 */
	bool SetTextByName(const std::string& name, const std::string& newText);
	/**
	 * @brief Clears all cached text objects managed by the editor.
	 */
	void ClearTextObjects();
	/**
	 * @brief Returns mutable access to the cached text object list.
	 * @return Mutable reference to the editor-managed text object collection.
	 */
	std::vector<TextObjectData>& GetMutableTextObjects();

	/**
	 * @brief Returns the currently selected text-object index.
	 * @return Selected text-object index, or a negative value when none is selected.
	 */
	int GetSelectedTextIndex();
	/**
	 * @brief Updates the currently selected text-object index.
	 * @param index New selected text-object index.
	 */
	void SetSelectedTextIndex(int index);

	/**
	 * @brief Returns the names of fonts currently loaded into the font system.
	 * @return Read-only list of loaded font names.
	 */
	const std::vector<std::string>& GetLoadedFontNames();

	/**
	 * @brief Ensures that a named font is loaded and ready for use.
	 * @param fontName Logical name used to register the font.
	 * @param fontPath Filesystem path to the font asset.
	 * @param fontSize Requested font size in points/pixels depending on loader behavior.
	 * @return True when the font is available after the call.
	 */
	bool EnsureFontLoaded(const std::string& fontName,
		const std::string& fontPath,
		unsigned int fontSize);

	/**
	 * @brief Loads any fonts referenced by the current cached text objects.
	 */
	void EnsureFontsForTextObjectsLoaded();
}

