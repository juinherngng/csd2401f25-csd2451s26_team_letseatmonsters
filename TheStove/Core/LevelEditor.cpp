/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelEditor.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Simple in-engine level editor window.
					- JSON/Editor store rotation in DEGREES.
					- GameObject setters should receive RADIANS (convert at call-site).

		 All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Graphics/SceneManager.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "LevelEditor.hpp"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <filesystem>
#include <unordered_map>
#include <iostream>

namespace fs = std::filesystem;

// Windows (native) includes for file dialog
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#endif

// Controls / Path
bool LevelEditor::IsEnabled() const {
	return isEnabled;
}

void LevelEditor::Toggle() {
	isEnabled = !isEnabled;
}

void LevelEditor::SetPath(const std::string& path) {
	levelPath = path;
}

// OS file picker. Returns empty string if cancelled.
static std::string OpenFileDialog(const char* filter) {
#ifdef _WIN32
	char filePathBuffer[MAX_PATH] = { 0 };

	OPENFILENAMEA ofn{};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = nullptr;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = filePathBuffer;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

	if (GetOpenFileNameA(&ofn)) {
		return std::string(filePathBuffer);
	}
#endif
	return {};
}

// Copy src into destDir/<basename>.
static std::string CopyFileIntoProjectUnique(const std::string& sourcePath, const std::string& destinationDir) {
	if (sourcePath.empty()) {
		return {};
	}

	std::error_code ec;
	fs::path src(sourcePath);

	if (!fs::exists(src, ec)) {
		return {};
	}

	fs::path dstDir(destinationDir);
	if (!fs::exists(dstDir, ec)) {
		fs::create_directories(dstDir, ec);

		if (ec) {
			return {};
		}
	}

	fs::path baseName = src.filename();
	fs::path dst = dstDir / baseName;

	int suffix = 1;

	while (fs::exists(dst, ec)) {
		dst = dstDir / (baseName.stem().string() + " (" + std::to_string(suffix++) + ")" + baseName.extension().string());
	}

	fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);

	if (ec) {
		return {};
	}

	// Return normalized with forward slashes so the rest of the pipeline stays happy.
	return (fs::path("..") / dst.lexically_normal().relative_path()).generic_string();
}

// Force-load a texture even if your ResourceManager caches by key.
static Texture* LoadTextureBypassingCache(const std::string& path) {
	const uint64_t tick = static_cast<uint64_t>(ImGui::GetTime() * 1'000'000.0);
	const std::string key = "sprite_" + path + "#v" + std::to_string(tick);

	return ResourceManager::Instance().LoadTexture(key, path);
}

// Move file to sibling "trash" folder.
static bool MoveToTrash(const std::string& filePath) {
	std::error_code ec;
	fs::path src(filePath);

	if (!fs::exists(src, ec)) {
		return false;
	}

	fs::path trashDir = src.parent_path() / "trash";

	if (!fs::exists(trashDir, ec)) {
		fs::create_directories(trashDir, ec);
	}

	fs::path dst = trashDir / src.filename();
	fs::rename(src, dst, ec);

	return !ec;
}

// Prefab helpers
static std::unordered_map<int, std::string> sPrefabLinkByID;

static bool SavePrefabToFile(std::string prefabPath, const LevelObject& src) {
	// Ensure extension
	if (fs::path(prefabPath).extension().empty()) {
		prefabPath += ".json";
	}

	// Ensure directory exists
	std::error_code ec;
	fs::path dir = fs::path(prefabPath).parent_path();

	if (!dir.empty() && !fs::exists(dir, ec)) {
		fs::create_directories(dir, ec);

		if (ec) {
			return false; // cannot create directory
		}
	}

	LevelData one;
	one.objects.clear();
	one.objects.push_back(src);

	return LevelSerializer::Save(prefabPath, one);
}

static bool LoadPrefabFromFile(const std::string& prefabPath, LevelObject& out) {
	LevelData one;

	if (!LevelSerializer::Load(prefabPath, one) || one.objects.empty()) {
		return false;
	}

	out = one.objects.front();
	return true;
}

// Apply prefab data to an existing object but keep its current position/z
static void ApplyPrefabToObjectKeepPosition(const LevelObject& prefab, Scene& scene, GameObject* obj) {
	if (obj == nullptr) {
		return;
	}

	const int id = obj->GetID();
	glm::vec3 keepPos = obj->GetPositionGLM();

	const float ww = prefab.w;
	const float hh = prefab.h;

	obj->SetScale(glm::vec3(ww, hh, 1.0f));
	obj->SetColliderSize({ prefab.colWidth, prefab.colHeight });
	obj->SetColliderOffset({ prefab.colOffsetX, prefab.colOffsetY });

	scene.SetTransformFromLevel(id, obj->GetPositionGLM(), { ww, hh, 1.0f }, prefab.rotation);

	scene.SetObjectTexturePath(id, prefab.texture);

	if (auto* tex = ResourceManager::Instance().LoadTexture(("sprite_" + prefab.texture), prefab.texture)) {
		obj->SetTexture(tex);
	}

	obj->SetPosition(keepPos);
	scene.ClampToWalkArea(obj);
}

// Point-in-AABB hit test for a sprite treated as a box centered at (pos.x, pos.y)
static bool IsPointInsideObject(const ImVec2 pointPx, const GameObject* obj) {
	if (obj == nullptr) {
		return false;
	}

	const glm::vec3 pos = obj->GetPositionGLM();
	const glm::vec3 size = obj->GetScaleGLM();

	const float halfW = 0.5f * size.x;
	const float halfH = 0.5f * size.y;

	const float minX = pos.x - halfW;
	const float maxX = pos.x + halfW;
	const float minY = pos.y - halfH;
	const float maxY = pos.y + halfH;

	return (pointPx.x >= minX && pointPx.x <= maxX && pointPx.y >= minY && pointPx.y <= maxY);
}

