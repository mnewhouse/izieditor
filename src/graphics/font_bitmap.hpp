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

#ifndef FONT_BITMAP_HPP
#define FONT_BITMAP_HPP

#include <array>
#include <cstdint>
#include <exception>
#include <string>

#include <SFML/Graphics.hpp>

namespace graphics
{
    struct FontBitmapLoadError
        : public std::runtime_error
    {
        FontBitmapLoadError();
    };

    class FontBitmap
    {
    public:
        FontBitmap(const std::uint8_t* data, std::size_t data_size);        

        struct Glyph
        {
            std::uint32_t offset = 0;
            std::uint32_t width = 0;
        };

        const sf::Texture& texture() const;
        const Glyph& glyph(std::uint8_t character) const;

    private:
        sf::Texture texture_;
        std::array<Glyph, 256> glyph_mapping_;
    };

    template <typename OutputIt>
    void generate_text_vertices(const std::string& text, const FontBitmap& font_bitmap, OutputIt out,
        sf::Vector2f position = {}, sf::Color = sf::Color::White);
};

#include "font_bitmap.inl"

#endif