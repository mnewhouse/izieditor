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

#include "pattern_builder.hpp"
#include "pattern.hpp"
#include "track.hpp"
#include "tile_definition.hpp"

#include "tile_group_expansion.hpp"

#include "core/rect.hpp"
#include "core/vector2.hpp"
#include "core/rotation.hpp"

#include <algorithm>
#include <cmath>

namespace components
{
    void apply_pattern(Pattern& dest, const Pattern& source, core::IntRect rect, core::Vector2i position, 
        core::Rotation<double> rotation);

    PatternBuilder::PatternBuilder(const Track& track, PatternStore pattern_store)
        : track_(track),
        pattern_store_(pattern_store)
    {
    }

    Pattern PatternBuilder::operator()(std::function<void()> step_operation)
    {
        Pattern pattern(track_.size());

        std::vector<PlacedTile> tile_expansion;

        const auto& layers = track_.layers();
        for (const auto& layer : layers)
        {
            expand_tile_groups(layer->tiles.begin(), layer->tiles.end(), track_.tile_library(),
                std::back_inserter(tile_expansion));
        }

        for (const auto& placed_tile : tile_expansion)
        {
            const auto* tile_def = placed_tile.tile_def;
            const auto& tile = placed_tile.tile;

            auto handle = pattern_store_.load_from_file(tile_def->pattern_file);
            apply_pattern(pattern, *handle, tile_def->pattern_rect, tile.position, tile.rotation);

            if (step_operation) step_operation();
        }

        return pattern;
    }

    void PatternBuilder::preload_pattern(const std::string& path)
    {
        pattern_store_.load_from_file(path);
    }

    void apply_pattern(Pattern& dest, const Pattern& source,
        core::IntRect rect, core::Vector2i position, core::Rotation<double> rotation)
    {
        double radians = rotation.radians();

        double sin = -std::sin(radians);
        double cos = std::cos(radians);        

        core::Vector2i world_size(dest.size().x, dest.size().y);
        core::Vector2i pattern_size(source.size().x, dest.size().y);
        core::Vector2i source_size(rect.width, rect.height);

        core::Vector2i dest_size;
        {
            double x = source_size.x * 0.5f;
            double y = source_size.y * 0.5f;

            double cx = x * cos;
            double cy = y * cos;
            double sx = x * sin;
            double sy = y * sin;

            double half_width = std::abs(cx) + std::abs(sy);
            double half_height = std::abs(cy) + std::abs(sx);

            dest_size.x = static_cast<std::int32_t>(std::ceil(half_width * 2.0));
            dest_size.y = static_cast<std::int32_t>(std::ceil(half_height * 2.0));
        }

        auto source_center = core::vector2_cast<double>(source_size) * 0.5;

        if (rect.right() > pattern_size.x) rect.width = pattern_size.x - rect.left;
        if (rect.bottom() > pattern_size.y) rect.height = pattern_size.y - rect.top;

        std::int32_t start_x = (source_size.x - dest_size.x) / 2 - 1;
        std::int32_t start_y = (source_size.y - dest_size.y) / 2 - 1;

        std::int32_t end_x = start_x + dest_size.x + 2;
        std::int32_t end_y = start_y + dest_size.y + 2;

        std::int32_t offset_x = position.x - source_size.x / 2;
        std::int32_t offset_y = position.y - source_size.y / 2;

        core::Vector2<double> dest_point;
        for (std::int32_t y = start_y; y <= end_y; ++y)
        {
            dest_point.y = static_cast<double>(y) - source_center.y;

            for (std::int32_t x = start_x; x <= end_x; ++x)
            {
                dest_point.x = static_cast<double>(x) - source_center.x;
                
                std::int32_t absolute_x = x + offset_x;
                std::int32_t absolute_y = y + offset_y;

                if (absolute_x >= 0 && absolute_y >= 0 && absolute_x < world_size.x && absolute_y < world_size.y)
                {
                    auto source_point = core::transform_point<double>(dest_point, sin, cos) + source_center;
                    auto point = core::vector2_round<std::int32_t>(core::vector2_cast<float>(source_point));

                    if (point.x >= 0 && point.y >= 0 && point.x < rect.width && point.y < rect.height)
                    {
                        if (auto terrain = source(point.x + rect.left, point.y + rect.top))
                        {
                            dest(absolute_x, absolute_y) = terrain;
                        }
                    }
                }
            }
        }
    }
}