// NOT WORKING
// Scene picking / dragging (Scene viewport only; respects ImGui capture)
static void HandleScenePickDrag(Scene& scene, int& selectedIndex, int& selectedObjectId) {
	ImGuiIO& io = ImGui::GetIO();

	if (io.WantCaptureMouse) {
		return;
	}

	// Ask GraphicsEngine where the mouse is *inside the Scene image*
	glm::vec2 mouseWorld;

	if (!GraphicsEngine::Instance().GetMouseWorldInScene(mouseWorld)) {
		return; // mouse not over the Scene viewport this frame
	}

	static bool isDragging = false;
	static int draggingId = -1;
	static ImVec2 grabOffset = ImVec2(0.f, 0.f);

	// Build a list of objects for hit testing
	std::vector<GameObject*> list;
	scene.CollectRenderablePointers(list);

	// LMB press to pick (top-most)
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		int picked = -1;

		for (int i = (int)list.size() - 1; i >= 0; --i) {
			GameObject* g = list[i];

			if (!g) {
				continue;
			}

			// Treat sprite as centered AABB in world pixels
			const glm::vec3 pos = g->GetPositionGLM();
			const glm::vec3 sz = g->GetScaleGLM();

			const float hx = 0.5f * sz.x;
			const float hy = 0.5f * sz.y;

			const bool inside =
				(mouseWorld.x >= pos.x - hx && mouseWorld.x <= pos.x + hx) &&
				(mouseWorld.y >= pos.y - hy && mouseWorld.y <= pos.y + hy);

			if (inside) {
				picked = i;
				break;
			}
		}

		if (picked >= 0) {
			selectedIndex = picked;
			selectedObjectId = list[picked]->GetID();

			const glm::vec3 p = list[picked]->GetPositionGLM();
			grabOffset = ImVec2(mouseWorld.x - p.x, mouseWorld.y - p.y);

			isDragging = true;
			draggingId = selectedObjectId;
		}
		else {
			isDragging = false;
			draggingId = -1;
		}
	}

	// While LMB held to drag
	if (isDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		GameObject* g = scene.GetGameObjectByID(draggingId);

		if (g) {
			const float newX = mouseWorld.x - grabOffset.x;
			const float newY = mouseWorld.y - grabOffset.y;

			const glm::vec3 size = g->GetScaleGLM();
			const float rotDeg = glm::degrees(g->GetRotationAngleZ());

			scene.SetTransformFromLevel(
				draggingId,
				{ newX, newY, g->GetPositionGLM().z },
				{ size.x, size.y, 1.f },
				rotDeg
			);

			scene.ClampToWalkArea(g);
		}
	}

	// Release to stop dragging
	if (isDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
		isDragging = false;
		draggingId = -1;
	}
}

static void SyncSceneToLevel(Scene& scene, LevelData& levelOut);
static void SyncLevelToScene(const LevelData& levelIn, Scene& scene);

// Public Methods
bool LevelEditor::LoadIntoScene(Scene& scene) {
	if (!LevelSerializer::Load(levelPath, level)) {
		return false;
	}

	SyncLevelToScene(level, scene);
	return true;
}

// Enumerate .json filess
static std::vector<std::string> ListJsonFiles(const std::string& dir) {
	std::vector<std::string> out;

	std::error_code ec;

	if (!fs::exists(dir, ec)) {
		return out;
	}

	for (const auto& p : fs::directory_iterator(dir, ec)) {
		if (p.is_regular_file()) {
			const auto& path = p.path();

			if (path.extension() == ".json") {
				out.push_back(path.generic_string()); // keep forward slashes
			}
		}
	}

	std::sort(out.begin(), out.end());
	return out;
}

// Enumerate files with extensions.
static std::vector<std::string> ListAssetsWithExt(const std::string& dir, const std::vector<std::string>& exts) {
	std::vector<std::string> out;

	std::error_code ec;

	if (!fs::exists(dir, ec)) {
		return out;
	}

	for (const auto& p : fs::directory_iterator(dir, ec)) {
		if (!p.is_regular_file()) {
			continue;
		}

		auto ext = p.path().extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		for (auto const& e : exts) {
			if (ext == e) {
				out.push_back(p.path().generic_string());
				break;
			}
		}
	}

	std::sort(out.begin(), out.end());
	return out;
}

