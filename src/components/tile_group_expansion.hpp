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

                auto sub_tile_offset = core::transform_point(sub_tile.position, tile_it->rotation);

                PlacedTile placed_tile;
                placed_tile.tile_def = tile_def;
                placed_tile.tile.id = sub_tile.id;
                placed_tile.tile.level = sub_tile.level;
                placed_tile.tile.position = tile_it->position + sub_tile_offset;
                placed_tile.tile.rotation = tile_it->rotation + sub_tile.rotation;

                *out = placed_tile;
                ++out;
            }
        }
    }
}

#endif