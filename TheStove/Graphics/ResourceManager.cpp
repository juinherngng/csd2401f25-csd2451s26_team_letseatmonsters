#include "ResourceManager.h"
#include <iostream>
#include <fmod_errors.h>

Shader* ResourceManager::LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) {
    auto it = shaders.find(name);
    if (it != shaders.end()) {
        std::cout << "Shader '" << name << "' already loaded, returning existing." << std::endl;
        return it->second.get();
    }

    auto shader = std::make_unique<Shader>(vertexPath, fragmentPath);
    Shader* shaderPtr = shader.get();
    shaders[name] = std::move(shader);

    std::cout << "Loaded shader: " << name << std::endl;
    return shaderPtr;
}

Shader* ResourceManager::GetShader(const std::string& name) {
    auto it = shaders.find(name);
    if (it != shaders.end()) {
        return it->second.get();
    }

    std::cerr << "Shader '" << name << "' not found!" << std::endl;
    return nullptr;
}

Mesh* ResourceManager::LoadMesh(const std::string& name, const std::vector<float>& vertices, GLsizei vertexCount, GLsizei vertexSize, Mesh::VertexLayout layout) {
    auto it = meshes.find(name);
    if (it != meshes.end()) {
        std::cout << "Mesh '" << name << "' already loaded, returning existing." << std::endl;
        return it->second.get();
    }

    auto mesh = std::make_unique<Mesh>(vertices.data(), vertexCount, vertexSize, layout);
    Mesh* meshPtr = mesh.get();
    meshes[name] = std::move(mesh);

    std::cout << "Loaded mesh: " << name << std::endl;
    return meshPtr;
}

Mesh* ResourceManager::GetMesh(const std::string& name) {
    auto it = meshes.find(name);
    if (it != meshes.end()) {
        return it->second.get();
    }

    std::cerr << "Mesh '" << name << "' not found!" << std::endl;
    return nullptr;
}

Texture* ResourceManager::LoadTexture(const std::string& name, const std::string& filePath) {
    auto it = textures.find(name);
    if (it != textures.end()) {
        std::cout << "Texture '" << name << "' already loaded, returning existing." << std::endl;
        return it->second.get();
    }

    auto texture = std::make_unique<Texture>();
    if (!texture->LoadFromFile(filePath)) {
        return nullptr;
    }

    Texture* texturePtr = texture.get();
    textures[name] = std::move(texture);

    std::cout << "Loaded texture: " << name << std::endl;
    return texturePtr;
}

Texture* ResourceManager::GetTexture(const std::string& name) {
    auto it = textures.find(name);
    if (it != textures.end()) {
        return it->second.get();
    }

    std::cerr << "Texture '" << name << "' not found!" << std::endl;
    return nullptr;
}

FMOD::Sound* ResourceManager::LoadAudio(std::string const  name, std::string const& filePath, bool loop, bool stream)
{
	// Ensure audio system is set
    if (!audioSystem) 
    {
        std::cerr << "Audio system not set!" << std::endl;
        return nullptr;
	}
    else
    {
		std::cout << "Audio system is set." << std::endl;
    }

	// Check if already loaded
    if (auto it = sounds.find(name); it != sounds.end())
    {
        std::cout << "Audio '" << name << "' already loaded, returning existing." << std::endl;
        return it->second;
	}

	// Set FMOD mode flags
	FMOD_MODE mode = FMOD_DEFAULT | (loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF) | 
                                    (stream ? FMOD_CREATESTREAM : FMOD_CREATESAMPLE);

	// Load sound
	FMOD::Sound* sound = nullptr;
	FMOD_RESULT result = audioSystem->createSound(filePath.c_str(), mode, nullptr, &sound);

	// Check for errors
    if (result != FMOD_OK)
    {
        std::cerr << "Failed to load audio '" << name << "': " << FMOD_ErrorString(result) << std::endl;
        return nullptr;
    }

	// Store sound
    sounds.emplace(name, sound);

	std::cout << "Loaded audio: " << name << std::endl;
    return sound;
}

FMOD::Sound* ResourceManager::GetAudio(std::string const& name) const
{
	// Check if audio exists
    if (auto it = sounds.find(name); it != sounds.end()) return it->second;

	std::cerr << "Audio '" << name << "' not found!" << std::endl;
	return nullptr;
}

void ResourceManager::UnloadAudio(std::string const& name)
{
	// Check if audio exists and erase
    if (auto it = sounds.find(name); it != sounds.end())
    {
		// Release sound
        sounds.erase(it);
		std::cout << "Unloaded audio: " << name << std::endl;
    }
}

bool ResourceManager::HasAudio(std::string const& name) const
{
	// Check if audio exists
	return sounds.find(name) != sounds.end();
}
bool ResourceManager::GetAudioInfo(std::string const& name, unsigned int& lengthMs, int& channels, int& bits, float& freq) const
{
	auto it = sounds.find(name);
    if (it == sounds.end() || !it->second) 
    {
        std::cerr << "Audio '" << name << "' not found!" << std::endl;
        return false;
	}

	// Retrieve sound info
	FMOD::Sound* snd = it->second;

	// Get length in milliseconds
    if (snd->getLength(&lengthMs, FMOD_TIMEUNIT_MS) != FMOD_OK) return false;

	// Get format info
    FMOD_SOUND_TYPE type;
    FMOD_SOUND_FORMAT format;
	// bits and channels are output parameters
	if (snd->getFormat(&type, &format, &channels, &bits) != FMOD_OK) return false;

	// Get default frequency
	if (snd->getDefaults(&freq, nullptr) != FMOD_OK) freq = 0;

    return true;
}

void ResourceManager::Clear() {
    if (!isCleared) {
        std::cout << "Clearing ResourceManager..." << std::endl;
        shaders.clear();
        meshes.clear();
        textures.clear();
        sounds.clear();
        isCleared = true;
    }
}