// DrawUI
void LevelEditor::DrawUI(Scene& scene) {
	static int selectedIndex = -1;
	static int selectedObjectId = -1;

	InputManager::Get().SetSceneViewportWantsGameMouse(false);

	if (!isEnabled) {
		return;
	}

	ImGuiContext* imguiContext = ImGui::GetCurrentContext();

	if (imguiContext == nullptr || !imguiContext->WithinFrameScope) {
		// ImGui frame hasn�t started yet; skip safely.
		return;
	}

	bool windowOpen = true;

	if (!windowOpen) {
		isEnabled = false;
	}

	// Level Window
	ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Level###LE_Level")) {
		// Level Path Row (dropdown + refresh)
		static char _pathBuf[256] = "../levels/kitchen01.json";

		if (levelPath.empty()) {
			levelPath = _pathBuf;
		}

		static std::vector<std::string> sLevelFiles = ListJsonFiles("../levels");

		ImGui::TextUnformatted("Level path");
		ImGui::SameLine();

		if (ImGui::Button("Refresh##levels")) {
			sLevelFiles = ListJsonFiles("../levels");
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::BeginCombo("##LevelCombo", levelPath.c_str())) {
			for (size_t i = 0; i < sLevelFiles.size(); ++i) {
				bool selected = (sLevelFiles[i] == levelPath);

				if (ImGui::Selectable(sLevelFiles[i].c_str(), selected)) {
					levelPath = sLevelFiles[i];
					std::snprintf(_pathBuf, sizeof(_pathBuf), "%s", levelPath.c_str());
				}

				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

		if (ImGui::InputText("##LevelPathEdit", _pathBuf, IM_ARRAYSIZE(_pathBuf))) {
			levelPath = _pathBuf;
		}

		// Load
		if (ImGui::Button("Load Level")) {
			if (LevelSerializer::Load(levelPath, level)) {
				scene.ClearAll();
				SyncLevelToScene(level, scene);

				scene.RebuildColliders();
				scene.SetSimulationActive(false);
				scene.ResetResizeBaseline();

				isPlaying = false;
				selectedIndex = -1;
				selectedObjectId = -1;
			}
		}

		ImGui::SameLine();

		// Save
		if (ImGui::Button("Save Level")) {
			level.objects.clear();

			std::vector<GameObject*> objectList;
			scene.CollectRenderablePointers(objectList);

			for (GameObject* obj : objectList) {
				if (!obj) {
					continue;
				}

				LevelObject out{};

				out.texture = scene.GetObjectTexturePath(obj->GetID());

				const glm::vec3 position = obj->GetPositionGLM();
				const glm::vec3 size = obj->GetScaleGLM();

				out.x = position.x;
				out.y = position.y;
				out.z = position.z;
				out.w = size.x;
				out.h = size.y;

				out.rotation = glm::degrees(obj->GetRotationAngleZ());

				const auto csz = obj->GetColliderSize();
				const auto cof = obj->GetColliderOffset();

				out.colWidth = csz.x;
				out.colHeight = csz.y;
				out.colOffsetX = cof.x;
				out.colOffsetY = cof.y;

				if (obj->GetID() == scene.GetPlayerID()) out.tag = "player";
				if (obj->GetID() == scene.GetNPC1ID()) out.tag = "npc1";
				if (obj->GetID() == scene.GetNPC2ID()) out.tag = "npc2";
				if (obj->GetID() == scene.GetDinoID()) out.tag = "dino";

				const glm::vec2 v = scene.GetNPCVelocity(obj->GetID());

				out.speedX = v.x; out.speedY = v.y;
				out.animated = scene.HasAnimations(obj->GetID());

				level.objects.push_back(out);
			}

			LevelSerializer::Save(levelPath, level);
		}

		// Play
		if (ImGui::Button(isPlaying ? "Playing..." : "Play")) {
			if (!isPlaying) {
				playStartSnapshot.objects.clear();

				SyncSceneToLevel(scene, playStartSnapshot);

				isPlaying = true;

				scene.SetSimulationActive(true);
				scene.ClearAll();

				SyncLevelToScene(playStartSnapshot, scene);
				scene.RebuildColliders();
			}
		}

		ImGui::SameLine();

		// Stop
		if (ImGui::Button("Stop")) {
			if (isPlaying) {
				scene.ClearAll();

				SyncLevelToScene(playStartSnapshot, scene);

				scene.RebuildColliders();
				scene.SetSimulationActive(false);

				isPlaying = false;
			}
		}

		ImGui::Separator();

		// Hierarchy
		std::vector<GameObject*> objectList;
		scene.CollectRenderablePointers(objectList);

		if (ImGui::BeginListBox("Objects", ImVec2(-FLT_MIN, 200.0f))) {
			for (int i = 0; i < (int)objectList.size(); ++i) {
				if (!objectList[i]) {
					continue;
				}

				std::string label = "ID " + std::to_string(objectList[i]->GetID());

				if (ImGui::Selectable(label.c_str(), selectedIndex == i)) {
					selectedIndex = i;
					selectedObjectId = objectList[i]->GetID();
				}
			}

			ImGui::EndListBox();
		}


		// Add
		if (isPlaying) {
			ImGui::BeginDisabled();
		}

		if (ImGui::Button("Add Object")) {
			LevelObject proto{};

			proto.texture = "../assets/goat_sprite_front.png";
			proto.tag = "npc";
			proto.x = 300.f;
			proto.y = 300.f;
			proto.z = 0.f;
			proto.w = 128.f;
			proto.h = 128.f;
			proto.rotation = 0.f;

			GameObject* obj = scene.SpawnStaticSprite(proto.texture, { proto.x, proto.y, proto.z }, { proto.w, proto.h });

			if (obj) {
				obj->SetColliderSize({ proto.colWidth, proto.colHeight });
				obj->SetColliderOffset({ proto.colOffsetX, proto.colOffsetY });

				scene.SetObjectTexturePath(obj->GetID(), proto.texture);

				Scene::Defaults defs{};
				defs.pos = { proto.x, proto.y, proto.z };
				defs.size = { proto.w, proto.h };
				defs.rot = proto.rotation;
				defs.colSize = { proto.colWidth, proto.colHeight };
				defs.colOff = { proto.colOffsetX, proto.colOffsetY };
				defs.vel = { proto.speedX, proto.speedY };
				defs.texture = proto.texture;
				defs.tag = proto.tag;

				scene.SetDefaults(obj->GetID(), defs);
			}
		}

		ImGui::SameLine();

		// Remove
		if (ImGui::Button("Remove Selected") && selectedIndex >= 0 && selectedIndex < (int)objectList.size()) {
			scene.DespawnByID(objectList[selectedIndex]->GetID());

			selectedIndex = -1;
			selectedObjectId = -1;
		}

		if (isPlaying) {
			ImGui::EndDisabled();
		}

		// Properties panel
		if (selectedIndex >= 0 && selectedIndex < (int)objectList.size() && objectList[selectedIndex]) {
			if (isPlaying) {
				ImGui::BeginDisabled();
			}

			GameObject* obj = objectList[selectedIndex];
			const int id = obj->GetID();

			ImGui::Separator();
			ImGui::Text("Properties (ID %d)", id);

			// Texture path buffer
			char textureBuf[256];
			{
				std::string texPath = scene.GetObjectTexturePath(id);

				if (texPath.empty()) {
					texPath = "../assets/goat_sprite_front.png";
				}

				std::snprintf(textureBuf, sizeof(textureBuf), "%s", texPath.c_str());
			}

			// Tag from special IDs
			char tagBuf[64] = "";

			if (id == scene.GetPlayerID()) {
				std::snprintf(tagBuf, sizeof(tagBuf), "player");
			}
			else if (id == scene.GetNPC1ID()) {
				std::snprintf(tagBuf, sizeof(tagBuf), "npc1");
			}
			else if (id == scene.GetNPC2ID()) {
				std::snprintf(tagBuf, sizeof(tagBuf), "npc2");
			}
			else if (id == scene.GetDinoID()) {
				std::snprintf(tagBuf, sizeof(tagBuf), "dino");
			}

			glm::vec3 position = obj->GetPositionGLM();
			glm::vec3 size = obj->GetScaleGLM();
			float rotationDeg = glm::degrees(obj->GetRotationAngleZ());
			auto colliderSize = obj->GetColliderSize();
			auto colliderOff = obj->GetColliderOffset();
			glm::vec2  velocity = scene.GetNPCVelocity(id);

			// Defaults for right-click reset
			const auto defaults = scene.GetDefaults(id);

			// Helpers: Drag widgets with right-click "Reset"
			auto DragVec2WithReset = [&](const char* label, float* v, ImVec2 d, float speed, auto apply) {
				bool changed = ImGui::DragFloat2(label, v, speed);

				if (ImGui::BeginPopupContextItem((std::string(label) + "_ctx").c_str())) {
					if (ImGui::MenuItem("Reset to default")) {
						v[0] = d.x; v[1] = d.y;
						apply(true);
					}

					ImGui::EndPopup();
				}

				if (changed) {
					apply(false);
				}

				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Right-click to reset");
				}
				};

			auto DragFloatWithReset = [&](const char* label, float* v, float d, float speed, auto apply) {
				bool changed = ImGui::DragFloat(label, v, speed);

				if (ImGui::BeginPopupContextItem((std::string(label) + "_ctx").c_str())) {
					if (ImGui::MenuItem("Reset to default")) {
						*v = d;
						apply(true);
					}
					ImGui::EndPopup();
				}

				if (changed) {
					apply(false);
				}

				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Right-click to reset");
				}
				};

			// Compact 2-column layout
			ImGui::Columns(2, nullptr, false);
			ImGui::SetColumnWidth(0, 120.0f);

			// Texture
			ImGui::Text("Texture");
			ImGui::NextColumn();

			ImGui::SetNextItemWidth(140.0f);

			if (ImGui::InputText("##TexturePath", textureBuf, IM_ARRAYSIZE(textureBuf))) {
				scene.SetObjectTexturePath(id, textureBuf);

				std::string newPath = textureBuf;
				if (auto* tex = ResourceManager::Instance().LoadTexture(("sprite_" + newPath), newPath)) {
					obj->SetTexture(tex);

					if (newPath.find("dino_") != std::string::npos) {
						scene.AttachDinoAnimations(id);
						scene.SetAnimation(id, "IDLE");
						scene.MarkAnimated(id, true);
					}
					else {
						obj->SetUVRect({ 0.f, 0.f, 1.f, 1.f });
						scene.MarkAnimated(id, false);
					}
				}
			}

			// Accept drag-drop of textures on the same row
			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* tp = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
					const char* dropped = static_cast<const char*>(tp->Data);

					std::vector<GameObject*> objs;
					scene.CollectRenderablePointers(objs);

					if (selectedIndex >= 0 && selectedIndex < (int)objs.size() && objs[selectedIndex]) {
						GameObject* o = objs[selectedIndex];
						const int id2 = o->GetID();

						std::string droppedPath = dropped;

						scene.SetObjectTexturePath(id2, droppedPath);

						if (auto* tex = ResourceManager::Instance().LoadTexture(("sprite_" + droppedPath), droppedPath)) {
							o->SetTexture(tex);

							if (droppedPath.find("dino_") != std::string::npos) {
								scene.AttachDinoAnimations(id2);
								scene.SetAnimation(id2, "IDLE");
								scene.MarkAnimated(id2, true);
							}
							else {
								o->SetUVRect({ 0.f, 0.f, 1.f, 1.f });
								scene.MarkAnimated(id2, false);
							}
						}
					}
				}

				ImGui::EndDragDropTarget();
			}

			ImGui::NextColumn();

			// Tag
			ImGui::Text("Tag");
			ImGui::NextColumn();

			ImGui::SetNextItemWidth(140.0f);
			ImGui::InputText("##Tag", tagBuf, IM_ARRAYSIZE(tagBuf));

			ImGui::NextColumn();

			// Position
			ImGui::Text("Position (x,y)");
			ImGui::NextColumn();

			ImGui::SetNextItemWidth(140.0f);

			DragVec2WithReset("##pos", &position.x, ImVec2(defaults.pos.x, defaults.pos.y), 1.0f, [&](bool) {
				obj->SetRotation(glm::radians(rotationDeg), { 0,0,1 });

				scene.SetTransformFromLevel(
					id,
					position,
					{ size.x, size.y, 1.0f },
					rotationDeg
				);

				scene.ClampToWalkArea(obj);
				});

			ImGui::NextColumn();

			// Size
			ImGui::Text("Size (w,h)");
			ImGui::NextColumn();

			ImGui::SetNextItemWidth(140.0f);

			DragVec2WithReset("##size", &size.x, ImVec2(defaults.size.x, defaults.size.y), 1.0f, [&](bool) {
				obj->SetRotation(glm::radians(rotationDeg), { 0,0,1 });

				scene.SetTransformFromLevel(
					id,
					position,
					{ size.x, size.y, 1.0f },
					rotationDeg
				);

				scene.ClampToWalkArea(obj);
				});

			ImGui::NextColumn();

			// Rotation
			ImGui::Text("Rotation (deg)");
			ImGui::NextColumn();

			ImGui::SetNextItemWidth(140.0f);

			DragFloatWithReset("##rot", &rotationDeg, defaults.rot, 0.25f, [&](bool) {
				obj->SetRotation(glm::radians(rotationDeg), { 0, 0, 1 });

				scene.SetTransformFromLevel(
					id,
					position,
					{ size.x, size.y, 1.0f },
					rotationDeg
				);
				});

			ImGui::NextColumn();

			// Collider size
			ImGui::Text("Collider (w,h)");
			ImGui::NextColumn();

			ImGui::SetNextItemWidth(140.0f);

			DragVec2WithReset("##colsz", &colliderSize.x, ImVec2(defaults.colSize.x, defaults.colSize.y), 1.0f, [&](bool) {
				obj->SetColliderSize({ colliderSize.x, colliderSize.y });
				});

			ImGui::NextColumn();

			// Collider offset
			ImGui::Text("Collider offset");
			ImGui::NextColumn();

			ImGui::SetNextItemWidth(140.0f);

			DragVec2WithReset("##coloff", &colliderOff.x, ImVec2(defaults.colOff.x, defaults.colOff.y), 1.0f, [&](bool) {
				obj->SetColliderOffset({ colliderOff.x, colliderOff.y });
				});

			ImGui::NextColumn();

			// Velocity
			ImGui::Text("Velocity (x,y)");
			ImGui::NextColumn();

			ImGui::SetNextItemWidth(140.0f);

			DragVec2WithReset("##vel", &velocity.x, ImVec2(defaults.vel.x, defaults.vel.y), 1.0f, [&](bool) {
				scene.SetNPCVelocity(id, velocity.x, velocity.y);
				});

			ImGui::NextColumn();

			// Start animation (if available)
			if (scene.HasAnimations(id)) {
				std::vector<std::string> animationNames = scene.GetAnimationList(id);
				std::string currentAnim = scene.GetCurrentAnimationName(id);

				int currentIndex = 0;

				for (int i = 0; i < (int)animationNames.size(); ++i) {
					if (animationNames[i] == currentAnim) {
						currentIndex = i; break;
					}
				}

				ImGui::Text("Start animation");
				ImGui::NextColumn();

				ImGui::SetNextItemWidth(140.0f);

				if (ImGui::BeginCombo("##animCombo", currentAnim.empty() ? "(none)" : currentAnim.c_str())) {
					for (int i = 0; i < (int)animationNames.size(); ++i) {
						bool selected = (i == currentIndex);

						if (ImGui::Selectable(animationNames[i].c_str(), selected)) {
							scene.SetAnimation(id, animationNames[i]);
						}

						if (selected) {
							ImGui::SetItemDefaultFocus();
						}
					}

					ImGui::EndCombo();
				}

				ImGui::NextColumn();
			}

			ImGui::Columns(1);

			// Allow dropping a prefab onto the inspector to APPLY values (keep position)
			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREFAB_PATH")) {
					const char* dropped = static_cast<const char*>(payload->Data);

					LevelObject data{};

					if (LoadPrefabFromFile(dropped, data)) {
						ApplyPrefabToObjectKeepPosition(data, scene, obj);
						sPrefabLinkByID[obj->GetID()] = dropped;
					}
				}

				ImGui::EndDragDropTarget();
			}

			// Final apply to keep scene/editor in sync
			scene.SetTransformFromLevel(id, position, { size.x, size.y, 1.0f }, rotationDeg);
			obj->SetColliderSize({ colliderSize.x, colliderSize.y });
			obj->SetColliderOffset({ colliderOff.x, colliderOff.y });
			scene.SetNPCVelocity(id, velocity.x, velocity.y);

			// Update special IDs if tag changed
			std::string newTag = tagBuf;

			if (newTag == "player") {
				scene.SetPlayerID(id);
			}

			if (newTag == "npc1") {
				scene.SetNPC1ID(id);
			}

			if (newTag == "npc2") {
				scene.SetNPC2ID(id);
			}

			if (newTag == "dino") {
				scene.SetDinoID(id);
			}

			if (isPlaying) {
				ImGui::EndDisabled();
			}
		}

		// Scene viewport area: drop-zone + pick/drag
		ImGui::Separator();

		ImGui::TextDisabled("Drop prefab to instantiate,\nor texture to apply to selected");

		ImVec2 viewportSize = ImGui::GetContentRegionAvail();

		if (viewportSize.y < 64.f) {
			viewportSize.y = 64.f;
		}
		if (viewportSize.x < 64.f) {
			viewportSize.x = 64.f;
		}

		// One big interactive area
		ImGui::InvisibleButton("##SceneViewport", viewportSize, ImGuiButtonFlags_MouseButtonLeft);

		const bool viewportHovered = ImGui::IsItemHovered();
		const bool viewportActive = ImGui::IsItemActive();

		InputManager::Get().SetSceneViewportWantsGameMouse(viewportHovered || viewportActive);

		// Drag-state for viewport dragging
		static bool isDragging = false;
		static int draggingObjectId = -1;
		static ImVec2 grabOffsetPx = ImVec2(0.f, 0.f);

		// Collect current objects (also used by drop handling below)
		std::vector<GameObject*> objectListForViewport;
		scene.CollectRenderablePointers(objectListForViewport);

		// Begin pick if user presses LMB inside the viewport and no ImGui drag-drop is in progress
		if (viewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsDragDropActive()) {
			const ImVec2 mouseScreen = ImGui::GetIO().MousePos;

			// Pick topmost by iterating back-to-front.
			int pickedIndex = -1;

			for (int i = static_cast<int>(objectListForViewport.size()) - 1; i >= 0; --i) {
				GameObject* g = objectListForViewport[i];

				if (!g) {
					continue;
				}

				if (IsPointInsideObject(mouseScreen, g)) {
					pickedIndex = i;
					break;
				}
			}

			if (pickedIndex >= 0) {
				// Select in hierarchy
				selectedIndex = pickedIndex;
				draggingObjectId = objectListForViewport[pickedIndex]->GetID();

				// Calculate grab offset so the object doesn't snap its center to the mouse instantly
				const glm::vec3 pos = objectListForViewport[pickedIndex]->GetPositionGLM();
				grabOffsetPx = ImVec2(mouseScreen.x - pos.x, mouseScreen.y - pos.y);

				isDragging = true;
			}
			else {
				// Clicked empty space; clear drag state
				isDragging = false;
				draggingObjectId = -1;
			}
		}

		// While dragging with LMB down, move selected object with clamp
		if (isDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			GameObject* g = scene.GetGameObjectByID(draggingObjectId);

			if (g) {
				const ImVec2 mouseScreen = ImGui::GetIO().MousePos;

				const float newX = mouseScreen.x - grabOffsetPx.x;
				const float newY = mouseScreen.y - grabOffsetPx.y;

				// Apply in one place so Properties stay in sync
				const glm::vec3 currentScale = g->GetScaleGLM();
				const float rotationDeg = glm::degrees(g->GetRotationAngleZ());

				// Update transform using your existing helper (rotation stored in degrees at editor layer)
				scene.SetTransformFromLevel(draggingObjectId,
					glm::vec3(newX, newY, g->GetPositionGLM().z),
					glm::vec3(currentScale.x, currentScale.y, 1.0f),
					rotationDeg);

				// Clamp to walkable area and bounce/order as per your existing rules
				scene.ClampToWalkArea(g);
			}
		}
		else {
			if (isDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
				// Stop dragging when mouse is released
				isDragging = false;
				draggingObjectId = -1;
			}
		}

		// Accept drag-drop
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* pp = ImGui::AcceptDragDropPayload("PREFAB_PATH")) {
				const char* dropped = static_cast<const char*>(pp->Data);

				LevelObject data{};

				if (LoadPrefabFromFile(dropped, data)) {
					GameObject* g = scene.SpawnStaticSprite(data.texture, { data.x, data.y, data.z }, { data.w, data.h });

					sPrefabLinkByID[g->GetID()] = dropped;

					if (g) {
						g->SetRotation(glm::radians(data.rotation), { 0,0,1 });
						g->SetColliderSize({ data.colWidth, data.colHeight });
						g->SetColliderOffset({ data.colOffsetX, data.colOffsetY });

						scene.SetObjectTexturePath(g->GetID(), data.texture);

						scene.SetTransformFromLevel(
							g->GetID(),
							{ data.x, data.y, data.z },
							{ data.w, data.h, 1.0f },
							data.rotation
						);

						scene.ClampToWalkArea(g);
					}
				}
			}

			if (const ImGuiPayload* tp = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
				const char* dropped = static_cast<const char*>(tp->Data);

				if (selectedIndex >= 0 && selectedIndex < (int)objectListForViewport.size() && objectListForViewport[selectedIndex]) {
					GameObject* o = objectListForViewport[selectedIndex];
					const int id2 = o->GetID();

					scene.SetObjectTexturePath(id2, dropped);

					if (auto* tex = ResourceManager::Instance().LoadTexture(("sprite_" + std::string(dropped)), dropped)) {
						o->SetTexture(tex);
					}
				}
			}

			ImGui::EndDragDropTarget();
		}
	}

	ImGui::End(); // Level window

	// Prefabs Window
	ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Prefabs###LE_Prefabs")) {
		// local object list used by buttons here
		std::vector<GameObject*> objectList;
		scene.CollectRenderablePointers(objectList);

		ImGui::Separator();
		ImGui::Text("Prefabs / Archetypes");
		ImGui::Spacing();

		static char prefabPathBuf[256] = "../prefabs/my_goat.json";
		static std::vector<std::string> sPrefabFiles = ListJsonFiles("../prefabs");

		ImGui::TextUnformatted("Prefab path");
		ImGui::SameLine();

		if (ImGui::Button("Refresh##prefabs")) {
			sPrefabFiles = ListJsonFiles("../prefabs");
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

		if (ImGui::BeginCombo("##PrefabCombo", prefabPathBuf)) {
			for (size_t i = 0; i < sPrefabFiles.size(); ++i) {
				bool selected = (std::string(prefabPathBuf) == sPrefabFiles[i]);

				if (ImGui::Selectable(sPrefabFiles[i].c_str(), selected)) {
					std::snprintf(prefabPathBuf, sizeof(prefabPathBuf), "%s", sPrefabFiles[i].c_str());
				}

				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputText("##PrefabPathEdit", prefabPathBuf, IM_ARRAYSIZE(prefabPathBuf));

		const bool prefabExists = fs::exists(prefabPathBuf);

		// Save selected as prefab
		if (ImGui::Button("Save selected as prefab")) {
			// We use the shared selectedObjectId captured from the Level window
			if (selectedObjectId >= 0) {
				// Find selected game object
				GameObject* gSel = nullptr;
				{
					std::vector<GameObject*> objs;
					scene.CollectRenderablePointers(objs);

					for (auto* g : objs) {
						if (g && g->GetID() == selectedObjectId) {
							gSel = g;
							break;
						}
					}
				}

				if (gSel) {
					LevelObject out{};

					out.texture = scene.GetObjectTexturePath(selectedObjectId);

					const glm::vec3 p = gSel->GetPositionGLM();
					const glm::vec3 s = gSel->GetScaleGLM();

					out.x = p.x;
					out.y = p.y;
					out.z = p.z;
					out.w = s.x;
					out.h = s.y;

					out.rotation = glm::degrees(gSel->GetRotationAngleZ());

					const auto csz = gSel->GetColliderSize();
					const auto cof = gSel->GetColliderOffset();

					out.colWidth = csz.x; out.colHeight = csz.y;
					out.colOffsetX = cof.x; out.colOffsetY = cof.y;

					// Tags & velocity
					if (selectedObjectId == scene.GetPlayerID()) { out.tag = "player"; }
					else if (selectedObjectId == scene.GetNPC1ID()) { out.tag = "npc1"; }
					else if (selectedObjectId == scene.GetNPC2ID()) { out.tag = "npc2"; }
					else if (selectedObjectId == scene.GetDinoID()) { out.tag = "dino"; }

					const glm::vec2 v = scene.GetNPCVelocity(selectedObjectId);

					out.speedX = v.x;
					out.speedY = v.y;

					out.animated = scene.HasAnimations(selectedObjectId);

					// Normalize path and save
					std::string savePath = prefabPathBuf;

					if (SavePrefabToFile(savePath, out)) {
						// Update text buffer with normalized path (adds .json if missing)
						std::snprintf(prefabPathBuf, sizeof(prefabPathBuf), "%s", savePath.c_str());

						// Link selection to this prefab
						sPrefabLinkByID[selectedObjectId] = savePath;

						// Refresh list so it appears immediately
						sPrefabFiles = ListJsonFiles("../prefabs");

						ImGui::OpenPopup("PrefabSavedPopup");
					}
				}
			}
		}

		// Instantiate from prefab
		if (fs::path(prefabPathBuf).extension().empty()) {
			std::string normalized = std::string(prefabPathBuf) + ".json";
			std::snprintf(prefabPathBuf, sizeof(prefabPathBuf), "%s", normalized.c_str());
		}

		ImGui::BeginDisabled(!prefabExists);

		if (ImGui::Button("Instantiate from prefab")) {
			LevelObject data{};

			if (LoadPrefabFromFile(prefabPathBuf, data)) {
				GameObject* g = nullptr;

				if (data.animated) {
					// Spawn animated with a valid first frame; attach real anims after
					const std::vector<glm::vec4> one = { glm::vec4(0.f,0.f,1.f,1.f) };

					g = scene.SpawnAnimatedSprite(
						data.texture,
						{ data.x, data.y, data.z },
						{ data.w, data.h },
						one,
						0.25f,
						true
					);

					scene.AttachDinoAnimations(g->GetID());
					scene.SetAnimation(g->GetID(), "IDLE");
				}
				else {
					g = scene.SpawnStaticSprite(
						data.texture,
						{ data.x, data.y, data.z },
						{ data.w, data.h }
					);
				}

				if (g) {
					g->SetRotation(glm::radians(data.rotation), { 0,0,1 });
					g->SetColliderSize({ data.colWidth, data.colHeight });
					g->SetColliderOffset({ data.colOffsetX, data.colOffsetY });

					scene.SetObjectTexturePath(g->GetID(), data.texture);

					scene.SetTransformFromLevel(
						g->GetID(),
						{ data.x, data.y, data.z },
						{ data.w, data.h, 1.0f },
						data.rotation
					);

					scene.SetNPCVelocity(g->GetID(), data.speedX, data.speedY);

					// special tags (optional)
					if (data.tag == "player") { scene.SetPlayerID(g->GetID()); }
					else if (data.tag == "npc1") { scene.SetNPC1ID(g->GetID()); }
					else if (data.tag == "npc2") { scene.SetNPC2ID(g->GetID()); }
					else if (data.tag == "dino") { scene.SetDinoID(g->GetID()); }

					scene.ClampToWalkArea(g);

					// link this new instance to the prefab path so "Propagate" works
					sPrefabLinkByID[g->GetID()] = prefabPathBuf;
				}
			}
		}

		// Propagate prefab changes
		if (ImGui::Button("Propagate prefab changes")) {
			if (prefabExists) {
				LevelObject data{};

				if (LoadPrefabFromFile(prefabPathBuf, data)) {
					std::vector<GameObject*> objs; scene.CollectRenderablePointers(objs);

					for (auto* g : objs) {
						if (!g) {
							continue;
						}

						const int gid = g->GetID();

						auto it = sPrefabLinkByID.find(gid);

						if ((it != sPrefabLinkByID.end()) && (it->second == std::string(prefabPathBuf))) {
							// Apply but keep each instance's current position/z
							ApplyPrefabToObjectKeepPosition(data, scene, g);
						}
					}
				}
			}
		}

		ImGui::EndDisabled();
	}

	ImGui::End(); // Prefabs window

	// Assets Window
	ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Assets###LE_Assets")) {
		ImGui::Text("Assets");
		ImGui::Spacing();

		ImGui::BeginChild("##AssetsBox", ImVec2(0, 0), true);

		static std::vector<std::string> sTextures = ListAssetsWithExt("../assets", { ".png", ".jpg", ".jpeg" });
		static std::vector<std::string> sPrefabs = ListAssetsWithExt("../prefabs", { ".json" });

		// Import row (browse & copy into ../assets)
		if (ImGui::Button("Import Texture...")) {
			const std::string pickedPath = OpenFileDialog("PNG files\0*.png\0All files\0*.*\0");

			if (!pickedPath.empty()) {
				const std::string projectPath = CopyFileIntoProjectUnique(pickedPath, "../assets");

				if (!projectPath.empty()) {
					// Live refresh so the new file appears immediately
					sTextures = ListAssetsWithExt("../assets", { ".png", ".jpg", ".jpeg" });

					ImGuiIO& io = ImGui::GetIO();

					const bool skipAutoApply = io.KeyCtrl
						|| ImGui::IsKeyDown(ImGuiKey_LeftCtrl)
						|| ImGui::IsKeyDown(ImGuiKey_RightCtrl);

					if (!skipAutoApply) {
						std::vector<GameObject*> objectList;
						scene.CollectRenderablePointers(objectList);

						if (selectedIndex >= 0 &&
							selectedIndex < static_cast<int>(objectList.size()) &&
							objectList[selectedIndex] != nullptr) {
							GameObject* obj = objectList[selectedIndex];
							const int id = obj->GetID();

							scene.SetObjectTexturePath(id, projectPath);

							if (Texture* tex = LoadTextureBypassingCache(projectPath)) {
								obj->SetTexture(tex);

								if (projectPath.find("dino_") != std::string::npos) {
									scene.AttachDinoAnimations(id);
									scene.SetAnimation(id, "IDLE");
									scene.MarkAnimated(id, true);
								}
								else {
									obj->SetUVRect({ 0.f, 0.f, 1.f, 1.f });
									scene.MarkAnimated(id, false);
								}
							}
						}
					}
				}
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Import Prefab...")) {
			const std::string picked = OpenFileDialog("JSON files\0*.json\0All files\0*.*\0");

			if (!picked.empty()) {
				const std::string projPath = CopyFileIntoProjectUnique(picked, "../prefabs");

				if (!projPath.empty()) {
					sPrefabs = ListAssetsWithExt("../prefabs", { ".json" }); // live refresh
				}
			}
		}

		ImGui::Separator();

		// Textures listing
		if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Button("Refresh##tex")) {
				sTextures = ListAssetsWithExt("../assets", { ".png", ".jpg", ".jpeg" });
			}

			bool refreshTextures = false;

			for (auto const& path : sTextures) {
				ImGui::PushID(path.c_str());
				ImGui::Selectable(path.c_str(), false);

				// Robust double-click even while Selectable is active
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
					ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					std::vector<GameObject*> objs; scene.CollectRenderablePointers(objs);

					if (selectedIndex >= 0 && selectedIndex < (int)objs.size() && objs[selectedIndex]) {
						GameObject* o = objs[selectedIndex];
						const int id2 = o->GetID();

						scene.SetObjectTexturePath(id2, path);

						if (auto* tex = LoadTextureBypassingCache(path)) {
							o->SetTexture(tex);

							if (path.find("dino_") != std::string::npos) {
								scene.AttachDinoAnimations(id2);
								scene.SetAnimation(id2, "IDLE");
								scene.MarkAnimated(id2, true);
							}
							else {
								o->SetUVRect({ 0.f, 0.f, 1.f, 1.f });
								scene.MarkAnimated(id2, false);
							}
						}
					}
				}

				// Drag source still works
				if (ImGui::BeginDragDropSource()) {
					ImGui::SetDragDropPayload("ASSET_PATH", path.c_str(), path.size() + 1);
					ImGui::TextUnformatted("Texture");
					ImGui::TextWrapped("%s", path.c_str());
					ImGui::EndDragDropSource();
				}

				// Right-click context menu (unique context id per item)
				if (ImGui::BeginPopupContextItem(("ctx_tex##" + path).c_str())) {
					if (ImGui::MenuItem("Delete")) {
						if (MoveToTrash(path)) {
							refreshTextures = true;
						}
					}

					ImGui::EndPopup();
				}

				ImGui::PopID();
			}

			if (refreshTextures) {
				sTextures = ListAssetsWithExt("../assets", { ".png", ".jpg", ".jpeg" });
			}
		}

		// Prefabs listing
		if (ImGui::CollapsingHeader("Prefabs", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Button("Refresh##pf")) {
				sPrefabs = ListAssetsWithExt("../prefabs", { ".json" });
			}

			bool refreshPrefabs = false;

			for (auto const& path : sPrefabs) {
				ImGui::PushID(path.c_str());
				ImGui::Selectable(path.c_str(), false);

				// Drag source for prefab
				if (ImGui::BeginDragDropSource()) {
					ImGui::SetDragDropPayload("PREFAB_PATH", path.c_str(), path.size() + 1);
					ImGui::TextUnformatted("Prefab");
					ImGui::TextWrapped("%s", path.c_str());
					ImGui::EndDragDropSource();
				}

				// Right-click context menu
				if (ImGui::BeginPopupContextItem(("ctx_prefab##" + path).c_str())) {
					if (ImGui::MenuItem("Delete")) {
						if (MoveToTrash(path)) {
							refreshPrefabs = true;
						}
					}

					ImGui::EndPopup();
				}

				ImGui::PopID();
			}

			if (refreshPrefabs) {
				sPrefabs = ListAssetsWithExt("../prefabs", { ".json" });
			}
		}

		ImGui::EndChild();
	}

	// Scene pick/drag integration (separate helper also handles dragging if mouse is on the Scene image).
	HandleScenePickDrag(scene, selectedIndex, selectedObjectId);

	ImGui::End(); // Assets window
}

