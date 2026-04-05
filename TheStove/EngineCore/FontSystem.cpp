/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			FontSystem.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (70%)
 CO-AUTHORS:        Yat Chun Wee, y.chunwee@digipen.edu		(20%)
					Vu Phan Hung, phanhung.vu@digipen.edu   (10%)

 DESCRIPTION:		Implementation of font system using FreeType for loading fonts
					and OpenGL for rendering text.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "EngineCore/FontSystem.hpp"
#include "EngineCore/Logger.hpp"
#include "EngineCore/Math.hpp"

namespace FontSystem {
	// ===========================
	// Font Implementation
	// ===========================

	/**
	 * @brief Destroys the font and releases all cached glyph textures.
	 */
	Font::~Font() {
		// Cleanup all character textures
		for (auto& pair : m_characters) {
			glDeleteTextures(1, &pair.second.textureID);
		}
		m_characters.clear();
	}

	/**
	 * @brief Loads glyph textures for this font at the requested size.
	 * @param fontPath Path to the font file.
	 * @param fontSize Requested pixel size for glyph generation.
	 * @return True if the font loaded successfully, otherwise false.
	 */
	bool Font::Load(const std::string& fontPath, unsigned int fontSize) {
		m_fontPath = fontPath;
		m_fontSize = fontSize;

		FT_Library library = FontManager::Instance().GetLibrary();
		if (!library) {
			TS_LOG_ERROR("[FontSystem::Font] FreeType library not initialized.");
			return false;
		}

		// Load font face
		FT_Face face;
		if (FT_New_Face(library, fontPath.c_str(), 0, &face)) {
			TS_LOG_ERROR("[FontSystem::Font] Failed to load font: " << fontPath);
			return false;
		}

		// Set font size (width=0 means dynamically calculated)
		FT_Set_Pixel_Sizes(face, 0, fontSize);

		// Glyph bitmaps are single-channel, so use byte alignment that matches the glyph buffer layout.
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		// Preload the basic ASCII range so common debug/editor text is always available.
		for (unsigned char c = 0; c < 128; c++) {
			// Load character glyph
			if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
				TS_LOG_WARN("[FontSystem::Font] Failed to load glyph for character: " << static_cast<int>(c));
				continue;
			}

			// Upload each rendered glyph bitmap into its own OpenGL texture.
			GLuint texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RED,
				face->glyph->bitmap.width,
				face->glyph->bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				face->glyph->bitmap.buffer
			);

