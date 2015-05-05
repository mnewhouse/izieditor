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
#include "terrain_library.hpp"

#include <cstdint>

namespace components
{

    TerrainLibrary::TerrainLibrary()
        : sub_terrains_(256 * 256)
    {
    }

    void TerrainLibrary::define_terrain(const TerrainDefinition& terrain_definition)
    {
        auto id = terrain_definition.id;
        terrains_[id] = terrain_definition;

        define_sub_terrain(id, id, 0);
    }

    void TerrainLibrary::define_sub_terrain(const SubTerrain& sub_terrain)
    {
        for (std::uint32_t level = sub_terrain.level_start, level_end = level + sub_terrain.level_count; level != level_end; ++level) {
            define_sub_terrain(sub_terrain.terrain_id, sub_terrain.component_id, level);
        }
    }

    void TerrainLibrary::define_sub_terrain(TerrainId terrain, TerrainId sub_id, std::size_t level)
    {
        std::size_t index = (terrain * 256) + level;
        sub_terrains_[index] = sub_id;
    }

    TerrainId TerrainLibrary::sub_terrain(TerrainId terrain, std::size_t level) const
    {
        auto index = terrain * 256 + level;
        return sub_terrains_[index];
    }


    const TerrainDefinition& TerrainLibrary::terrain_by_id(TerrainId id) const
    {
        return terrains_[id];
    }
}