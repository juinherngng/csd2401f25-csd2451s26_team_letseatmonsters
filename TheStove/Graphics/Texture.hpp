#pragma once

#include <glad/glad.h>
#include <string>

class Texture {
public:
    Texture();
    ~Texture();

    // Delete copy constructor and assignment (move-only like your other classes)
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Allow move operations
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool LoadFromFile(const std::string& filePath);
    void Bind(unsigned int slot = 0) const;
    void Unbind() const;

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    GLuint GetID() const { return textureID; }

private:
    GLuint textureID;
    int width, height, channels;
};