			// Set texture options
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			// Cache the glyph metrics and texture handle for later text layout and rendering.
			Character character = {
				texture,
				glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
				glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
				static_cast<GLuint>(face->glyph->advance.x)
			};
			m_characters.insert(std::pair<char, Character>(c, character));
		}

		glBindTexture(GL_TEXTURE_2D, 0);

		// Cleanup FreeType face
		FT_Done_Face(face);

		TS_LOG_INFO("[FontSystem::Font] Loaded font: " << fontPath << " (size: " << fontSize << ")");
		return true;
	}

	/**
	 * @brief Returns glyph data for a single character.
	 * @param c ASCII character to look up.
	 * @return Pointer to the glyph data, or nullptr if unavailable.
	 */
	const Character* Font::GetCharacter(char c) const {
		auto it = m_characters.find(c);
		if (it != m_characters.end()) {
			return &it->second;
		}
		return nullptr;
	}

	// ===========================
	// FontManager Implementation
	// ===========================

	/**
	 * @brief Returns the global FontManager singleton.
	 * @return Reference to the shared FontManager instance.
	 */
	FontManager& FontManager::Instance() {
		static FontManager instance;
		return instance;
	}

	/**
	 * @brief Destroys the FontManager and releases loaded resources.
	 */
	FontManager::~FontManager() {
		Shutdown();
	}

	/**
	 * @brief Initializes the FreeType library used by the font system.
	 * @return True if initialization succeeded, otherwise false.
	 */
	bool FontManager::Initialize() {
		if (m_initialized) {
			TS_LOG_DEBUG("[FontSystem::FontManager] Already initialized.");
			return true;
		}

		// Initialize FreeType library
		if (FT_Init_FreeType(&m_library)) {
			TS_LOG_ERROR("[FontSystem::FontManager] Failed to initialize FreeType library.");
			return false;
		}

		m_initialized = true;
		TS_LOG_INFO("[FontSystem::FontManager] Initialized successfully.");
		return true;
	}

	/**
	 * @brief Shuts down the font manager and releases all loaded fonts.
	 */
	void FontManager::Shutdown() {
		if (!m_initialized)
			return;

		// Clear all fonts (this will delete textures)
		m_fonts.clear();

		// Done with FreeType
		if (m_library) {
			FT_Done_FreeType(m_library);
			m_library = nullptr;
		}

		m_initialized = false;
		TS_LOG_INFO("[FontSystem::FontManager] Shutdown complete.");
	}

	/**
	 * @brief Loads a named font into the manager.
	 * @param name Logical font name used for lookup.
	 * @param fontPath Path to the font file.
	 * @param fontSize Requested pixel size.
	 * @return Pointer to the loaded font, or nullptr on failure.
	 */
	Font* FontManager::LoadFont(const std::string& name, const std::string& fontPath, unsigned int fontSize) {
		if (!m_initialized) {
			TS_LOG_ERROR("[FontSystem::FontManager] Cannot load font, not initialized.");
			return nullptr;
		}

		// Check if font already loaded
		auto it = m_fonts.find(name);
		if (it != m_fonts.end()) {
			TS_LOG_DEBUG("[FontSystem::FontManager] Font '" << name << "' already loaded.");
			return it->second.get();
		}

		// Create a new Font object only when the name has not already been registered.
		auto font = std::make_unique<Font>();
		if (!font->Load(fontPath, fontSize)) {
			TS_LOG_ERROR("[FontSystem::FontManager] Failed to load font '" << name << "' from: " << fontPath);
			return nullptr;
		}

		// Transfer ownership to the manager while returning a raw pointer for immediate use.
		Font* fontPtr = font.get();
		m_fonts[name] = std::move(font);
		return fontPtr;
	}

	/**
	 * @brief Retrieves a previously loaded font by name.
	 * @param name Logical font name.
	 * @return Pointer to the font, or nullptr if not found.
	 */
	Font* FontManager::GetFont(const std::string& name) {
		auto it = m_fonts.find(name);
		if (it != m_fonts.end()) {
			return it->second.get();
		}
		return nullptr;
	}

	// ===========================
	// Text Implementation
	// ===========================

	/**
	 * @brief Constructs a text object with deferred OpenGL setup.
	 */
	Text::Text() {
		// Don't call SetupRendering() here - OpenGL may not be initialized yet
		// SetupRendering will be called lazily in Render() when needed
	}

	/**
	 * @brief Destroys the text object and releases its render buffers.
	 */
	Text::~Text() {
		CleanupRendering();
	}

	/**
	 * @brief Sets the font used by this text object.
	 * @param font Font pointer used for glyph lookup.
	 */
	void Text::SetFont(Font* font) {
		// Swap the font reference used for all subsequent glyph lookups.
		m_font = font;
	}

	/**
	 * @brief Sets the string content to render.
	 * @param text Text string to display.
	 */
	void Text::SetText(const std::string& text) {
		// Store the raw string so layout can be recomputed during the next render.
		m_text = text;
	}

	/**
	 * @brief Sets the screen-space position of the text object.
	 * @param position New text position.
	 */
	void Text::SetPosition(const glm::vec2& position) {
		// Cache the new origin used when generating glyph quads.
		m_position = position;
	}

	/**
	 * @brief Sets the tint color used to render the text.
	 * @param color New RGBA color.
	 */
	void Text::SetColor(const glm::vec4& color) {
		// Cache the color that will be pushed into the text shader uniform.
		m_color = color;
	}

	/**
	 * @brief Sets the glyph scale factor used during rendering.
	 * @param scale New text scale.
	 */
	void Text::SetScale(float scale) {
		// Cache the per-glyph scale factor for future layout and rendering.
		m_scale = scale;
	}

	/**
	 * @brief Sets the text rotation in degrees.
	 * @param degrees Rotation angle in degrees.
	 */
	void Text::SetRotation(float degrees) {
		// Store the rotation angle; actual trig work happens during Render().
		m_rotation = degrees;
	}

	/**
	 * @brief Sets how rotation is applied to the text.
	 * @param mode Rotation mode to use.
	 */
	void Text::SetRotationMode(RotationMode mode) {
		// Switch between per-character rotation and whole-block rotation behavior.
		m_rotationMode = mode;
	}

	/**
	 * @brief Sets the horizontal alignment used for line layout.
	 * @param align Horizontal alignment mode.
	 */
	void Text::SetHorizontalAlign(HorizontalAlign align) {
		// Cache the alignment so line offsets can be computed during rendering.
		m_horizontalAlign = align;
	}

	/**
	 * @brief Creates the VAO and VBO required to render text quads.
	 */
	void Text::SetupRendering() {
		if (m_renderingSetup)
			return;

		// Allocate a reusable quad buffer that will be updated once per glyph draw.
		glGenVertexArrays(1, &m_VAO);
		glGenBuffers(1, &m_VBO);

		glBindVertexArray(m_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

		// Allocate memory for quad (6 vertices * 4 floats per vertex)
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		m_renderingSetup = true;
	}

	/**
	 * @brief Releases the OpenGL buffers used by this text object.
	 */
	void Text::CleanupRendering() {
		if (!m_renderingSetup)
			return;

		if (m_VBO)
			glDeleteBuffers(1, &m_VBO);
		if (m_VAO)
			glDeleteVertexArrays(1, &m_VAO);

		m_VAO = 0;
		m_VBO = 0;
		m_renderingSetup = false;
	}

	/**
	 * @brief Renders the text object using the supplied shader and projection matrix.
	 * @param shaderProgram OpenGL shader program used for text rendering.
	 * @param projection Projection matrix used for screen-space rendering.
	 */
	void Text::Render(GLuint shaderProgram, const glm::mat4& projection) {
		// Lazy initialization - setup rendering on first render call when OpenGL is ready
		if (!m_renderingSetup)
			SetupRendering();

		if (!m_font || m_text.empty() || !m_renderingSetup)
			return;

		// Save blend state so text rendering can temporarily force alpha blending.
		GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
		GLint prevBlendSrc, prevBlendDst;
		glGetIntegerv(GL_BLEND_SRC_ALPHA, &prevBlendSrc);
		glGetIntegerv(GL_BLEND_DST_ALPHA, &prevBlendDst);

		// Use shader
		glUseProgram(shaderProgram);

		// Set uniforms
		GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
		GLint colorLoc = glGetUniformLocation(shaderProgram, "textColor");

		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
		glUniform4fv(colorLoc, 1, glm::value_ptr(m_color));

		// Enable blending for text transparency
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Disable depth testing so text overlays are not clipped by scene geometry.
		GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
		glDisable(GL_DEPTH_TEST);

		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(m_VAO);

		// Calculate rotation in radians
		float rotRad = Math::ToRadians(m_rotation);
		float cosR = std::cos(rotRad);
		float sinR = std::sin(rotRad);

		// First measure each line so alignment offsets can be computed before drawing.
		float cursorX = 0.0f;
		float cursorY = 0.0f;
		const float lineAdvance = static_cast<float>(m_font->GetFontSize()) * m_scale * 1.2f;
		std::vector<float> lineWidths(1, 0.0f);

		for (char c : m_text) {
			if (c == '\r') {
				continue;
			}

			if (c == '\n') {
				lineWidths.emplace_back(0.0f);
				continue;
			}

			const Character* ch = m_font->GetCharacter(c);
			if (!ch) {
				continue;
			}

			lineWidths.back() += static_cast<float>(ch->advance >> 6) * m_scale;
		}

		auto GetLineOffset = [this, &lineWidths](size_t lineIndex) {
			if (lineIndex >= lineWidths.size()) {
				return 0.0f;
			}

			switch (m_horizontalAlign) {
			case HorizontalAlign::Center:
				return -lineWidths[lineIndex] * 0.5f;
			case HorizontalAlign::Right:
				return -lineWidths[lineIndex];
			case HorizontalAlign::Left:
			default:
				return 0.0f;
			}
			};
		size_t lineIndex = 0;

		// Draw each glyph by updating the shared quad buffer and submitting one textured quad.
		for (char c : m_text) {
			if (c == '\r') {
				continue;
			}

			if (c == '\n') {
				cursorX = 0.0f;
				cursorY += lineAdvance;
				++lineIndex;
				continue;
			}

			const Character* ch = m_font->GetCharacter(c);
			if (!ch)
				continue;

			const float lineOffsetX = GetLineOffset(lineIndex);

			// Convert glyph metrics into the top-left-origin screen-space convention used here.
			float xpos = lineOffsetX + cursorX + ch->bearing.x * m_scale;
			float ypos = cursorY - ch->bearing.y * m_scale;

			float w = ch->size.x * m_scale;
			float h = ch->size.y * m_scale;

			// Define quad vertices in local space (relative to cursor)
			struct Vertex {
				float x, y;
			};

			Vertex vertices[6] = {
				{ xpos,     ypos },      // top-left
				{ xpos,     ypos + h },  // bottom-left
				{ xpos + w, ypos + h },  // bottom-right

				{ xpos,     ypos },      // top-left
				{ xpos + w, ypos + h },  // bottom-right
				{ xpos + w, ypos }       // top-right
			};

			// Generate final quad vertices by applying the requested rotation mode.
			float finalVertices[6][4];

			if (m_rotationMode == RotationMode::PerCharacter) {
				// Per-character rotation: rotate each character individually
				for (int i = 0; i < 6; i++) {
					// Rotate around origin
					float rx = vertices[i].x * cosR - vertices[i].y * sinR;
					float ry = vertices[i].x * sinR + vertices[i].y * cosR;

					// Translate to world position
					finalVertices[i][0] = rx + m_position.x;
					finalVertices[i][1] = ry + m_position.y;

					// UV coordinates
					if (i == 0 || i == 3)      // top-left
					{
						finalVertices[i][2] = 0.0f; finalVertices[i][3] = 0.0f;
					}
					else if (i == 1)           // bottom-left
					{
						finalVertices[i][2] = 0.0f; finalVertices[i][3] = 1.0f;
					}
					else if (i == 2 || i == 4) // bottom-right
					{
						finalVertices[i][2] = 1.0f; finalVertices[i][3] = 1.0f;
					}
					else                       // top-right
					{
						finalVertices[i][2] = 1.0f; finalVertices[i][3] = 0.0f;
					}
				}

				// Advance along the rotated axis so the string itself bends/turns character by character.
				float advanceX = (ch->advance >> 6) * m_scale;
				cursorX += advanceX * cosR;
				cursorY += advanceX * sinR;
			}
			else // RotationMode::Block
			{
				// Keep layout straight, then rotate the whole block as a single unit.
				for (int i = 0; i < 6; i++) {
					// First rotate the vertices around origin
					float rx = vertices[i].x * cosR - vertices[i].y * sinR;
					float ry = vertices[i].x * sinR + vertices[i].y * cosR;

					// Then translate to world position
					finalVertices[i][0] = rx + m_position.x;
					finalVertices[i][1] = ry + m_position.y;

					// UV coordinates
					if (i == 0 || i == 3)      // top-left
					{
						finalVertices[i][2] = 0.0f; finalVertices[i][3] = 0.0f;
					}
					else if (i == 1)           // bottom-left
					{
						finalVertices[i][2] = 0.0f; finalVertices[i][3] = 1.0f;
					}
					else if (i == 2 || i == 4) // bottom-right
					{
						finalVertices[i][2] = 1.0f; finalVertices[i][3] = 1.0f;
					}
					else                       // top-right
					{
						finalVertices[i][2] = 1.0f; finalVertices[i][3] = 0.0f;
					}
				}

				// Advance in straight text space before the final block rotation is applied.
				float advanceX = (ch->advance >> 6) * m_scale;
				cursorX += advanceX;
				// cursorY stays 0 for block mode
			}

			// Render glyph texture over quad
			glBindTexture(GL_TEXTURE_2D, ch->textureID);

			// Stream the current glyph quad into the dynamic vertex buffer.
			glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(finalVertices), finalVertices);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			// Render quad
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Restore depth state
		if (depthWasEnabled) {
			glEnable(GL_DEPTH_TEST);
		}

		// Restore the previous blend configuration so surrounding render code is unaffected.
		if (!blendWasEnabled) {
			glDisable(GL_BLEND);
		}
		else {
			glBlendFunc(prevBlendSrc, prevBlendDst);
		}
	}

	// ===========================
	// TextRenderer Implementation
	// ===========================

	/**
	 * @brief Returns the global TextRenderer singleton.
	 * @return Reference to the shared TextRenderer instance.
	 */
	TextRenderer& TextRenderer::Instance() {
		static TextRenderer instance;
		return instance;
	}

	/**
	 * @brief Destroys the text renderer and releases its shader program.
	 */
	TextRenderer::~TextRenderer() {
		Shutdown();
	}

	/**
	 * @brief Initializes the text renderer and loads its shaders.
	 * @return True if initialization succeeded, otherwise false.
	 */
	bool TextRenderer::Initialize() {
		if (m_initialized) {
			TS_LOG_DEBUG("[FontSystem::TextRenderer] Already initialized.");
			return true;
		}

		if (!LoadShaders()) {
			TS_LOG_ERROR("[FontSystem::TextRenderer] Failed to load shaders.");
			return false;
		}

		m_initialized = true;
		TS_LOG_INFO("[FontSystem::TextRenderer] Initialized successfully.");
		return true;
	}

	/**
	 * @brief Shuts down the text renderer and releases its shader resources.
	 */
	void TextRenderer::Shutdown() {
		if (!m_initialized)
			return;

		if (m_shaderProgram) {
			glDeleteProgram(m_shaderProgram);
			m_shaderProgram = 0;
		}

		m_initialized = false;
		TS_LOG_INFO("[FontSystem::TextRenderer] Shutdown complete.");
	}

	/**
	 * @brief Renders a single text object.
	 * @param text Text object to render.
	 * @param projection Projection matrix used for rendering.
	 */
	void TextRenderer::RenderText(Text& text, const glm::mat4& projection) {
		if (!m_initialized || !m_shaderProgram)
			return;

		// Forward rendering to the Text object once the shared shader is ready.
		text.Render(m_shaderProgram, projection);
	}

	/**
	 * @brief Renders a collection of text objects.
	 * @param texts Text objects to render.
	 * @param projection Projection matrix used for rendering.
	 */
	void TextRenderer::RenderTexts(const std::vector<Text*>& texts, const glm::mat4& projection) {
		if (!m_initialized || !m_shaderProgram)
			return;

		for (Text* text : texts) {
			if (text) {
				text->Render(m_shaderProgram, projection);
			}
		}
	}

	/**
	 * @brief Loads and links the shader program used for text rendering.
	 * @return True if the shader program was created successfully, otherwise false.
	 */
	bool TextRenderer::LoadShaders() {
		// Use a minimal shader pair that samples a glyph atlas and tints it with a uniform color.
		const char* vertexShaderSource = R"(
			#version 330 core
			layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
			out vec2 TexCoords;

			uniform mat4 projection;

			void main()
			{
				gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
				TexCoords = vertex.zw;
			}
		)";

		// Fragment shader source
		const char* fragmentShaderSource = R"(
			#version 330 core
			in vec2 TexCoords;
			out vec4 color;

			uniform sampler2D text;
			uniform vec4 textColor;

			void main()
			{
				vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
				color = textColor * sampled;
			}
		)";

		GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
		if (!vertexShader)
			return false;

		GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
		if (!fragmentShader) {
			glDeleteShader(vertexShader);
			return false;
		}

		m_shaderProgram = LinkProgram(vertexShader, fragmentShader);

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		return m_shaderProgram != 0;
	}

	/**
	 * @brief Compiles an OpenGL shader from GLSL source.
	 * @param type Shader type such as `GL_VERTEX_SHADER`.
	 * @param source GLSL source code.
	 * @return Compiled shader handle, or `0` on failure.
	 */
	GLuint TextRenderer::CompileShader(GLenum type, const std::string& source) {
		GLuint shader = glCreateShader(type);
		const char* src = source.c_str();
		glShaderSource(shader, 1, &src, nullptr);
		glCompileShader(shader);

		GLint success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			char infoLog[512];
			glGetShaderInfoLog(shader, 512, nullptr, infoLog);
			TS_LOG_ERROR("[FontSystem::TextRenderer] Shader compilation failed: " << infoLog);
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}

	/**
	 * @brief Links a vertex and fragment shader into an OpenGL program.
	 * @param vertexShader Compiled vertex shader handle.
	 * @param fragmentShader Compiled fragment shader handle.
	 * @return Linked program handle, or `0` on failure.
	 */
	GLuint TextRenderer::LinkProgram(GLuint vertexShader, GLuint fragmentShader) {
		GLuint program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);

		GLint success;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success) {
			char infoLog[512];
			glGetProgramInfoLog(program, 512, nullptr, infoLog);
			TS_LOG_ERROR("[FontSystem::TextRenderer] Program linking failed: " << infoLog);
			glDeleteProgram(program);
			return 0;
		}
		return program;
	}

} // namespace FontSystem
