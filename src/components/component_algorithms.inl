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

#ifndef COMPONENT_ALGORITHMS_INL
#define COMPONENT_ALGORITHMS_INL

#include "tile_definition.hpp"
#include "tile_library.hpp"
#include "component_algorithms.hpp"

#include "core/transform.hpp"

#include <random>
#include <iostream>

template <typename RNG, typename OutIt>
void components::fill_area(const TileGroupDefinition& tile_group, const TileLibrary& tile_library,
    const FillProperties& properties, RNG&& rng, OutIt out)
{
    auto box = tile_group_bounding_box(tile_group, tile_library);

    std::uniform_real_distribution<double> offset_dist(-properties.position_jitter, properties.position_jitter);
    std::uniform_int_distribution<std::int32_t> rotation_dist(0, 359);

    double density_multiplier = 1.0 / std::max(properties.density, 0.1);
    const auto& area = properties.area;

    double y_increment = std::max(box.height * density_multiplier - 1.0, 1.0);
    double x_increment = std::max(box.width * density_multiplier - 1.0, 1.0);

    double sin = 0.0;
    double cos = 1.0;

    core::DoubleRect transformed_area(area.left, area.top, area.width, area.height);  
    if (!properties.randomize_rotation)
    {
        auto rotation = core::Rotation<double>::degrees(properties.rotation);
        sin = std::sin(rotation.radians());
        cos = std::cos(rotation.radians());

        transformed_area = core::transform_rect(transformed_area, -sin, cos);
    }

    core::Vector2<double> center(transformed_area.left + transformed_area.width * 0.5,
        transformed_area.top + transformed_area.height * 0.5);

    for (double y = transformed_area.top; y < transformed_area.right(); y += y_increment)
    {
        for (double x = transformed_area.left; x < transformed_area.right(); x += x_increment)
        {
            auto position = core::transform_point<double>({ x - center.x, y - center.y }, sin, cos);

            Tile tile;
            tile.id = tile_group.id();

            core::Vector2<double> offset(offset_dist(rng) * box.width, offset_dist(rng) * box.height);
            tile.position = core::vector2_round<std::int32_t>(position + center + offset);

            tile.rotation = core::Rotation<double>::degrees(static_cast<double>(properties.rotation));
            if (properties.randomize_rotation)
            {
                tile.rotation = core::Rotation<double>::degrees(static_cast<double>(rotation_dist(rng)));
            }

            if (contains(area, tile.position))
            {
                *out = tile; ++out;
            }
        }       
    }
}

template <typename OutIt>
void components::generate_default_start_points(const ControlPoint& finish_line, std::int32_t direction,
                                               std::size_t num_points, OutIt out)
{
    auto direction_rotation = core::Rotation<double>::degrees(direction);

    double sin = std::sin(direction_rotation.radians());
    double cos = std::cos(direction_rotation.radians());

    auto center = core::vector2_cast<double>(finish_line.start);
    if (finish_line.direction == ControlPoint::Horizontal) center.x += finish_line.length * 0.5;
    else center.y += finish_line.length * 0.5;

    const double grid_spacing = 12.0;

    core::Vector2<double> grid_direction = core::transform_point<double>({ -1.0, 0.0 }, sin, cos);
    core::Vector2<double> lateral_offset(grid_direction.y, -grid_direction.x);
    lateral_offset *= grid_spacing;

    core::Vector2<double> left_column_start = center - lateral_offset + grid_direction * (3.0 + grid_spacing);
    core::Vector2<double> right_column_start = center + lateral_offset + grid_direction * (3.0 + grid_spacing * 2.0);

    StartPoint start_point;
    start_point.rotation = direction;

    core::Vector2<double> offset{};

    for (std::size_t i = 0; i != num_points; ++i)
    {
        if ((i & 1) == 0)
        {
            start_point.position = left_column_start + offset;
        }

        else
        {
            start_point.position = right_column_start + offset;
            offset += grid_direction * grid_spacing * 2.0;
        }

        *out = start_point; ++out;        
    }
}



#endif