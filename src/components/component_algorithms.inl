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
    
    /*
    // Get the bounding box of the tile group
    core::DoubleRect box = [&]()
    {
        auto bb = tile_group_bounding_box(tile_group, tile_library);

        if (properties.randomize_rotation)
        {
            double max_value = std::max({
                std::abs(bb.left), std::abs(bb.top),
                std::abs(bb.right()), std::abs(bb.bottom())
            });

            double size = max_value * 2.0;
            return core::DoubleRect(-max_value, -max_value, size, size);
        }

        return core::transform_rect(core::DoubleRect(bb.left, bb.top, bb.width, bb.height), properties.rotation);
    }();
    */

    std::uniform_real_distribution<double> offset_dist(-properties.position_jitter, properties.position_jitter);
    std::uniform_real_distribution<double> rotation_dist(0.0, 359.0);

    double density_multiplier = 1.0 / std::max(properties.density, 0.1);
    const auto& area = properties.area;

    double y_increment = std::max(box.height * density_multiplier - 1.0, 1.0);
    double x_increment = std::max(box.width * density_multiplier - 1.0, 1.0);

    double sin = 0.0;
    double cos = 1.0;

    core::DoubleRect transformed_area(area.left, area.top, area.width, area.height);  
    if (!properties.randomize_rotation)
    {
        sin = std::sin(properties.rotation.radians());
        cos = std::cos(properties.rotation.radians());

        transformed_area = core::transform_rect(transformed_area, -sin, cos);
    }

    double center_x = transformed_area.left + transformed_area.width * 0.5;
    double center_y = transformed_area.top + transformed_area.height * 0.5;

    for (double y = transformed_area.top; y < transformed_area.right(); y += y_increment)
    {
        for (double x = transformed_area.left; x < transformed_area.right(); x += x_increment)
        {
            auto position = core::transform_point({ x - center_x, y - center_y }, sin, cos);

            Tile tile;
            tile.id = tile_group.id();
            tile.position.x = position.x + center_x + offset_dist(rng) * box.width;
            tile.position.y = position.y + center_y + offset_dist(rng) * box.height;

            tile.rotation = properties.rotation;
            if (properties.randomize_rotation)
            {
                double deg = rotation_dist(rng);
                tile.rotation = core::Rotation<double>::degrees(deg);
            }

            if (contains(area, tile.position))
            {
                *out = tile; ++out;
            }
        }       
    }
}



#endif