// Sync helpers (scene <-> level)
static void SyncLevelToScene(const LevelData& levelIn, Scene& scene) {
	for (const auto& obj : levelIn.objects) {
		const float x = obj.x;
		const float y = obj.y;
		const float ww = obj.w;
		const float hh = obj.h;

		GameObject* g = nullptr;

		if (obj.animated) {
			// Give it a valid frame immediately so nothing smears before AttachDinoAnimations.
			const std::vector<glm::vec4> fullFrame = { glm::vec4(0.f, 0.f, 1.f, 1.f) };

			g = scene.SpawnAnimatedSprite(
				obj.texture,
				{ x, y, 0.0f },
				{ ww, hh },
				fullFrame,
				0.25f,
				true
			);

			// Now attach real animations (IDLE/WALK/ATTACK etc.)
			if (obj.texture.find("dino") != std::string::npos) {
				scene.AttachDinoAnimations(g->GetID());
				scene.SetAnimation(g->GetID(), "IDLE");
			}
		}
		else {
			g = scene.SpawnStaticSprite(obj.texture, { x, y, 0.0f }, { ww, hh });
		}

		if (!g) {
			std::cerr << "Failed to spawn object: " << obj.texture << std::endl;
			continue;  // skip this object, continue with next
		}


		// Rotation in LEVEL is degrees; GameObject expects radians
		g->SetRotation(glm::radians(obj.rotation), { 0, 0, 1 });

		// Collider
		g->SetColliderSize({ obj.colWidth, obj.colHeight });
		g->SetColliderOffset({ obj.colOffsetX, obj.colOffsetY });

		// Track texture path for save/inspector
		scene.SetObjectTexturePath(g->GetID(), obj.texture);

		// Tag special handles per-NPC velocity
		if (obj.tag == "player") {
			scene.SetPlayerID(g->GetID());
		}
		else if (obj.tag == "npc1") {
			scene.SetNPC1ID(g->GetID());
			scene.SetNPCVelocity(g->GetID(), obj.speedX, obj.speedY);
		}
		else if (obj.tag == "npc2") {
			scene.SetNPC2ID(g->GetID());
			scene.SetNPCVelocity(g->GetID(), obj.speedX, obj.speedY);
		}

		// This sets velocity for all objects 
		//scene.SetNPCVelocity(g->GetID(), obj.speedX, obj.speedY);

		// Keep editor/scene caches consistent (rotation stays in degrees at editor layer)
		scene.SetTransformFromLevel(g->GetID(),
			{ x, y, 0.0f },
			{ ww, hh, 1.0f },
			obj.rotation
		);

		// Store defaults so right-click Reset works
		Scene::Defaults defs{};
		defs.pos = { x, y, 0.0f };
		defs.size = { ww, hh };
		defs.rot = obj.rotation; // degrees

		defs.colSize = { obj.colWidth,  obj.colHeight };
		defs.colOff = { obj.colOffsetX, obj.colOffsetY };

		defs.vel = { obj.speedX, obj.speedY };
		defs.texture = obj.texture;
		defs.tag = obj.tag;

		scene.SetDefaults(g->GetID(), defs);
		scene.AttachLogicForTag(g->GetID(), obj.tag);

		// Clamp to walk area once after spawn
		scene.ClampToWalkArea(g);
	}
}

