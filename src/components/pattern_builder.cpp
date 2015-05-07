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
    void apply_pattern(Pattern& dest, const Pattern& source, core::IntRect rect, core::Vector2<double> position, Rotation<double> rotation);

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
        core::IntRect rect, core::Vector2<double> position, Rotation<double> rotation)
    {
        double sangle = -std::sin(rotation.radians());
        double cangle = std::cos(rotation.radians());

        core::Vector2i world_size = dest.size();
        core::Vector2i pattern_size = source.size();
        core::Vector2i source_size(rect.width, rect.height);

        auto dest_size = [=]()
        {
            auto x = double(source_size.x >> 1);
            auto y = double(source_size.y >> 1);

            auto cx = x * cangle;
            auto cy = y * cangle;
            auto sx = x * sangle;
            auto sy = y * sangle;

            auto width = std::ceil(std::abs(cx) + std::abs(sy));
            auto height = std::ceil(std::abs(cy) + std::abs(sx));

            return core::Vector2i(std::int32_t(width) << 1, std::int32_t(height) << 1);
        }();

        auto isin = std::int32_t(65536.0 * sangle);
        auto icos = std::int32_t(65536.0 * cangle);

        std::int32_t cx = dest_size.x >> 1;
        std::int32_t cy = dest_size.y >> 1;

        core::Vector2i int_pos(static_cast<std::int32_t>(position.x + 0.5), static_cast<std::int32_t>(position.y + 0.5));

        std::int32_t xd = (source_size.x - dest_size.x) << 15;
        std::int32_t yd = (source_size.y - dest_size.y) << 15;
        std::int32_t ax = (cx << 16) - (icos * cx);
        std::int32_t ay = (cy << 16) - (isin * cx);

        std::int32_t base_x = int_pos.x - cx;
        std::int32_t base_y = int_pos.y - cy;

        std::int32_t start_x = std::min(std::max(base_x, 0), world_size.x);
        std::int32_t start_y = std::min(std::max(base_y, 0), world_size.y);

        std::int32_t end_x = std::min(std::max(base_x + dest_size.x + 1, 0), world_size.x);
        std::int32_t end_y = std::min(std::max(base_y + dest_size.y + 1, 0), world_size.y);

        if (rect.right() >= pattern_size.x) rect.width = pattern_size.x - rect.left;
        if (rect.bottom() >= pattern_size.y) rect.height = pattern_size.y - rect.top;

        for (std::int32_t dest_y = start_y, y = dest_y - base_y; dest_y != end_y; ++y, ++dest_y)
        {
            std::int32_t sdx = (ax + (isin * (cy - y))) + xd + 0x8000;
            std::int32_t sdy = (ay - (icos * (cy - y))) + yd + 0x8000;

            // Get the range in which (0 <= source_x < rect.width AND 0 <= source_y < rect.height)

            auto determine_range = [](std::int32_t base_value, std::int32_t multiplier, std::int32_t min, std::int32_t max)
                -> std::pair < std::int32_t, std::int32_t >
            {
                if (multiplier == 0)
                {
                    auto value = base_value >> 16;
                    if (value < min || value >= max) return std::make_pair(0, 0); // Empty range

                    // Infinite range
                    return std::make_pair(std::numeric_limits<std::int32_t>::min(),
                        std::numeric_limits<std::int32_t>::max());
                }

                auto equal_range = [multiplier, base_value](std::int32_t value)
                {
                    auto target_values = std::make_pair((value << 16) - base_value,
                        ((value << 16) + 0xFFFF) - base_value);

                    auto div = std::make_pair(std::div(target_values.first, multiplier),
                        std::div(target_values.second, multiplier));

                    if (div.first.rem != 0 && target_values.first > 0)
                    {
                        div.first.quot += (multiplier < 0 ? -1 : 1);
                    }

                    if (div.second.rem != 0 && target_values.second < 0)
                    {
                        div.second.quot += (multiplier < 0 ? 1 : -1);
                    }

                    if (div.first.quot > div.second.quot)
                    {
                        std::swap(div.first.quot, div.second.quot);
                    }

                    return std::make_pair(div.first.quot, div.second.quot);
                };

                auto min_range = equal_range(min);
                auto max_range = equal_range(max);

                auto begin = min_range.first;
                auto end = max_range.first;

                if (begin > end)
                {
                    begin = max_range.second + 1;
                    end = min_range.second + 1;
                }

                return std::make_pair(begin, end);
            };

            auto x_range = determine_range(sdx, icos, 0, rect.width);
            auto y_range = determine_range(sdy, isin, 0, rect.height);

            // Get the overlap of the ranges
            auto overlap_range = std::make_pair(std::max(x_range.first, y_range.first),
                std::min(x_range.second, y_range.second));

            auto range = std::make_pair(std::max(overlap_range.first + base_x, start_x),
                std::min(overlap_range.second + base_x, end_x));

            range.second = std::max(range.first, range.second);

            auto row_begin = dest.row_begin(dest_y);
            auto dest_start = row_begin + range.first;
            auto dest_end = row_begin + range.second;

            auto skipped = range.first - base_x;
            sdx += skipped * icos;
            sdy += skipped * isin;

            // Loop invariant: 0 <= source_x < rect.width && 0 <= source_y < rect.height
            for (auto dest_ptr = dest_start; dest_ptr != dest_end; ++dest_ptr, sdx += icos, sdy += isin)
            {
                std::int32_t source_x = sdx >> 16;
                std::int32_t source_y = sdy >> 16;

                if (auto terrain = source(source_x + rect.left, source_y + rect.top))
                {
                    *dest_ptr = terrain;
                }
            }
        }
    }
}