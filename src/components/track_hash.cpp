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

#include "track_hash.hpp"
#include "track.hpp"
#include "terrain_library.hpp"
#include "pattern.hpp"
#include "control_point.hpp"
#include "start_point.hpp"

#include "core/md5.hpp"

namespace components
{
    TrackHash calculate_track_hash(const Track& track, const Pattern& pattern)
    {
        MD5 md5;

        const auto& terrain_library = track.terrain_library();       

        const auto& control_points = track.control_points();
        for (ControlPoint point : control_points)
        {
            std::int32_t x = point.start.x;
            std::int32_t y = point.start.y;
            std::int32_t length = point.length;
            std::uint8_t direction = (point.direction == ControlPoint::Vertical ? 0 : 1);

            md5 << x << y << length << direction;
        }

        const auto& start_points = track.start_points();
        for (StartPoint point : start_points)
        {
            std::int32_t x = point.position.x;
            std::int32_t y = point.position.y;
            std::int32_t rotation = point.rotation;
            std::uint8_t level = point.level;

            md5 << x << y << rotation << level;
        }

        core::Vector2i track_size = track.size();
        md5 << track_size.y << track_size.x;

        std::int32_t num_levels = track.num_levels();
        if (num_levels != 1)
        {
            md5 << num_levels;
        }

        if (track.is_start_direction_overridden())
        {
            std::int32_t start_direction = track.start_direction();
            md5 << start_direction;
        }

        if (track_size.y != 0)
        {
            std::uint32_t hash_index = 0;

            for (std::int32_t y = 0; y != track_size.y; ++y)
            {                
                for (std::int32_t x = 0; x != track_size.x; ++x)
                {                   
                    const auto& hash = terrain_library.terrain_hash(pattern(x, y));
                    md5 << hash[hash_index];

                    if (++hash_index >= 4)
                    {
                        hash_index = 0;
                    }
                }
            }
        }

        else
        {
            md5 << std::uint32_t(0x70) << std::uint32_t(0x6F);
        }


        md5.finalize();
        return md5.digest();
    }
}
