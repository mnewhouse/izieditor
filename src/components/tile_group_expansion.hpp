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

#ifndef TILE_GROUP_EXPANSION
#define TILE_GROUP_EXPANSION

#include "tile_library.hpp"

#include "core/transform.hpp"

#include <iostream>

namespace components
{
    class TileLibrary;

    template <typename InputIt, typename OutIt>
    void expand_tile_groups(InputIt tile_it, InputIt tile_end, const TileLibrary& tile_library, OutIt out)
    {
        for (; tile_it != tile_end; ++tile_it)
        {
            const auto* tile_group = tile_library.tile_group(tile_it->id);
            if (!tile_group) continue;

            for (const auto& sub_tile : tile_group->sub_tiles())
            {
                const auto* tile_def = tile_library.tile(sub_tile.id);
                if (!tile_def) continue;

                core::Rotation<double> tile_rotation = tile_it->rotation;

                auto sub_tile_position = core::vector2_cast<double>(sub_tile.position);
                auto sub_tile_offset = core::transform_point(sub_tile_position, tile_rotation);

                PlacedTile placed_tile;
                placed_tile.tile_def = tile_def;
                placed_tile.tile.id = sub_tile.id;
                placed_tile.tile.level = sub_tile.level;
                placed_tile.tile.position = tile_it->position + core::vector2_round<std::int32_t>(sub_tile_offset);

                std::int32_t degrees = static_cast<std::int32_t>(std::round(tile_rotation.degrees())) +
                    static_cast<std::int32_t>(std::round(sub_tile.rotation.degrees()));
                placed_tile.tile.rotation = components::convert_rotation(degrees);

                *out = placed_tile;
                ++out;
            }
        }
    }
}

#endif