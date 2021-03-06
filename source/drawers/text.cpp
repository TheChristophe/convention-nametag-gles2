#include "text.hpp"

#include <utility>

namespace Drawers {
    TextString::TextString(std::shared_ptr<Wrappers::Shader> shader, int width, int height, const char *text)
        : _shader(std::move(shader))
        , _scaleX{ 2.f / static_cast<float>(width) }
        , _scaleY{ 2.f / static_cast<float>(height) }
    {
        if (FT_Init_FreeType(&_freetype)) {
            throw std::runtime_error("could not load freetype");
        }

        if (FT_New_Face(_freetype, "res/ttf/DejaVuSans.ttf", 0, &_face)) {
            throw std::runtime_error("could not open font");
        }
        FT_Set_Pixel_Sizes(_face, 0, _fontSize);

        if (FT_Load_Char(_face, 'X', FT_LOAD_RENDER)) {
            throw std::runtime_error("could not load character");
        }

        glActiveTexture(GL_TEXTURE1);
        glGenTextures(1, &_texture.handle);
        glBindTexture(GL_TEXTURE_2D, _texture.handle);
        _shader->Use();
        _shader->Set("tex", 1);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glGenBuffers(1, &_text.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, _text.vbo);

        _posLoc = _shader->GetAttribLocation("pos");
        _texLoc = _shader->GetAttribLocation("texPos");

        //auto viewMatrix       = glm::lookAt(glm::vec3(0, 0, 1), glm::vec3(0, 0, -1), glm::vec3(0.0f, 1.0f, 0.0f));
        //auto projectionMatrix = glm::perspective(45.f, 2.f, 0.1f, 100.0f);
        //_shader.Set("projection", projectionMatrix * viewMatrix);

        LoadText(text);
    }

    void TextString::LoadText(const char *text)
    {
        // height above the baseline
        int glyphHeight{ 0 };
        // height below the baseline
        int glyphLowth{ 0 };

        int charCount{ 0 };

        for (const char *p = text; *p; p++) {
            if (FT_Load_Char(_face, *p, FT_LOAD_RENDER)) {
                throw std::runtime_error(std::string("failed to load \"") + text + std::string("\" due to '") + *p + std::string("'"));
            }

            auto &glyph = _face->glyph;

            // bitmap width minus horizontal offset
            _texture.width += glyph->bitmap.width;
            //
            glyphHeight = std::max(glyphHeight, glyph->bitmap_top);
            glyphLowth  = std::max(glyphLowth, static_cast<int>(glyph->bitmap.rows - glyph->bitmap_top));

            charCount++;
        }

        // create string texture buffer
        _texture.height = glyphHeight + glyphLowth;
        _texture.buffer = std::make_unique<uint8_t[]>(_texture.width * _texture.height);

        _text.charCount   = charCount;
        _text.quadCount   = charCount;
        _text.vertexCount = charCount * 4;

        _text.vertices = std::make_unique<TextVertex[]>(_text.vertexCount);

        // current x coordinate in texture buffer
        unsigned xOffset{ 0 };

        int i{ 0 };
        for (const char *p = text; *p; p++, i++) {
            // reload char bitmap
            FT_Load_Char(_face, *p, FT_LOAD_RENDER);

            auto &glyph = _face->glyph;

            // vertical offset for characters that are not full height
            const int yOffset = glyphHeight - glyph->bitmap_top;

            GLfloat xPos = static_cast<float>(xOffset) / static_cast<float>(_texture.width);

            // left vertices
            _text.vertices[i * 4]     = TextVertex{ xPos, 1, xPos, 0 };
            _text.vertices[i * 4 + 1] = TextVertex{ xPos, 0, xPos, 1 };

            // copy bitmaps to texture
            for (unsigned y = 0; y < glyph->bitmap.rows; y++) {
                for (unsigned x = 0; x < glyph->bitmap.width; x++) {
                    // destination buffer offset
                    const unsigned xCoord      = xOffset + x;
                    const unsigned yCoord      = yOffset + y;
                    const unsigned textureAddr = yCoord * _texture.width + xCoord;

                    // origin buffer offset
                    const int letterPixValue = glyph->bitmap.buffer[y * glyph->bitmap.width + x];

                    // add instead of set to avoid issues with overlapping letter bounding boxes
                    _texture.buffer[textureAddr] += letterPixValue;
                }
            }

            xOffset += glyph->bitmap.width;

            xPos = static_cast<float>(xOffset) / static_cast<float>(_texture.width);
            // right vertices
            _text.vertices[i * 4 + 2] = TextVertex{ xPos, 1, xPos, 0 };
            _text.vertices[i * 4 + 3] = TextVertex{ xPos, 0, xPos, 1 };
        }

        // load vertices to GL buffer
        glBindBuffer(GL_ARRAY_BUFFER, _text.vbo);
        glBufferData(GL_ARRAY_BUFFER, _text.vertexCount * sizeof(TextVertex), _text.vertices.get(), GL_STATIC_DRAW);

        // create texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _texture.width, _texture.height, 0,
            GL_LUMINANCE, GL_UNSIGNED_BYTE, _texture.buffer.get());
    }

    void TextString::Draw(float time)
    {
        _shader->Use();
        _shader->Set("time", time);
        _shader->Set("wavy", _properties._wavy);
        _shader->Set("offset", _at);

        glBindBuffer(GL_ARRAY_BUFFER, _text.vbo);
        glVertexAttribPointer(_posLoc, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), nullptr);
        glEnableVertexAttribArray(_posLoc);
        glVertexAttribPointer(_texLoc, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), reinterpret_cast<void *>(sizeof(GLfloat) * 2));
        glEnableVertexAttribArray(_texLoc);

        glBindTexture(GL_TEXTURE_2D, _texture.handle);

        for (int i = 0; i < _text.quadCount; i++) {
            glDrawArrays(GL_TRIANGLE_STRIP, i * 4, 4);
        }

        glDisableVertexAttribArray(_posLoc);
        glDisableVertexAttribArray(_texLoc);
    }

    void TextString::SetWavy(bool wavy)
    {
        _properties._wavy = wavy;
    }

    void TextString::MoveTo(glm::vec2 to)
    {
        _at = to;
    }
} // namespace Drawers