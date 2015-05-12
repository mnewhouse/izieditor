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

#ifndef TILE_HPP
#define TILE_HPP

#include "core/vector2.hpp"
#include "core/rotation.hpp"
#include "core/rect.hpp"

#include <cstdint>
#include <vector>
#include <string>

namespace components
{
    using core::IntRect;
    using core::Vector2;
    using core::Rotation;

    using TileId = std::uint16_t;

    struct Tile
    {
        TileId id = 0;
        core::Vector2i position;
        std::int32_t rotation;
    };

    struct LevelTile
        : public Tile
    {
        LevelTile(Tile placed_tile = Tile())
        : Tile(placed_tile)
        {}

        std::uint32_t level = 0;
    };

    struct TileDefinition
    {
    public:
        TileDefinition(std::string pattern_file, std::string image_file)
            : id(), image_rect(), pattern_rect(),
                pattern_file_(std::move(pattern_file)),
                image_file_(std::move(image_file))
        {}

        TileId id;
        IntRect image_rect;
        IntRect pattern_rect;

        const std::string& image_file() const { return image_file_; }
        const std::string& pattern_file() const { return pattern_file_; }

    private:
        std::string pattern_file_;
        std::string image_file_;
    };

    struct TileGroupDefinition
    {
    public:
        TileGroupDefinition(TileId id, std::size_t size)
            : id_(id)
        {
            sub_tiles_.reserve(size);
        }

        void add_sub_tile(const LevelTile& tile)
        {
            sub_tiles_.push_back(tile);
        }

        TileId id() const
        {
            return id_;
        }

        const std::vector<LevelTile>& sub_tiles() const
        {
            return sub_tiles_;
        }

    private:
        TileId id_;
        std::vector<LevelTile> sub_tiles_;
    };

    struct PlacedTile
    {
        const TileDefinition* tile_def = nullptr;
        LevelTile tile;
    };
}

#endif