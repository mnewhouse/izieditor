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

#ifndef TERRAIN_DEFINITION_HPP
#define TERRAIN_DEFINITION_HPP

#include <cstdint>
#include <istream>

namespace components
{
    using TerrainId = std::uint8_t;

    struct TerrainDefinition
    {
        std::string name = "Unnamed";
        TerrainId id = 0;        

        double acceleration = 1.0;
        double steering = 1.0;
        double grip = 1.0;
        double viscosity = 1.0;
        double braking = 1.0;        
        double bounciness = 1.0;
        double slowing = 0.0;
        double jump = 0.0;
        double maxjumpspeed = 140.0;

        std::int32_t energyloss = 0;
        std::int32_t gravity = 0;
        std::int32_t gravitydirection = 90;
        std::int32_t size = 1;
        std::int32_t red = 150;
        std::int32_t green = 150;
        std::int32_t blue = 150;

        bool tyre_mark = false;
        bool skid_mark = false;
        bool is_wall = false;
        bool pit = false;
    };

    struct SubTerrain
    {
        TerrainId terrain_id = 0;
        TerrainId component_id = 0;
        std::int32_t level_start = 0;
        std::int32_t level_count = 0;
    };
}

#endif