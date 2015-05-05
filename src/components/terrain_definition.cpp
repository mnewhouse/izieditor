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
#include "terrain_definition.hpp"

#include <sstream>
#include <cstdint>
#include <string>

/*
namespace components
{
    std::istream& operator>>(std::istream& stream, TerrainDefinition& terrain_def)
    {
        std::istringstream line_stream;

        for (std::string line, directive; directive != "end" && std::getline(stream, line);)
        {
            boost::trim(line);
            line_stream.clear();
            line_stream.str(line);

            read_directive(line_stream, directive);

            if (directive == "id")
            {
                std::uint32_t id;
                if (line_stream >> id)
                {
                    terrain_def.id = id;
                }
            }

            else if (directive == "steering")
            {
                line_stream >> terrain_def.steering;
            }

            else if (directive == "antislide")
            {
                line_stream >> terrain_def.antislide;
            }

            else if (directive == "acceleration")
            {
                line_stream >> terrain_def.acceleration;
            }

            else if (directive == "drag")
            {
                line_stream >> terrain_def.drag;
            }

            else if (directive == "traction")
            {
                line_stream >> terrain_def.traction;
            }

            else if (directive == "braking")
            {
                line_stream >> terrain_def.braking;
            }

            else if (directive == "bounciness")
            {
                line_stream >> terrain_def.wall_definition.elasticity;
            }

            else if (directive == "red")
            {
                std::uint32_t value;
                if (line_stream >> value)
                {
                    terrain_def.color.red = value;
                }
            }

            else if (directive == "green")
            {
                std::uint32_t value;
                if (line_stream >> value)
                {
                    terrain_def.color.green = value;
                }
            }

            else if (directive == "blue")
            {
                std::uint32_t value;
                if (line_stream >> value)
                {
                    terrain_def.color.blue = value;
                }
            }

            else if (directive == "tiremark")
            {
                std::uint32_t value;
                if (line_stream >> value)
                {
                    terrain_def.tire_mark = (value != 0);
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

    std::istream& operator>>(std::istream& stream, SubTerrain& sub_terrain)
    {
        std::int32_t TerrainId, component_id, level_start, level_count;
        if (stream >> TerrainId >> component_id >> level_start >> level_count)
        {
            sub_terrain.TerrainId = TerrainId;
            sub_terrain.component_id = component_id;
            sub_terrain.level_start = level_start;
            sub_terrain.level_count = level_count;
        }

        return stream;
    }

    bool operator==(const TerrainDefinition& first, const TerrainDefinition& second)
    {
        return first.id == second.id;
    }

    bool operator!=(const TerrainDefinition& first, const TerrainDefinition& second)
    {
        return first.id != second.id;
    }
}

*/