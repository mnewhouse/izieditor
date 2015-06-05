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

#include "core/clamp.hpp"
#include "core/md5.hpp"

#include <cstdint>
#include <algorithm>

namespace components
{
    static const std::uint32_t max_sub_terrains = 16;

    TerrainLibrary::TerrainLibrary()
        : terrains_(256),
          sub_terrains_(terrains_.size() * max_sub_terrains)
    {
        for (std::uint32_t id = 0; id != 256; ++id)
        {
            terrains_[id].id = id;
            terrains_[id].hash = calculate_terrain_hash(id);
        }
    }

    void TerrainLibrary::define_terrain(TerrainDefinition terrain)
    {
        auto id = terrain.id;
        
        using core::clamp;
        terrain.bounciness = clamp(terrain.bounciness, 0.0, 5.0);
        terrain.viscosity = clamp(terrain.viscosity, 0.0, 100.0);
        terrain.acceleration = clamp(terrain.acceleration, 0.01, 100.0);
        terrain.braking = clamp(terrain.braking, 0.0, 100.0);
        terrain.grip = clamp(terrain.grip, 0.0, 100.0);
        terrain.steering = clamp(terrain.steering, 0.0, 100.0);
        terrain.slowing = clamp(terrain.slowing, 0.0, 100.0);
        terrain.jump = clamp(terrain.jump, 0.0, 10.0);
        terrain.maxjumpspeed = clamp(terrain.maxjumpspeed, 0.0, 250.0);

        auto convert_double = [](double value)
        {
            return std::round(value * 1000.0) / 1000.0;
        };

        terrain.bounciness = convert_double(terrain.bounciness);
        terrain.viscosity = convert_double(terrain.viscosity);
        terrain.acceleration = convert_double(terrain.acceleration);
        terrain.braking = convert_double(terrain.braking);
        terrain.grip = convert_double(terrain.grip);
        terrain.steering = convert_double(terrain.steering);
        terrain.slowing = convert_double(terrain.slowing);
        terrain.jump = convert_double(terrain.jump);
        terrain.maxjumpspeed = convert_double(terrain.maxjumpspeed);

        terrain.energyloss = clamp(terrain.energyloss, 0, 100000);
        terrain.gravity = clamp(terrain.gravity, 0, 10000);
        terrain.gravitydirection = clamp(terrain.gravitydirection, 0, 359);        

        if (terrain.size == 1)
        {
            terrain.size += terrain.is_wall ? 1 : 0;
        }

        static_cast<TerrainDefinition&>(terrains_[id]) = terrain;

        {
            auto sub_terrains = &sub_terrains_[id * 16];

            SubTerrainDefinition sub_terrain;
            sub_terrain.slowing = terrain.slowing;
            sub_terrain.gravity = terrain.gravity;
            sub_terrain.gravitydirection = terrain.gravitydirection;
            sub_terrain.energyloss = terrain.energyloss;
            sub_terrain.pit = terrain.pit;
            sub_terrain.energyloss = terrain.energyloss;
            sub_terrain.jump = terrain.jump;
            sub_terrain.maxjumpspeed = terrain.maxjumpspeed;            
            sub_terrain.level = 0.0;
            sub_terrain.roof_level = 127.0;
            std::fill(sub_terrains, sub_terrains + 16, sub_terrain);
        }       

        SubTerrain sub_terrain;
        sub_terrain.terrain_id = id;
        sub_terrain.component_id = id;        
        sub_terrain.level_start = 0;
        sub_terrain.level_count = clamp(terrain.size, 1, 16);
        define_sub_terrain(sub_terrain);
    }

