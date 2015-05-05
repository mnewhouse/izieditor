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

#ifndef TERRAIN_LIBRARY_HPP
#define TERRAIN_LIBRARY_HPP

#include "terrain_definition.hpp"

#include <vector>

namespace components
{
    // The terrain library stores all the terrains and sub-terrains, and provides functions
    // to obtain information about said terrains and sub-terrains.

    class TerrainLibrary
    {
    public:
        TerrainLibrary();

        void define_terrain(const TerrainDefinition& TerrainDefinition);
        void define_sub_terrain(const SubTerrain& sub_terrain);

        const TerrainDefinition& terrain_by_id(TerrainId id) const;
        TerrainId sub_terrain(TerrainId id, std::size_t level) const;


    private:
        void define_sub_terrain(TerrainId terrain, TerrainId sub_id, std::size_t level);

        TerrainDefinition terrains_[256];
        std::vector<TerrainId> sub_terrains_;
    };
}

#endif