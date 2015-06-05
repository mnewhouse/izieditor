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

#include "tile_library.hpp"

namespace components
{
    void TileLibrary::define_tile(const TileDefinition& tile_def)
    {
        auto it = tile_map_.find(tile_def.id);
        if (it == tile_map_.end())
        {
            tile_map_.insert(std::make_pair(tile_def.id, tile_def));
        }
        else
        {
            it->second = tile_def;
        }

        TileGroupDefinition tile_group_def(tile_def.id, 1, tile_def.rotatable);

        LevelTile sub_tile{};
        sub_tile.id = tile_def.id;

        tile_group_def.add_sub_tile(sub_tile);
        define_tile_group(tile_group_def);
    }

    void TileLibrary::define_tile_group(const TileGroupDefinition& tile_group_def)
    {
        auto id = tile_group_def.id();
        auto it = tile_group_map_.find(id);
        if (it == tile_group_map_.end()) 
        {
            tile_group_map_.insert(std::make_pair(id, tile_group_def));
        }

        else 
        {
            it->second = tile_group_def;
        }
    }


    const TileDefinition* TileLibrary::tile(TileId id) const
    {
        auto it = tile_map_.find(id);
        if (it == tile_map_.end()) return nullptr;

        return &it->second;
    }

    const TileGroupDefinition* TileLibrary::tile_group(TileId id) const
    {
        auto it = tile_group_map_.find(id);
        if (it == tile_group_map_.end()) return nullptr;

        return &it->second;
    }

    const TileGroupDefinition* TileLibrary::first_tile_group() const
    {
        if (tile_group_map_.empty()) return nullptr;

        return &tile_group_map_.begin()->second;
    }

    const TileGroupDefinition* TileLibrary::last_tile_group() const
    {
        if (tile_group_map_.empty()) return nullptr;

        return &std::prev(tile_group_map_.end())->second;
    }

    const TileGroupDefinition* TileLibrary::next_tile_group(TileId current) const
    {
        auto it = tile_group_map_.upper_bound(current);
        if (it == tile_group_map_.end())
        {
            return nullptr;
        }

        return &it->second;
    }

    const TileGroupDefinition* TileLibrary::previous_tile_group(TileId current) const
    {
        auto it = tile_group_map_.lower_bound(current);
        if (it == tile_group_map_.end())
        {
            return last_tile_group();
        }

        if (it == tile_group_map_.begin())
        {
            return nullptr;
        }

        return &std::prev(it)->second;
    }

    const TileDefinition* TileLibrary::first_tile() const
    {
        if (tile_map_.empty()) return nullptr;

        return &tile_map_.begin()->second;
    }

    const TileDefinition* TileLibrary::next_tile(TileId current) const
    {
        if (tile_map_.empty()) return nullptr;

        auto it = tile_map_.upper_bound(current);
        if (it == tile_map_.end())
        {
            return nullptr;
        }

        return &it->second;
    }
}