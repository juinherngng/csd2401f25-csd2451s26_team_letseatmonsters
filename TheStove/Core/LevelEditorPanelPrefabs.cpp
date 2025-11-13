/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelPrefabs.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:       Implementation of the Level Editor Prefabs panel.
					- Select/refresh prefab paths
					- Save selected scene object as a prefab (JSON)
					- Instantiate a new object from a prefab
					- Propagate prefab changes to all linked instances

		All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "LevelEditorPanelPrefabs.hpp"

#include "LevelEditor.hpp"
#include "LevelEditorFileIO.hpp"
#include "LevelEditorPrefabLinks.hpp"
#include "LevelSerializer.hpp"

#include "../Graphics/GraphicsEngine.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/SceneManager.hpp"
#include "../Graphics/GameObject.hpp"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace LEFILEIO;
using namespace LELINKS;

namespace LEPANELPREFABS {
	// Ensure a .json extension for save paths
	static inline void EnsureJsonExt(std::string& path) {
		if (fs::path(path).extension().empty()) {
			path += ".json";
		}
	}

	// Draw the Prefabs docked window
	void DrawPrefabsPanel(LevelEditor& editor, Scene& scene, int& selectedObjectId) {
		ImGui::SetNextWindowDockID(
			GraphicsEngine::Instance().GetMainDockspaceID(),
			ImGuiCond_FirstUseEver
		);


		if (!ImGui::Begin("Prefabs###LE_Prefabs")) {
			ImGui::End();
			return;
		}

		ImGui::SeparatorText("Prefabs / Archetypes");

		// Prefab path row (combo + input + refresh)
		static char prefabPathBuf[256] = "../prefabs/my_goat.json";
		static std::vector<std::string> sPrefabs = ListJsonFiles("../prefabs");

		ImGui::TextUnformatted("Prefab path");
		ImGui::SameLine();

		if (ImGui::Button("Refresh##pf")) {
			sPrefabs = ListJsonFiles("../prefabs");
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::BeginCombo("##PrefabCombo", prefabPathBuf)) {
			for (size_t i = 0; i < sPrefabs.size(); ++i) {
				bool selected = (sPrefabs[i] == prefabPathBuf);
				if (ImGui::Selectable(sPrefabs[i].c_str(), selected)) {
					std::snprintf(prefabPathBuf, sizeof(prefabPathBuf), "%s", sPrefabs[i].c_str());
				}

				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputText("##PrefabPathEdit", prefabPathBuf, IM_ARRAYSIZE(prefabPathBuf));

		std::string prefabPath = prefabPathBuf;
		EnsureJsonExt(prefabPath);
		const bool prefabExists = fs::exists(prefabPath);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Save selected as prefab (disabled during Play)
		if (editor.IsPlaying()) {
			ImGui::BeginDisabled();
		}

		if (ImGui::Button("Save selected as prefab")) {
			if (selectedObjectId >= 0) {
				// Find the selected GameObject
				GameObject* gSel = scene.GetGameObjectByID(selectedObjectId);
				if (gSel) {
					LevelObject out{};

					// Texture path
					out.texture = scene.GetObjectTexturePath(selectedObjectId);

					// Transform (position/scale) � store rotation in DEGREES for JSON
					const glm::vec3 p = gSel->GetPositionGLM();
					const glm::vec3 s = gSel->GetScaleGLM();

					out.x = p.x; out.y = p.y; out.z = p.z;
					out.w = s.x; out.h = s.y;

					out.rotation = glm::degrees(gSel->GetRotationAngleZ());

					// Collider
					const auto csz = gSel->GetColliderSize();
					const auto cof = gSel->GetColliderOffset();

					out.colWidth = csz.x;
					out.colHeight = csz.y;
					out.colOffsetX = cof.x;
					out.colOffsetY = cof.y;

					// Tagging by special IDs (project-specific convenience)
					if (selectedObjectId == scene.GetPlayerID()) {
						out.tag = "player";
					}
					else if (selectedObjectId == scene.GetNPC1ID()) {
						out.tag = "npc1";
					}
					else if (selectedObjectId == scene.GetNPC2ID()) {
						out.tag = "npc2";
					}
					else if (selectedObjectId == scene.GetDinoID()) {
						out.tag = "dino";
					}

					// NPC velocity (store even if unused by static objects)
					const glm::vec2 v = scene.GetNPCVelocity(selectedObjectId);
					out.speedX = v.x;
					out.speedY = v.y;

					// Animation flag
					out.animated = scene.HasAnimations(selectedObjectId);

					// Save and refresh list
					std::string savePath = prefabPath;
					if (SavePrefabToFile(savePath, out)) {
						std::snprintf(prefabPathBuf, sizeof(prefabPathBuf), "%s", savePath.c_str());
						sPrefabs = ListJsonFiles("../prefabs");

						// Link this instance to the prefab we just saved
						PrefabLinkByID[selectedObjectId] = savePath;
					}
				}
			}
		}

		if (editor.IsPlaying()) {
			ImGui::EndDisabled();
		}

		// Instantiate from prefab (disabled if path not found)
		ImGui::BeginDisabled(!prefabExists);
		if (ImGui::Button("Instantiate from prefab")) {
			LevelObject data{};
			if (LoadPrefabFromFile(prefabPath, data)) {
				GameObject* g = nullptr;

				if (data.animated) {
					// Minimal UV set for animated sprite; your attach logic can expand this
					const std::vector<glm::vec4> uvs = { glm::vec4(0.f, 0.f, 1.f, 1.f) };
					g = scene.SpawnAnimatedSprite(
						data.texture,
						{ data.x, data.y, data.z },
						{ data.w, data.h },
						uvs,
						0.25f,
						true,
						data.layer
					);

					// Optional: project-specific animation attach
					if (data.texture.find("dino") != std::string::npos) {
						scene.AttachDinoAnimations(g->GetID());
						scene.SetAnimation(g->GetID(), "IDLE");
					}
				}
				else {
					g = scene.SpawnStaticSprite(
						data.texture,
						{ data.x, data.y, data.z },
						{ data.w, data.h },
						data.layer
					);
				}

				if (g != nullptr) {
					// Apply rotation (JSON stores degrees; engine expects radians)
					g->SetRotation(glm::radians(data.rotation), { 0, 0, 1 });

					// Collider
					g->SetColliderSize({ data.colWidth,  data.colHeight });
					g->SetColliderOffset({ data.colOffsetX, data.colOffsetY });

					// Store back into scene helpers for consistency
					scene.SetObjectTexturePath(g->GetID(), data.texture);
					scene.SetTransformFromLevel(
						g->GetID(),
						{ data.x, data.y, data.z },
						{ data.w, data.h, 1.0f },
						data.rotation // degrees
					);
					scene.SetNPCVelocity(g->GetID(), data.speedX, data.speedY);

					// Re-assign special IDs if the prefab carries a tag
					if (data.tag == "player") {
						scene.SetPlayerID(g->GetID());
					}
					else if (data.tag == "npc1") {
						scene.SetNPC1ID(g->GetID());
					}
					else if (data.tag == "npc2") {
						scene.SetNPC2ID(g->GetID());
					}
					else if (data.tag == "dino") {
						scene.SetDinoID(g->GetID());
					}

					// Keep within walkable area
					scene.ClampToWalkArea(g);

					// Link instance to prefab path for propagation
					PrefabLinkByID[g->GetID()] = prefabPath;
				}
			}
		}

		ImGui::EndDisabled(); // !prefabExists

		// Propagate prefab changes to all instances linked to this prefab path
		if (ImGui::Button("Propagate prefab changes")) {
			if (prefabExists) {
				LevelObject data{};
				if (LoadPrefabFromFile(prefabPath, data)) {
					std::vector<GameObject*> objs; scene.CollectRenderablePointers(objs);

					for (auto* g : objs) {
						if (g == nullptr) {
							continue;
						}

						const int gid = g->GetID();
						auto it = PrefabLinkByID.find(gid);

						if (it != PrefabLinkByID.end() && it->second == prefabPath) {
							// Keep current position/Z; apply refreshed prefab data
							ApplyPrefabToObjectKeepPosition(data, scene, g);
						}
					}
				}
			}
		}

		ImGui::End();
	}
}
