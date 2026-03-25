/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			FontSystem.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (70%)
 CO-AUTHORS:        Yat Chun Wee, y.chunwee@digipen.edu		(30%)

 DESCRIPTION:		Font system using FreeType to load TTF fonts, render text with
					different fonts at different positions using OpenGL.

		All content  2025 DigiPen Institute of Technology Singapore. All rights reserved.
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

		/**
		 * @brief Constructs a `Font` instance.
		 */
		Font() = default;

		/**
		 * @brief Destroys the `Font` instance and releases owned resources.
		 */
		~Font();

		/**
		 * @brief Loads this object.
		 * @param fontPath Parameter for font path.
		 * @param fontSize Parameter for font size.
		 * @return True when the operation succeeds or the condition is met.
		 */
		bool Load(const std::string& fontPath, unsigned int fontSize);

		/**
		 * @brief Returns character.
		 * @param c Parameter for c.
		 * @return Requested value.
		 */
		const Character* GetCharacter(char c) const;

		/**
		 * @brief Returns font size.
		 * @return Requested value.
		 */
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

		/**
		 * @brief Performs instance.
		 * @return Result produced by this operation.
		 */
		static FontManager& Instance();

		/**
		 * @brief Initializes this object.
		 * @return True when the operation succeeds or the condition is met.
		 */
		bool Initialize();

		/**
		 * @brief Performs shutdown.
		 */
		void Shutdown();

		/**
		 * @brief Loads font.
		 * @param name Parameter for name.
		 * @param fontPath Parameter for font path.
		 * @param fontSize Parameter for font size.
		 * @return Result produced by this operation.
		 */
		Font* LoadFont(const std::string& name, const std::string& fontPath, unsigned int fontSize);

		/**
		 * @brief Returns font.
		 * @param name Parameter for name.
		 * @return Requested value.
		 */
		Font* GetFont(const std::string& name);

		/**
		 * @brief Returns library.
		 * @return Requested value.
		 */
		FT_Library GetLibrary() const {
			return m_library;
		}

	private:

		/**
		 * @brief Constructs a `FontManager` instance.
		 */
		FontManager() = default;

		/**
		 * @brief Destroys the `FontManager` instance and releases owned resources.
		 */
		~FontManager();

		/**
		 * @brief Constructs a `FontManager` instance.
		 */
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

		/**
		 * @brief Constructs a `Text` instance.
		 */
		Text();

		/**
		 * @brief Destroys the `Text` instance and releases owned resources.
		 */
		~Text();

		/**
		 * @brief Sets font.
		 * @param font Parameter for font.
		 */
		void SetFont(Font* font);

		/**
		 * @brief Sets text.
		 * @param text Parameter for text.
		 */
		void SetText(const std::string& text);

		/**
		 * @brief Sets position.
		 * @param position Parameter for position.
		 */
		void SetPosition(const glm::vec2& position);

		/**
		 * @brief Sets color.
		 * @param color Parameter for color.
		 */
		void SetColor(const glm::vec4& color);

		/**
		 * @brief Sets scale.
		 * @param scale Parameter for scale.
		 */
		void SetScale(float scale);

		/**
		 * @brief Sets rotation.
		 * @param degrees Parameter for degrees.
		 */
		void SetRotation(float degrees);

		/**
		 * @brief Sets rotation mode.
		 * @param mode Parameter for mode.
		 */
		void SetRotationMode(RotationMode mode);

		/**
		 * @brief Returns text.
		 * @return Requested value.
		 */
		const std::string& GetText() const {
			return m_text;
		}

		/**
		 * @brief Returns position.
		 * @return Requested value.
		 */
		const glm::vec2& GetPosition() const {
			return m_position;
		}

		/**
		 * @brief Returns color.
		 * @return Requested value.
		 */
		const glm::vec4& GetColor() const {
			return m_color;
		}

		/**
		 * @brief Returns scale.
		 * @return Requested value.
		 */
		float GetScale() const {
			return m_scale;
		}

		/**
		 * @brief Returns rotation.
		 * @return Requested value.
		 */
		float GetRotation() const {
			return m_rotation;
		}

		/**
		 * @brief Returns rotation mode.
		 * @return Requested value.
		 */
		RotationMode GetRotationMode() const {
			return m_rotationMode;
		}

		/**
		 * @brief Returns font.
		 * @return Requested value.
		 */
		Font* GetFont() const {
			return m_font;
		}

		/**
		 * @brief Renders this object.
		 * @param shaderProgram Parameter for shader program.
		 * @param projection Parameter for projection.
		 */
		void Render(GLuint shaderProgram, const glm::mat4& projection);

	private:

		/**
		 * @brief Performs setup rendering.
		 */
		void SetupRendering();

		/**
		 * @brief Performs cleanup rendering.
		 */
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

		/**
		 * @brief Performs instance.
		 * @return Result produced by this operation.
		 */
		static TextRenderer& Instance();

		/**
		 * @brief Initializes this object.
		 * @return True when the operation succeeds or the condition is met.
		 */
		bool Initialize();

		/**
		 * @brief Performs shutdown.
		 */
		void Shutdown();

		/**
		 * @brief Renders text.
		 * @param text Parameter for text.
		 * @param projection Parameter for projection.
		 */
		void RenderText(Text& text, const glm::mat4& projection);

		/**
		 * @brief Renders texts.
		 * @param texts Parameter for texts.
		 * @param projection Parameter for projection.
		 */
		void RenderTexts(const std::vector<Text*>& texts, const glm::mat4& projection);

	private:

		/**
		 * @brief Constructs a `TextRenderer` instance.
		 */
		TextRenderer() = default;

		/**
		 * @brief Destroys the `TextRenderer` instance and releases owned resources.
		 */
		~TextRenderer();

		/**
		 * @brief Constructs a `TextRenderer` instance.
		 */
		TextRenderer(const TextRenderer&) = delete;
		TextRenderer& operator=(const TextRenderer&) = delete;

		/**
		 * @brief Loads shaders.
		 * @return True when the operation succeeds or the condition is met.
		 */
		bool LoadShaders();

		/**
		 * @brief Performs compile shader.
		 * @param type Parameter for type.
		 * @param source Parameter for source.
		 * @return Result produced by this operation.
		 */
		GLuint CompileShader(GLenum type, const std::string& source);

		/**
		 * @brief Performs link program.
		 * @param vertexShader Parameter for vertex shader.
		 * @param fragmentShader Parameter for fragment shader.
		 * @return Result produced by this operation.
		 */
		GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);

		GLuint m_shaderProgram = 0;
		bool m_initialized = false;
	};

} // namespace FontSystem