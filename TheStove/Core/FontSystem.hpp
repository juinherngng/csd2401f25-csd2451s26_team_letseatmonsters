/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			FontSystem.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:		Font system using FreeType to load TTF fonts, render text with
					different fonts at different positions using OpenGL.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <map>
#include <memory>

namespace FontSystem {
	// Character information for rendering
	struct Character {
		GLuint     textureID;   // ID handle of the glyph texture
		glm::ivec2 size;        // Size of glyph
		glm::ivec2 bearing;     // Offset from baseline to left/top of glyph
		GLuint     advance;     // Horizontal offset to advance to next glyph
	};

	// Font class - manages a single loaded font
	class Font {
	public:
		Font() = default;
		~Font();

		// Load font from file
		bool Load(const std::string& fontPath, unsigned int fontSize);

		// Get character data
		const Character* GetCharacter(char c) const;

		// Get font size
		unsigned int GetFontSize() const {
			return m_fontSize;
		}

	private:
		std::map<char, Character> m_characters;
		unsigned int m_fontSize = 0;
		std::string m_fontPath;
	};

	// FontManager - singleton to manage FreeType library and multiple fonts
	class FontManager {
	public:
		static FontManager& Instance();

		// Initialize FreeType library
		bool Initialize();

		// Shutdown and cleanup
		void Shutdown();

		// Load a font with a given name
		Font* LoadFont(const std::string& name, const std::string& fontPath, unsigned int fontSize);

		// Get a loaded font by name
		Font* GetFont(const std::string& name);

		// Get FreeType library (for Font class to use)
		FT_Library GetLibrary() const {
			return m_library;
		}

	private:
		FontManager() = default;
		~FontManager();

		// Non-copyable
		FontManager(const FontManager&) = delete;
		FontManager& operator=(const FontManager&) = delete;

		FT_Library m_library = nullptr;
		bool m_initialized = false;
		std::map<std::string, std::unique_ptr<Font>> m_fonts;
	};

	// Text class - represents a renderable text object
	class Text {
	public:
		// Rotation mode enum
		enum class RotationMode {
			PerCharacter,  // Each character rotates individually (curved text effect)
			Block          // Entire text block rotates as one (normal rotation)
		};

		Text();
		~Text();

		// Set the font for this text
		void SetFont(Font* font);

		// Set text content
		void SetText(const std::string& text);

		// Set position in screen/world coordinates
		void SetPosition(const glm::vec2& position);

		// Set color (RGBA, 0-1 range)
		void SetColor(const glm::vec4& color);

		// Set scale
		void SetScale(float scale);

		// Set rotation in degrees
		void SetRotation(float degrees);

		// Set rotation mode
		void SetRotationMode(RotationMode mode);

		// Getters
		const std::string& GetText() const {
			return m_text;
		}
		const glm::vec2& GetPosition() const {
			return m_position;
		}
		const glm::vec4& GetColor() const {
			return m_color;
		}
		float GetScale() const {
			return m_scale;
		}
		float GetRotation() const {
			return m_rotation;
		}
		RotationMode GetRotationMode() const {
			return m_rotationMode;
		}
		Font* GetFont() const {
			return m_font;
		}

		// Render the text (called by TextRenderer)
		void Render(GLuint shaderProgram, const glm::mat4& projection);

	private:
		void SetupRendering();
		void CleanupRendering();

		Font* m_font = nullptr;
		std::string m_text;
		glm::vec2 m_position = glm::vec2(0.0f, 0.0f);
		glm::vec4 m_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		float m_scale = 1.0f;
		float m_rotation = 0.0f;  // Rotation in degrees
		RotationMode m_rotationMode = RotationMode::Block;  // Default to block rotation

		// OpenGL rendering resources
		GLuint m_VAO = 0;
		GLuint m_VBO = 0;
		bool m_renderingSetup = false;
	};

	// TextRenderer - manages shader and renders all text objects
	class TextRenderer {
	public:
		static TextRenderer& Instance();

		// Initialize shader
		bool Initialize();

		// Shutdown and cleanup
		void Shutdown();

		// Render a single text object
		void RenderText(Text& text, const glm::mat4& projection);

		// Render multiple text objects
		void RenderTexts(const std::vector<Text*>& texts, const glm::mat4& projection);

	private:
		TextRenderer() = default;
		~TextRenderer();

		// Non-copyable
		TextRenderer(const TextRenderer&) = delete;
		TextRenderer& operator=(const TextRenderer&) = delete;

		bool LoadShaders();
		GLuint CompileShader(GLenum type, const std::string& source);
		GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);

		GLuint m_shaderProgram = 0;
		bool m_initialized = false;
	};

} // namespace FontSystem