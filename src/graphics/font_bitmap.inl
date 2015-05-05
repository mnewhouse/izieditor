/*
* The MIT License (MIT)
*
* IziEditor
* Copyright (c) 2015 Martin Newhouse
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#ifndef FONT_BITMAP_INL
#define FONT_BITMAP_INL

#include "font_bitmap.hpp"

namespace graphics
{
    template <typename OutputIt>
    void generate_text_vertices(const std::string& text, const FontBitmap& font_bitmap, OutputIt out,
        sf::Vector2f position, sf::Color color)
    {
        const auto& texture = font_bitmap.texture();
        float texture_height = static_cast<float>(texture.getSize().y);

        sf::Vertex vertex;
        vertex.color = color;

        for (std::uint8_t ch : text)
        {
            const auto& glyph = font_bitmap.glyph(ch);

            const float offset = static_cast<float>(glyph.offset);
            const float width = static_cast<float>(glyph.width);

            vertex.texCoords = { offset, 1.0f };
            vertex.position = position;
            *out = vertex; ++out;

            vertex.texCoords.y += texture_height;
            vertex.position.y += texture_height;
            *out = vertex; ++out;
            
            vertex.texCoords.x += width;
            vertex.position.x += width;
            *out = vertex; ++out;

            vertex.texCoords.y -= texture_height;
            vertex.position.y -= texture_height;
            *out = vertex; ++out;

            position.x = vertex.position.x;
        }
    }
}

#endif