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

#ifndef PATTERN_HPP
#define PATTERN_HPP

#include "terrain_definition.hpp"

#include "core/vector2.hpp"
#include "core/rect.hpp"

#include <exception>
#include <string>
#include <vector>
#include <cstdint>

namespace components
{
    struct PatternLoadError
        : std::runtime_error
    {
        PatternLoadError(std::string file_path);

    private:
        std::string file_path_;
    };

    class Pattern
    {
    public:
        explicit Pattern(core::Vector2u size = {});
        explicit Pattern(const std::string& file_name, core::IntRect rect = {});

        Pattern(Pattern&& pattern);
        Pattern(const Pattern& pattern) = default;

        Pattern& operator=(Pattern&& pattern);
        Pattern& operator=(const Pattern& pattern) = default;

        void load_from_file(const std::string& file_name, core::IntRect rect);

        const TerrainId& operator()(std::uint32_t x, std::uint32_t y) const;
        TerrainId& operator()(std::uint32_t x, std::uint32_t y);

        core::Vector2u size() const;
        void resize(core::Vector2u new_size);
        void resize(std::uint32_t x, std::uint32_t y);

        using iterator = TerrainId*;
        iterator begin();
        iterator end();
        iterator row_begin(std::uint32_t row_id);
        iterator row_end(std::uint32_t row_id);

        using const_iterator = const TerrainId*;
        const_iterator begin() const;
        const_iterator end() const;

        const_iterator row_begin(std::uint32_t row_id) const;
        const_iterator row_end(std::uint32_t row_id) const;

    private:
        core::Vector2u size_;
        std::vector<TerrainId> bytes_;
    };

    struct PatternSaveError
        : std::runtime_error
    {
        PatternSaveError(const std::string& file_name);
    };

    class TerrainLibrary;
    void save_pattern(const Pattern& pattern, const TerrainLibrary& terrain_library, const std::string& file_name);
}


#endif