    void TerrainLibrary::define_sub_terrain(const SubTerrain& sub_terrain)
    {
        std::uint32_t index = sub_terrain.level_start;
        std::uint32_t index_end = std::min(index + sub_terrain.level_count, 16U);

        if (index < max_sub_terrains && index != index_end)
        {
            SubTerrainDefinition* sub_terrains = &sub_terrains_[sub_terrain.terrain_id * 16];
            SubTerrainDefinition* range_begin = sub_terrains + index;
            SubTerrainDefinition* range_end = sub_terrains + index_end;
            SubTerrainDefinition* end = sub_terrains + 16;            

            double new_level = static_cast<double>(index_end * 8 - 8);
            double new_roof = static_cast<double>(index * 8 - 1);

            double prior_level = range_begin->level;
            double prior_roof = range_begin->roof_level;

            const auto& terrain = terrain_by_id(sub_terrain.component_id);
            for (auto ptr = range_begin; ptr != range_end; ++ptr)
            {
                static_cast<TerrainDefinition&>(*ptr) = terrain;
            }

            if (index != 0)
            {
                for (auto ptr = sub_terrains; ptr != range_begin; ++ptr)
                {
                    if (ptr->roof_level == prior_roof)
                    {
                        ptr->roof_level = new_roof;
                    }
                }
            }

            for (auto ptr = range_begin; ptr != end; ++ptr)
            {
                if (ptr->level == prior_level)
                {
                    ptr->level = new_level;
                }
            }

            if (terrain.is_wall && range_end != end)
            {
                new_level = static_cast<double>(index_end * 8.0);
                prior_level = range_end->level;
                for (auto ptr = range_end; ptr != end; ++ptr)
                {
                    if (ptr->level == prior_level)
                    {
                        ptr->level = new_level;
                    }
                }
            }
        }

        TerrainId id = sub_terrain.terrain_id;
        terrains_[id].hash = calculate_terrain_hash(id);
    }

    const TerrainDefinition& TerrainLibrary::sub_terrain(TerrainId terrain, std::uint32_t index) const
    {
        return sub_terrains_[terrain * max_sub_terrains + index];
    }

    const TerrainDefinition& TerrainLibrary::terrain_by_id(TerrainId id) const
    {
        return terrains_[id];
    }

    bool TerrainLibrary::has_custom_sub_terrains(TerrainId terrain_id) const
    {
        const auto& terrain = terrains_[terrain_id];
        const SubTerrainDefinition* sub_terrains = &sub_terrains_[terrain_id * 16U];

        return !std::all_of(sub_terrains, sub_terrains + 16, 
            [](const SubTerrainDefinition& sub_terrain)
        {
            return sub_terrain.level == 0.0 && sub_terrain.roof_level == 127.0;
        });
    }

    void TerrainLibrary::define_kill_terrain(TerrainId terrain_id)
    {
        terrains_[terrain_id].energyloss = 100000;

        auto* sub_terrain = &sub_terrains_[terrain_id * 16U];
        auto* sub_terrain_end = sub_terrain + 16U;
        
        for (; sub_terrain != sub_terrain_end; ++sub_terrain)
        {
            sub_terrain->energyloss = 100000;
        }
    }

    const TerrainHash& TerrainLibrary::terrain_hash(TerrainId terrain_id) const
    {
        return terrains_[terrain_id].hash;
    }

    TerrainHash TerrainLibrary::calculate_terrain_hash(TerrainId terrain_id) const
    {
        auto f2i = [](double value)
        {
            return static_cast<std::int32_t>(std::round(value * 1000.0));
        };        
        
        const SubTerrainDefinition* sub_terrains = &sub_terrains_[terrain_id * 16U];

        bool has_sub_terrains = has_custom_sub_terrains(terrain_id);
        std::uint32_t sub_terrain_end = (has_sub_terrains ? 16 : 1);

        MD5 md5;
        for (std::uint32_t sub_terrain = 0; sub_terrain != sub_terrain_end; ++sub_terrain)
        {
            const auto& terrain = sub_terrains[sub_terrain];

            md5 << static_cast<std::uint8_t>(terrain.is_wall);
            if (terrain.is_wall)
            {
                md5 << f2i(terrain.bounciness);
            }

            md5 << f2i(terrain.viscosity);
            md5 << f2i(terrain.acceleration);
            md5 << f2i(terrain.braking);
            md5 << f2i(terrain.grip);
            md5 << f2i(terrain.steering);

            if (terrain.slowing != 0.0 || terrain.jump != 0.0 || terrain.maxjumpspeed != 140.0)
            {
                md5 << f2i(terrain.slowing);
                md5 << f2i(terrain.jump);
                md5 << f2i(terrain.maxjumpspeed);
            }

            if (terrain.pit)
            {
                md5 << static_cast<std::int32_t>(terrain.pit);
            }

            if (terrain.energyloss != 0)
            {
                md5 << terrain.energyloss;
            }

            if (terrain.gravity != 0)
            {
                md5 << terrain.gravity << terrain.gravitydirection;
            }
        }

        if (has_sub_terrains)
        {
            const SubTerrainDefinition* levels = &sub_terrains_[terrain_id * 16U];

            for (std::uint32_t index = 0; index != 16; ++index)
            {
                md5 << f2i(levels[index].level) << f2i(levels[index].roof_level);
            }
        }


        md5.finalize();
        return md5.digest();
    }
}