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
    void apply_pattern(Pattern& dest, const Pattern& source, core::IntRect rect, core::Vector2i position, std::int32_t rotation);

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

            auto handle = pattern_store_.load_from_file(tile_def->pattern_file());
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
        core::IntRect rect, core::Vector2i position, std::int32_t rotation)
    {
        double radians = core::Rotation<double>::degrees(rotation).radians();

        double sin = -std::sin(radians);
        double cos = std::cos(radians);

        core::Vector2i world_size = dest.size();
        core::Vector2i pattern_size = source.size();
        core::Vector2i source_size(rect.width, rect.height);

        /*
        auto dest_size = [=]()
        {
        double x = source_size.x * 0.5;
        double y = source_size.y * 0.5;

        double cx = x * cangle;
        double cy = y * cangle;
        double sx = x * sangle;
        double sy = y * sangle;

        double w = (std::abs(cx) + std::abs(sy)) * 2.0;
        double h = (std::abs(cy) + std::abs(sx)) * 2.0;

        double width = std::ceil(w);
        double height = std::ceil(h);

        return core::Vector2i(static_cast<std::int32_t>(width), static_cast<std::int32_t>(height));
        }();

        std::int32_t isin = 65536.0 * sangle;
        std::int32_t icos = 65536.0 * cangle;

        std::int32_t cx = dest_size.x / 2;
        std::int32_t cy = dest_size.y / 2;

        std::int32_t xd = (source_size.x - dest_size.x) << 15;
        std::int32_t yd = (source_size.y - dest_size.y) << 15;

        std::int32_t ax = (cx << 16) - (icos * cx);
        std::int32_t ay = (cy << 16) - (isin * cx);

        std::int32_t base_x = position.x - cx;
        std::int32_t base_y = position.y - cy;

        if (rect.right() >= pattern_size.x) rect.width = pattern_size.x - rect.left;
        if (rect.bottom() >= pattern_size.y) rect.height = pattern_size.y - rect.top;

        for (std::int32_t y = -1; y <= dest_size.y; ++y)
        {
        std::int32_t dest_y = y + base_y;
        if (dest_y < 0 || dest_y >= world_size.y) continue;

        std::int32_t sdx = (ax + (isin * (cy - y))) + 0x8000 + xd;
        std::int32_t sdy = (ay - (icos * (cy - y))) + 0x8000 + yd;

        // cy ->
        // sdx, sdy -> too high

        for (std::int32_t x = -1; x <= dest_size.x; ++x, sdx += icos, sdy += isin)
        {
        std::int32_t dest_x = x + base_x;
        if (dest_x < 0 || dest_x >= world_size.x) continue;

        std::int32_t source_x = (sdx >> 16);
        std::int32_t source_y = (sdy >> 16);

        if (source_x >= 0 && source_y >= 0 && source_x < rect.width && source_y < rect.height)
        {
        if (auto terrain = source(source_x + rect.left, source_y + rect.top))
        {
        dest(dest_x, dest_y) = terrain;
        }
        }
        }
        }
        */

        core::Vector2<double> dest_size;
        {
            double x = source_size.x * 0.5;
            double y = source_size.y * 0.5;

            double cx = x * cos;
            double cy = y * cos;
            double sx = x * sin;
            double sy = y * sin;

            double half_width = std::abs(cx) + std::abs(sy);
            double half_height = std::abs(cy) + std::abs(sx);

            dest_size = { std::ceil(half_width * 2.0), std::ceil(half_height * 2.0) };
        }

        core::Vector2<double> source_center(source_size.x * 0.5, source_size.y * 0.5);
        core::Vector2<double> real_position = position;

        if (rect.right() > pattern_size.x) rect.width = pattern_size.x - rect.left;
        if (rect.bottom() > pattern_size.y) rect.height = pattern_size.y - rect.top;

        std::int32_t start_x = (source_size.x - dest_size.x) / 2 - 1;
        std::int32_t start_y = (source_size.y - dest_size.y) / 2 - 1;

        std::int32_t end_x = start_x + dest_size.x + 2;
        std::int32_t end_y = start_y + dest_size.y + 2;

        std::int32_t offset_x = position.x - source_size.x / 2;
        std::int32_t offset_y = position.y - source_size.y / 2;

        for (std::int32_t y = start_y; y < end_y; ++y)
        {
            for (std::int32_t x = start_x; x < end_x; ++x)
            {
                core::Vector2<double> point(x, y);
                point -= source_center;
                
                std::int32_t absolute_x = x + offset_x;
                std::int32_t absolute_y = y + offset_y;

                if (absolute_x >= 0 && absolute_y >= 0 && absolute_x < world_size.x && absolute_y < world_size.y)
                {
                    core::Vector2<double> source_point = core::transform_point(point, sin, cos);
                    source_point += source_center;

                    auto source_x = static_cast<std::int32_t>(std::round(source_point.x));
                    auto source_y = static_cast<std::int32_t>(std::round(source_point.y));

                    if (source_x >= 0 && source_y >= 0 && source_x < rect.width && source_y < rect.height)
                    {
                        if (auto terrain = source(source_x + rect.left, source_y + rect.top))
                        {
                            dest(absolute_x, absolute_y) = terrain;
                        }
                    }
                }
            }
        }
    }
}