/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			FontSystem.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (70%)
 CO-AUTHOR:         Yat Chun Wee, y.chunwee@digipen.edu		(30%)

 DESCRIPTION:		Font system using FreeType to load TTF fonts, render text with
					different fonts at different positions using OpenGL.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
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
		 * @brief Loads glyph textures for this font at the requested size.
		 * @param fontPath Path to the font file.
		 * @param fontSize Requested pixel height for glyph generation.
		 * @return True if the font loaded successfully, otherwise false.
		 */
		bool Load(const std::string& fontPath, unsigned int fontSize);

		/**
		 * @brief Returns glyph data for a single character.
		 * @param c ASCII character to look up.
		 * @return Pointer to the glyph data, or nullptr if unavailable.
		 */
		const Character* GetCharacter(char c) const;

		/**
		 * @brief Returns the loaded font size in pixels.
		 * @return Font size in pixels.
		 */
		unsigned int GetFontSize() const {
			// Expose the cached font size so layout code can compute line spacing.
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
		 * @brief Returns the global FontManager singleton.
		 * @return Reference to the shared FontManager instance.
		 */
		static FontManager& Instance();

		/**
		 * @brief Initializes the FreeType library used by the font system.
		 * @return True if initialization succeeded, otherwise false.
		 */
		bool Initialize();

		/**
		 * @brief Shuts down the font system and releases loaded fonts.
		 */
		void Shutdown();

		/**
		 * @brief Loads a named font into the manager.
		 * @param name Logical font name used for lookup.
		 * @param fontPath Path to the font file.
		 * @param fontSize Requested pixel size.
		 * @return Pointer to the loaded font, or nullptr on failure.
		 */
		Font* LoadFont(const std::string& name, const std::string& fontPath, unsigned int fontSize);

		/**
		 * @brief Retrieves a previously loaded font by name.
		 * @param name Logical font name.
		 * @return Pointer to the font, or nullptr if not found.
		 */
		Font* GetFont(const std::string& name);

		/**
		 * @brief Returns the raw FreeType library handle.
		 * @return The active `FT_Library` handle.
		 */
		FT_Library GetLibrary() const {
			// Provide low-level access for font-loading helpers that need the shared library handle.
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
		 * @brief Disables copying of the `FontManager` singleton.
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

		enum class HorizontalAlign {
			Left,
			Center,
			Right
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
		 * @brief Sets the font used to render this text object.
		 * @param font Font pointer to use for glyph lookup.
		 */
		void SetFont(Font* font);

		/**
		 * @brief Sets the string content to render.
		 * @param text Text string to display.
		 */
		void SetText(const std::string& text);

		/**
		 * @brief Sets the screen-space position of the text object.
		 * @param position New text position.
		 */
		void SetPosition(const glm::vec2& position);

		/**
		 * @brief Sets the tint color used to render the text.
		 * @param color RGBA color tint.
		 */
		void SetColor(const glm::vec4& color);

		/**
		 * @brief Sets the render scale applied to glyphs.
		 * @param scale New text scale.
		 */
		void SetScale(float scale);

		/**
		 * @brief Sets the text rotation in degrees.
		 * @param degrees Rotation amount in degrees.
		 */
		void SetRotation(float degrees);

		/**
		 * @brief Sets how rotation is applied to the text.
		 * @param mode Rotation mode to use.
		 */
		void SetRotationMode(RotationMode mode);

		/**
		 * @brief Sets the horizontal alignment used during text layout.
		 * @param align Horizontal alignment mode.
		 */
		void SetHorizontalAlign(HorizontalAlign align);

		/**
		 * @brief Returns the current text string.
		 * @return Const reference to the text string.
		 */
		const std::string& GetText() const {
			// Expose the stored string so UI tools can inspect the current text content.
			return m_text;
		}

		/**
		 * @brief Returns the current text position.
		 * @return Const reference to the text position.
		 */
		const glm::vec2& GetPosition() const {
			// Expose the cached position used during rendering.
			return m_position;
		}

		/**
		 * @brief Returns the current text tint color.
		 * @return Const reference to the text color.
		 */
		const glm::vec4& GetColor() const {
			// Expose the cached color used by the text shader.
			return m_color;
		}

		/**
		 * @brief Returns the current text scale.
		 * @return Current scale factor.
		 */
		float GetScale() const {
			// Expose the cached glyph scaling factor.
			return m_scale;
		}

		/**
		 * @brief Returns the current text rotation in degrees.
		 * @return Rotation in degrees.
		 */
		float GetRotation() const {
			// Expose the cached rotation value used during vertex generation.
			return m_rotation;
		}

		/**
		 * @brief Returns the current rotation mode.
		 * @return Active rotation mode.
		 */
		RotationMode GetRotationMode() const {
			// Expose how rotation is currently being applied to the text object.
			return m_rotationMode;
		}

		/**
		 * @brief Returns the font currently assigned to this text object.
		 * @return Pointer to the active font, or nullptr if none is assigned.
		 */
		Font* GetFont() const {
			// Expose the font pointer so external layout and debug code can inspect it.
			return m_font;
		}

		/**
		 * @brief Renders the text object using the supplied shader and projection matrix.
		 * @param shaderProgram OpenGL shader program used for text rendering.
		 * @param projection Projection matrix for screen-space rendering.
		 */
		void Render(GLuint shaderProgram, const glm::mat4& projection);

	private:
		/**
		 * @brief Creates the OpenGL buffers required for text rendering.
		 */
		void SetupRendering();

		/**
		 * @brief Releases the OpenGL buffers used for text rendering.
		 */
		void CleanupRendering();

		Font* m_font = nullptr;
		std::string m_text;
		glm::vec2 m_position = glm::vec2(0.0f, 0.0f);
		glm::vec4 m_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		float m_scale = 1.0f;
		float m_rotation = 0.0f;  // Rotation in degrees
		RotationMode m_rotationMode = RotationMode::Block;  // Default to block rotation
		HorizontalAlign m_horizontalAlign = HorizontalAlign::Left;

		// OpenGL rendering resources
		GLuint m_VAO = 0;
		GLuint m_VBO = 0;
		bool m_renderingSetup = false;
	};

	// TextRenderer - manages shader and renders all text objects
	class TextRenderer {
	public:
		/**
		 * @brief Returns the global TextRenderer singleton.
		 * @return Reference to the shared TextRenderer instance.
		 */
		static TextRenderer& Instance();

		/**
		 * @brief Initializes the text renderer and its shader program.
		 * @return True if initialization succeeded, otherwise false.
		 */
		bool Initialize();

		/**
		 * @brief Shuts down the text renderer and releases its shader program.
		 */
		void Shutdown();

		/**
		 * @brief Renders a single text object.
		 * @param text Text object to render.
		 * @param projection Projection matrix used for rendering.
		 */
		void RenderText(Text& text, const glm::mat4& projection);

		/**
		 * @brief Renders a list of text objects.
		 * @param texts Text objects to render.
		 * @param projection Projection matrix used for rendering.
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
		 * @brief Disables copying of the `TextRenderer` singleton.
		 */
		TextRenderer(const TextRenderer&) = delete;
		TextRenderer& operator=(const TextRenderer&) = delete;

		/**
		 * @brief Loads and links the shader program used for text rendering.
		 * @return True if shader creation succeeded, otherwise false.
		 */
		bool LoadShaders();

		/**
		 * @brief Compiles an OpenGL shader from source.
		 * @param type Shader type such as `GL_VERTEX_SHADER`.
		 * @param source GLSL source code to compile.
		 * @return Compiled shader handle, or `0` on failure.
		 */
		GLuint CompileShader(GLenum type, const std::string& source);

		/**
		 * @brief Links a vertex and fragment shader into a program.
		 * @param vertexShader Compiled vertex shader handle.
		 * @param fragmentShader Compiled fragment shader handle.
		 * @return Linked program handle, or `0` on failure.
		 */
		GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);

		GLuint m_shaderProgram = 0;
		bool m_initialized = false;
	};

} // namespace FontSystem
