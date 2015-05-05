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

#ifndef COMPONENT_READERS_HPP
#define COMPONENT_READERS_HPP

#include <istream>

namespace components
{
    struct Tile;
    struct TileDefinition;
    struct LevelTile;

    struct TerrainDefinition;
    struct SubTerrain;

    namespace readers
    {
        std::istream& operator>>(std::istream& stream, Tile& tile);
        std::istream& operator>>(std::istream& stream, LevelTile& tile);
        std::istream& operator>>(std::istream& stream, TileDefinition& tile);

        std::istream& operator>>(std::istream& stream, TerrainDefinition& terrain);
        std::istream& operator>>(std::istream& stream, SubTerrain& sub_terrain);
    }
}

#endif