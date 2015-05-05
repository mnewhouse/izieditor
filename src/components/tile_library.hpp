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

#ifndef TILE_LIBRARY_HPP
#define TILE_LIBRARY_HPP

#include "tile_definition.hpp"

#include <map>

namespace components
{
    // The tile library keeps track of all the tiles and tile groups that have been defined during
    // the track loading process, allowing for relatively efficient retrieval later on.
    class TileLibrary
    {
    public:
        void define_tile(const TileDefinition& tile_def);
        void define_tile_group(const TileGroupDefinition& tile_group_def);

        const TileDefinition* tile(TileId id) const;
        const TileGroupDefinition* tile_group(TileId id) const;

        const TileGroupDefinition* first_tile_group() const;
        const TileGroupDefinition* last_tile_group() const;

        const TileGroupDefinition* next_tile_group(TileId current) const;
        const TileGroupDefinition* previous_tile_group(TileId current) const;

        const TileDefinition* first_tile() const;
        const TileDefinition* next_tile(const TileDefinition* current) const;

    private:
        std::map<TileId, TileDefinition> tile_map_;
        std::map<TileId, TileGroupDefinition> tile_group_map_;
    };
}

#endif