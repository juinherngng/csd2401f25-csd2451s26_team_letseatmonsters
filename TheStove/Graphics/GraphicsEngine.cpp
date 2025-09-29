#include "GraphicsEngine.h"
#include "MeshLoader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

GraphicsEngine::GraphicsEngine()
	: resourceManager(ResourceManager::Instance()),
	projection(1.0f),
	view(1.0f)
{
}

void GraphicsEngine::Initialize() {
	renderer.Initialize();
	renderer.SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, 1200, 800);

	// Setup matrices
	projection = glm::ortho(0.0f, 1200.0f, 800.0f, 0.0f);
	view = glm::mat4(1.0f);

	// Load default resources
	LoadDefaultResources();
}

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

	// Load triangle mesh
	std::vector<float> vertices;
	size_t vertexCount, vertexSize;
	MeshLoader::LoadSimpleTriangle(vertices, vertexCount, vertexSize);
	resourceManager.LoadMesh("triangle", vertices, vertexCount, vertexSize);

	// Load sprite mesh
	MeshLoader::LoadSprite(vertices, vertexCount, vertexSize);
	resourceManager.LoadMesh("sprite", vertices, vertexCount, vertexSize);

	// Load fullscreen quad mesh
	MeshLoader::LoadFullscreenQuad(vertices, vertexCount, vertexSize);
	resourceManager.LoadMesh("fullscreen_quad", vertices, vertexCount, vertexSize);
}

GameObject* GraphicsEngine::CreateGameObject(const std::string& meshName, const std::string& shaderName) {
	Mesh* mesh = resourceManager.GetMesh(meshName);
	Shader* shader = resourceManager.GetShader(shaderName);

	if (!mesh || !shader) {
		std::cerr << "Failed to create GameObject: missing resources" << std::endl;
		return nullptr;
	}

	auto obj = std::make_unique<GameObject>(mesh, shader);
	GameObject* objPtr = obj.get();
	gameObjects.push_back(std::move(obj));

	return objPtr;
}

void GraphicsEngine::RemoveGameObject(GameObject* obj) {
	gameObjects.erase(
		std::remove_if(gameObjects.begin(), gameObjects.end(),
			[obj](const std::unique_ptr<GameObject>& ptr) {
				return ptr.get() == obj;
			}),
		gameObjects.end());
}

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

void GraphicsEngine::ClearBackground() {
	backgroundObject.reset();
}

void GraphicsEngine::BeginFrame() {
	renderer.Clear();
}

void GraphicsEngine::Render() {

	// Render background first (if exists)
	if (backgroundObject) {
		glDisable(GL_DEPTH_TEST); // Ensure background is always behind
		backgroundObject->Draw(view, projection);
		glEnable(GL_DEPTH_TEST);
	}
	// Render all game objects
	for (const auto& obj : gameObjects) {
		obj->Draw(view, projection);

		glDisable(GL_DEPTH_TEST);
		obj->DrawBoundingBox(view, projection, { 1.0f, 0.0f, 0.0f });
		glEnable(GL_DEPTH_TEST);
	}

	// Check for OpenGL errors
	GLenum error;
	while ((error = glGetError()) != GL_NO_ERROR) {
		std::cerr << "OpenGL error after draw call: " << error << std::endl;
	}
}

void GraphicsEngine::Shutdown() {
	gameObjects.clear();
	resourceManager.Clear();
}
