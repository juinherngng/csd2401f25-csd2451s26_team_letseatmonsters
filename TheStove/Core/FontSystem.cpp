/*
----------------------------------------------------------------------------------------------------
FILE NAME:			FontSystem.cpp
PROJECT NAME:		Project GAM200
AUTHOR:			Font System Implementation

DESCRIPTION:		Implementation of font system using FreeType for loading fonts
				and OpenGL for rendering text.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "FontSystem.hpp"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace FontSystem
{
	// ===========================
	// Font Implementation
	// ===========================

	Font::~Font()
	{
		// Cleanup all character textures
		for (auto& pair : m_characters)
		{
			glDeleteTextures(1, &pair.second.textureID);
		}
		m_characters.clear();
	}

	bool Font::Load(const std::string& fontPath, unsigned int fontSize)
	{
		m_fontPath = fontPath;
		m_fontSize = fontSize;

		FT_Library library = FontManager::Instance().GetLibrary();
		if (!library)
		{
			std::cerr << "FontSystem::Font - FreeType library not initialized!" << std::endl;
			return false;
		}

		// Load font face
		FT_Face face;
		if (FT_New_Face(library, fontPath.c_str(), 0, &face))
		{
			std::cerr << "FontSystem::Font - Failed to load font: " << fontPath << std::endl;
			return false;
		}

		// Set font size (width=0 means dynamically calculated)
		FT_Set_Pixel_Sizes(face, 0, fontSize);

		// Disable byte-alignment restriction
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		// Load first 128 ASCII characters
		for (unsigned char c = 0; c < 128; c++)
		{
			// Load character glyph
			if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			{
				std::cerr << "FontSystem::Font - Failed to load glyph for character: " << c << std::endl;
				continue;
			}

			// Generate texture
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

			// Store character
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

		std::cout << "FontSystem::Font - Loaded font: " << fontPath << " (size: " << fontSize << ")" << std::endl;
		return true;
	}

	const Character* Font::GetCharacter(char c) const
	{
		auto it = m_characters.find(c);
		if (it != m_characters.end())
		{
			return &it->second;
		}
		return nullptr;
	}

	// ===========================
	// FontManager Implementation
	// ===========================

	FontManager& FontManager::Instance()
	{
		static FontManager instance;
		return instance;
	}

	FontManager::~FontManager()
	{
		Shutdown();
	}

	bool FontManager::Initialize()
	{
		if (m_initialized)
		{
			std::cout << "FontSystem::FontManager - Already initialized." << std::endl;
			return true;
		}

		// Initialize FreeType library
		if (FT_Init_FreeType(&m_library))
		{
			std::cerr << "FontSystem::FontManager - Failed to initialize FreeType library!" << std::endl;
			return false;
		}

		m_initialized = true;
		std::cout << "FontSystem::FontManager - Initialized successfully." << std::endl;
		return true;
	}

	void FontManager::Shutdown()
	{
		if (!m_initialized)
			return;

		// Clear all fonts (this will delete textures)
		m_fonts.clear();

		// Done with FreeType
		if (m_library)
		{
			FT_Done_FreeType(m_library);
			m_library = nullptr;
		}

		m_initialized = false;
		std::cout << "FontSystem::FontManager - Shutdown complete." << std::endl;
	}

	Font* FontManager::LoadFont(const std::string& name, const std::string& fontPath, unsigned int fontSize)
	{
		if (!m_initialized)
		{
			std::cerr << "FontSystem::FontManager - Cannot load font, not initialized!" << std::endl;
			return nullptr;
		}

		// Check if font already loaded
		auto it = m_fonts.find(name);
		if (it != m_fonts.end())
		{
			std::cout << "FontSystem::FontManager - Font '" << name << "' already loaded." << std::endl;
			return it->second.get();
		}

		// Create new font
		auto font = std::make_unique<Font>();
		if (!font->Load(fontPath, fontSize))
		{
			std::cerr << "FontSystem::FontManager - Failed to load font '" << name << "' from: " << fontPath << std::endl;
			return nullptr;
		}

		// Store and return
		Font* fontPtr = font.get();
		m_fonts[name] = std::move(font);
		return fontPtr;
	}

	Font* FontManager::GetFont(const std::string& name)
	{
		auto it = m_fonts.find(name);
		if (it != m_fonts.end())
		{
			return it->second.get();
		}
		return nullptr;
	}

	// ===========================
	// Text Implementation
	// ===========================

	Text::Text()
	{
		SetupRendering();
	}

	Text::~Text()
	{
		CleanupRendering();
	}

	void Text::SetFont(Font* font)
	{
		m_font = font;
	}

	void Text::SetText(const std::string& text)
	{
		m_text = text;
	}

	void Text::SetPosition(const glm::vec2& position)
	{
		m_position = position;
	}

	void Text::SetColor(const glm::vec4& color)
	{
		m_color = color;
	}

	void Text::SetScale(float scale)
	{
		m_scale = scale;
	}

	void Text::SetupRendering()
	{
		if (m_renderingSetup)
			return;

		// Create VAO and VBO for rendering text quads
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

	void Text::CleanupRendering()
	{
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

	void Text::Render(GLuint shaderProgram, const glm::mat4& projection)
	{
		if (!m_font || m_text.empty() || !m_renderingSetup)
			return;

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

		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(m_VAO);

		// Current position for rendering
		float x = m_position.x;
		float y = m_position.y;

		// Iterate through all characters
		for (char c : m_text)
		{
			const Character* ch = m_font->GetCharacter(c);
			if (!ch)
				continue;

			float xpos = x + ch->bearing.x * m_scale;
			float ypos = y - (ch->size.y - ch->bearing.y) * m_scale;

			float w = ch->size.x * m_scale;
			float h = ch->size.y * m_scale;

			// Update VBO for each character
			float vertices[6][4] = {
				{ xpos,     ypos + h,   0.0f, 0.0f },
				{ xpos,     ypos,       0.0f, 1.0f },
				{ xpos + w, ypos,       1.0f, 1.0f },

				{ xpos,     ypos + h,   0.0f, 0.0f },
				{ xpos + w, ypos,       1.0f, 1.0f },
				{ xpos + w, ypos + h,   1.0f, 0.0f }
			};

			// Render glyph texture over quad
			glBindTexture(GL_TEXTURE_2D, ch->textureID);
			
			// Update content of VBO memory
			glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			
			// Render quad
			glDrawArrays(GL_TRIANGLES, 0, 6);
			
			// Advance cursor for next glyph (advance is in 1/64th pixels)
			x += (ch->advance >> 6) * m_scale;
		}

		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_BLEND);
	}

	// ===========================
	// TextRenderer Implementation
	// ===========================

	TextRenderer& TextRenderer::Instance()
	{
		static TextRenderer instance;
		return instance;
	}

	TextRenderer::~TextRenderer()
	{
		Shutdown();
	}

	bool TextRenderer::Initialize()
	{
		if (m_initialized)
		{
			std::cout << "FontSystem::TextRenderer - Already initialized." << std::endl;
			return true;
		}

		if (!LoadShaders())
		{
			std::cerr << "FontSystem::TextRenderer - Failed to load shaders!" << std::endl;
			return false;
		}

		m_initialized = true;
		std::cout << "FontSystem::TextRenderer - Initialized successfully." << std::endl;
		return true;
	}

	void TextRenderer::Shutdown()
	{
		if (!m_initialized)
			return;

		if (m_shaderProgram)
		{
			glDeleteProgram(m_shaderProgram);
			m_shaderProgram = 0;
		}

		m_initialized = false;
		std::cout << "FontSystem::TextRenderer - Shutdown complete." << std::endl;
	}

	void TextRenderer::RenderText(Text& text, const glm::mat4& projection)
	{
		if (!m_initialized || !m_shaderProgram)
			return;

		text.Render(m_shaderProgram, projection);
	}

	void TextRenderer::RenderTexts(const std::vector<Text*>& texts, const glm::mat4& projection)
	{
		if (!m_initialized || !m_shaderProgram)
			return;

		for (Text* text : texts)
		{
			if (text)
			{
				text->Render(m_shaderProgram, projection);
			}
		}
	}

	bool TextRenderer::LoadShaders()
	{
		// Vertex shader source
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
		if (!fragmentShader)
		{
			glDeleteShader(vertexShader);
			return false;
		}

		m_shaderProgram = LinkProgram(vertexShader, fragmentShader);

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		return m_shaderProgram != 0;
	}

	GLuint TextRenderer::CompileShader(GLenum type, const std::string& source)
	{
		GLuint shader = glCreateShader(type);
		const char* src = source.c_str();
		glShaderSource(shader, 1, &src, nullptr);
		glCompileShader(shader);

		GLint success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			char infoLog[512];
			glGetShaderInfoLog(shader, 512, nullptr, infoLog);
			std::cerr << "FontSystem::TextRenderer - Shader compilation failed: " << infoLog << std::endl;
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}

	GLuint TextRenderer::LinkProgram(GLuint vertexShader, GLuint fragmentShader)
	{
		GLuint program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);

		GLint success;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success)
		{
			char infoLog[512];
			glGetProgramInfoLog(program, 512, nullptr, infoLog);
			std::cerr << "FontSystem::TextRenderer - Program linking failed: " << infoLog << std::endl;
			glDeleteProgram(program);
			return 0;
		}
		return program;
	}

} // namespace FontSystem