// Sync helpers
static void SyncSceneToLevel(Scene& scene, LevelData& levelOut) {
	levelOut.objects.clear();

	std::vector<GameObject*> list;
	scene.CollectRenderablePointers(list);

	for (GameObject* g : list) {
		if (!g) {
			continue;
		}

		LevelObject obj{};

		// Texture & animation flag
		obj.texture = scene.GetObjectTexturePath(g->GetID());
		obj.animated = scene.HasAnimations(g->GetID());

		// Transform
		const glm::vec3 p = g->GetPositionGLM();
		const glm::vec3 s = g->GetScaleGLM();
		obj.x = p.x;
		obj.y = p.y;
		obj.z = p.z;
		obj.w = s.x;
		obj.h = s.y;
		obj.rotation = glm::degrees(g->GetRotationAngleZ());

		// Collider (this was missing before)
		const auto csz = g->GetColliderSize();
		const auto cof = g->GetColliderOffset();
		obj.colWidth = csz.x;
		obj.colHeight = csz.y;
		obj.colOffsetX = cof.x;
		obj.colOffsetY = cof.y;

		// Tag / special IDs
		obj.tag.clear();
		if (g->GetID() == scene.GetPlayerID()) { obj.tag = "player"; }
		else if (g->GetID() == scene.GetNPC1ID()) { obj.tag = "npc1"; }
		else if (g->GetID() == scene.GetNPC2ID()) { obj.tag = "npc2"; }
		else if (g->GetID() == scene.GetDinoID()) { obj.tag = "dino"; }

		// Velocity
		const glm::vec2 v = scene.GetNPCVelocity(g->GetID());
		obj.speedX = v.x;
		obj.speedY = v.y;

		levelOut.objects.push_back(obj);
	}
}
