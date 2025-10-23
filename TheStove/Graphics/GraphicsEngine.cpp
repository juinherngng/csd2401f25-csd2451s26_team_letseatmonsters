/*
----------------------------------------------------------------------------------------------------
FILE NAME:			GraphicsEngine.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Seah Wang Hua, wanghua.seah@digipen.edu

DESCRIPTION:		Implements initialization, default resource loading, background handling,
					and batched rendering of GameObjects with error checks.

		All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "GraphicsEngine.hpp"
#include "MeshLoader.hpp"

// Constructor: Initializes references and identity matrices for view/projection.
GraphicsEngine::GraphicsEngine()
	: resourceManager(ResourceManager::Instance()),
	projection(1.0f),
	view(1.0f)
{
}

// Initialize renderer state and default camera/projection.
void GraphicsEngine::Initialize() {
	renderer.Initialize();
	renderer.SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, 1200, 800);

	// Setup matrices
	projection = glm::ortho(0.0f, 1200.0f, 800.0f, 0.0f);
	view = glm::mat4(1.0f);

	// Load default resources
	LoadDefaultResources();

	DebugRenderer::Init();
}

// Internal helper to preload common shaders and meshes.
void GraphicsEngine::LoadDefaultResources() {
	// Load default shader
	resourceManager.LoadShader("basic",
		"../TheStove/Graphics/shaders/shader.vert",
		"../TheStove/Graphics/shaders/shader.frag");

	// Load texture shader
	resourceManager.LoadShader("texture",
		"../TheStove/Graphics/shaders/texture.vert",
		"../TheStove/Graphics/shaders/texture.frag");

	// Load sprite shader
	resourceManager.LoadShader("sprite",
		"../TheStove/Graphics/shaders/sprite.vert",
		"../TheStove/Graphics/shaders/sprite.frag");

	// Load static sprite shader
	resourceManager.LoadShader("staticsprite",
		"../TheStove/Graphics/shaders/staticsprite.vert",
		"../TheStove/Graphics/shaders/staticsprite.frag");

	// Load animated sprite shader
	resourceManager.LoadShader("animatedsprite",
		"../TheStove/Graphics/shaders/animatedsprite.vert",
		"../TheStove/Graphics/shaders/animatedsprite.frag");

	// Load triangle mesh
	std::vector<float> vertices;
	GLsizei vertexCount, vertexSize;
	MeshLoader::LoadSimpleTriangle(vertices, vertexCount, vertexSize);
	resourceManager.LoadMesh("triangle", vertices, vertexCount, vertexSize);

	// Load sprite mesh
	MeshLoader::LoadSprite(vertices, vertexCount, vertexSize);
	resourceManager.LoadMesh("sprite", vertices, vertexCount, vertexSize);


	// Load fullscreen quad mesh
	MeshLoader::LoadFullscreenQuad(vertices, vertexCount, vertexSize);
	resourceManager.LoadMesh("fullscreen_quad", vertices, vertexCount, vertexSize);
}

//Sets and update a fullscreen background texture and ensure a background quad exists.
void GraphicsEngine::SetBackground(const std::string& texturePath) {
	// Load background texture
	Texture* bgTexture = resourceManager.LoadTexture("background", texturePath);
	if (!bgTexture) {
		std::cerr << "Failed to load background texture: " << texturePath << std::endl;
		return;
	}

	// Create background object if it doesn't exist
	if (!backgroundObject) {
		Mesh* quadMesh = resourceManager.GetMesh("fullscreen_quad");
		Shader* textureShader = resourceManager.GetShader("texture");

		if (quadMesh && textureShader) {
			backgroundObject = std::make_unique<GameObject>(quadMesh, textureShader);
			// Position background to fill screen
			backgroundObject->SetPosition(glm::vec3(600.0f, 400.0f, 0.0f)); // Center of 1200x800 screen
			backgroundObject->SetScale(glm::vec3(1200.0f, 800.0f, 1.0f));   // Full screen size
		}
	}

	if (backgroundObject) {
		backgroundObject->SetTexture(bgTexture);
	}
}

// Remove the background object (if present).
void GraphicsEngine::ClearBackground() {
	backgroundObject.reset();
}

// Clear frame using Renderer to begin a new frame
void GraphicsEngine::BeginFrame() {
	renderer.Clear();
}

// Render the  background and then all provided GameObjects
void GraphicsEngine::Render(const std::vector<GameObject*>& objects) {

	// Render background first (if exists)

	if (backgroundObject) {

		glDisable(GL_DEPTH_TEST); // Ensure background is always behind

		backgroundObject->Draw(view, projection);

		glEnable(GL_DEPTH_TEST);

	}

	// Draw all scene-provided objects 

	for (const auto* obj : objects) {

		if (!obj) continue;

		obj->Draw(view, projection);

		glDisable(GL_DEPTH_TEST);

		obj->DrawBoundingBox(view, projection, { 1.0f, 0.0f, 0.0f });

		glEnable(GL_DEPTH_TEST);

	}

	// DebugRenderer::DrawRect({ 100.f, 100.f, 0.f }, { 300.f, 250.f, 0.f }, { 1,0,0 });

	glDisable(GL_DEPTH_TEST);
	DebugRenderer::Flush(view, projection);
	glEnable(GL_DEPTH_TEST);

	// Check for OpenGL errors

	GLenum error;

	while ((error = glGetError()) != GL_NO_ERROR) {

		std::cerr << "OpenGL error after draw call: " << error << std::endl;

	}
}

// Destroy background and clear ResourceManager caches
void GraphicsEngine::Shutdown() {
	backgroundObject.reset();
	DebugRenderer::Shutdown();
	resourceManager.Clear();
}
