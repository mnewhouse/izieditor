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

#include "component_readers.hpp"
#include "tile_definition.hpp"
#include "terrain_definition.hpp"

#include "core/directive_reader.hpp"

#include <boost/algorithm/string.hpp>

#include <sstream>

namespace components
{
    std::istream& readers::operator>>(std::istream& stream, Tile& tile)
    {
        std::int32_t degrees;
        if (stream >> tile.id >> tile.position.x >> tile.position.y >> degrees)
        {
            tile.rotation = convert_rotation(degrees);
        }

        return stream;
    }

    std::istream& readers::operator>>(std::istream& stream, LevelTile& tile)
    {
        return stream >> tile.level >> static_cast<Tile&>(tile);
    }

    std::istream& readers::operator>>(std::istream& stream, TileDefinition& tile_def)
    {
        auto& image_rect = tile_def.image_rect;
        auto& pattern_rect = tile_def.pattern_rect;

        return stream >> tile_def.id >>
            pattern_rect.left >> pattern_rect.top >> pattern_rect.width >> pattern_rect.height >>
            image_rect.left >> image_rect.top >> image_rect.width >> image_rect.height;
    }


    std::istream& readers::operator>>(std::istream& stream, TerrainDefinition& terrain_def)
    {
        std::istringstream line_stream;

        for (std::string line, directive; directive != "end" && std::getline(stream, line);)
        {
            boost::trim(line);
            line_stream.clear();
            line_stream.str(line);

            core::read_directive(line_stream, directive);

            if (directive == "id")
            {
                std::uint32_t id;
                if (line_stream >> id)
                {
                    terrain_def.id = id;
                }
            }

            else if (directive == "viscosity")
            {
                line_stream >> terrain_def.viscosity;
            }

            else if (directive == "steering")
            {
                line_stream >> terrain_def.steering;
            }

            else if (directive == "grip")
            {
                line_stream >> terrain_def.grip;
            }

            else if (directive == "acceleration")
            {
                line_stream >> terrain_def.acceleration;
            }

            else if (directive == "braking")
            {
                line_stream >> terrain_def.braking;
            }

            else if (directive == "bounciness")
            {
                line_stream >> terrain_def.bounciness;
            }

            else if (directive == "slowing")
            {
                line_stream >> terrain_def.slowing;
            }

            else if (directive == "jump")
            {
                line_stream >> terrain_def.jump;
            }

            else if (directive == "maxjumpspeed")
            {
                line_stream >> terrain_def.maxjumpspeed;
            }

            else if (directive == "energyloss")
            {
                line_stream >> terrain_def.energyloss;
            }

            else if (directive == "gravity")
            {
                line_stream >> terrain_def.gravity;
            }

            else if (directive == "gravitydirection")
            {
                line_stream >> terrain_def.gravitydirection;
            }

            else if (directive == "size")
            {
                line_stream >> terrain_def.size;
            }

            else if (directive == "pit")
            {
                std::int32_t value;
                if (line_stream >> value)
                {
                    terrain_def.pit = (value != 0);
                }
            }

            else if (directive == "red")
            {
                std::uint32_t value;
                if (line_stream >> value)
                {
                    terrain_def.red = value;
                }
            }

            else if (directive == "green")
            {
                std::uint32_t value;
                if (line_stream >> value)
                {
                    terrain_def.green = value;
                }
            }

            else if (directive == "blue")
            {
                std::uint32_t value;
                if (line_stream >> value)
                {
                    terrain_def.blue = value;
                }
            }

            else if (directive == "tyremark")
            {
                std::uint32_t value;
                if (line_stream >> value)
                {
                    terrain_def.tyre_mark = (value != 0);
                }
            }

            else if (directive == "skidmark")
            {
                std::uint32_t value;
                if (line_stream >> value)
                {
                    terrain_def.skid_mark = (value != 0);
                }
            }

            else if (directive == "iswall")
            {
                std::uint32_t value;
                if (line_stream >> value)
                {
                    terrain_def.is_wall = (value != 0);
                }
            }
        }

        return stream;
    }

    std::istream& readers::operator>>(std::istream& stream, SubTerrain& sub_terrain)
    {
        std::int32_t terrain_id, component_id, level_start, level_count;
        if (stream >> terrain_id >> component_id >> level_start >> level_count)
        {
            sub_terrain.terrain_id = terrain_id;
            sub_terrain.component_id = component_id;
            sub_terrain.level_start = level_start;
            sub_terrain.level_count = level_count;
        }

        return stream;
    }
}