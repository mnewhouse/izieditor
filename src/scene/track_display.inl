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


#ifndef TRACK_DISPLAY_INL
#define TRACK_DISPLAY_INL

#include "track_display.hpp"

#include "components/tile_definition.hpp"
#include "components/tile_group_expansion.hpp"

#include "core/rect.hpp"
#include "core/transform.hpp"

namespace scene
{
    template <typename OutIt>
    void generate_tile_vertices(const components::PlacedTile& placed_tile, const TilePlacement& placement, OutIt out)
    {
        const auto& tile = placed_tile.tile;
        const auto& tile_def = placed_tile.tile_def;

        const core::IntRect image_rect = tile_def->image_rect;
        const core::IntRect pattern_rect = tile_def->pattern_rect;  

        auto rotation = tile.rotation;
        auto cos = std::cos(rotation.radians());
        auto sin = std::sin(rotation.radians());

        const auto& texture_rect = placement.texture_rect;
        const auto& tile_rect = placement.tile_rect;

        const auto texture_left = static_cast<float>(texture_rect.left);
        const auto texture_top = static_cast<float>(texture_rect.top);
        const auto texture_right = static_cast<float>(texture_rect.right());
        const auto texture_bottom = static_cast<float>(texture_rect.bottom());

        auto scale_x = 0.5;
        auto scale_y = 0.5;

        if (pattern_rect.width * 2 != image_rect.width) scale_x = static_cast<double>(pattern_rect.width) / image_rect.width;
        if (pattern_rect.height * 2 != image_rect.height) scale_y = static_cast<double>(pattern_rect.height) / image_rect.height;

        const auto center_x = image_rect.width * scale_x * 0.5;
        const auto center_y = image_rect.height * scale_y * 0.5;

        auto source_left = tile_rect.left - 1.0;
        auto source_top = tile_rect.top - 1.0;

        auto source_right = static_cast<double>(tile_rect.right());
        auto source_bottom = static_cast<double>(tile_rect.bottom());

        const auto point = core::transform_point<double>({ -center_x, -center_y }, sin, cos);

        auto top_left = core::transform_point<double>({ source_left * scale_x - center_x, source_top * scale_y - center_y }, sin, cos);
        auto bottom_left = core::transform_point<double>({ source_left * scale_x - center_x, source_bottom * scale_y - center_y }, sin, cos);
        auto bottom_right = core::transform_point<double>({ source_right * scale_x - center_x, source_bottom * scale_y - center_y }, sin, cos);
        auto top_right = core::transform_point<double>({ source_right * scale_x - center_x, source_top * scale_y - center_y }, sin, cos);

        const auto tile_position = core::vector2_cast<double>(placed_tile.tile.position);

        top_left += tile_position;
        bottom_left += tile_position;
        bottom_right += tile_position;
        top_right += tile_position;

        sf::Vertex vertices[4];
        vertices[0].texCoords = sf::Vector2f(texture_left, texture_top);
        vertices[1].texCoords = sf::Vector2f(texture_left, texture_bottom);
        vertices[2].texCoords = sf::Vector2f(texture_right, texture_bottom);
        vertices[3].texCoords = sf::Vector2f(texture_right, texture_top);

        vertices[0].position = { static_cast<float>(top_left.x), static_cast<float>(top_left.y) };
        vertices[1].position = { static_cast<float>(bottom_left.x), static_cast<float>(bottom_left.y) };
        vertices[2].position = { static_cast<float>(bottom_right.x), static_cast<float>(bottom_right.y) };
        vertices[3].position = { static_cast<float>(top_right.x), static_cast<float>(top_right.y) };

        std::copy(std::begin(vertices), std::end(vertices), out);
    }

    template <typename TileIt>
    DisplayLayer create_display_layer(TileIt tile_it, TileIt tile_end,
        const components::TileLibrary& tile_library, const TileMapping& tile_mapping)
    {
        return create_display_layer(tile_it, tile_end, tile_library, tile_mapping, [](){});
    }

    template <typename TileIt, typename TileCallback>
    DisplayLayer create_display_layer(TileIt tile_it, TileIt tile_end, const components::TileLibrary& tile_library, 
        const TileMapping& tile_mapping, TileCallback tile_callback)
    {
        const sf::Texture* texture_hint = nullptr;
        std::vector<sf::Vertex> vertex_cache;

        DisplayLayer result;

        std::vector<components::PlacedTile> tile_cache;

        for (std::size_t tile_index = 0; tile_it != tile_end; ++tile_it, ++tile_index)
        {
            const components::Tile& tile = *tile_it;

            tile_cache.clear();
            components::expand_tile_groups(&tile, &tile + 1, tile_library, std::back_inserter(tile_cache));

            for (const auto& placed_tile : tile_cache)
            {
                auto placement_range = tile_mapping.find_tile(placed_tile.tile_def->id, texture_hint);
                for (const auto& placement : placement_range)
                {
                    texture_hint = placement.texture;

                    vertex_cache.clear();
                    generate_tile_vertices(placed_tile, placement, std::back_inserter(vertex_cache));

                    result.append_tile_vertices(tile_index, vertex_cache.begin(), vertex_cache.end(), placement.texture);
                }
            }

            tile_callback();
        }

        return result;
    }

    template <typename VertexIt>
    void DisplayLayer::insert_tile(std::size_t tile_index, VertexIt it, VertexIt end, const sf::Texture* texture)
    {
        insert_tile(tile_index);

        append_tile_vertices(tile_index, it, end, texture);
    }

    template <typename VertexIt>
    void DisplayLayer::append_tile_vertices(std::size_t tile_index, VertexIt it, VertexIt end, const sf::Texture* texture)
    {
        if (tile_index >= tile_info_.size())
        {
            insert_tile(tile_index, it, end, texture);
        }

        else
        {
            const auto& tile_info = tile_info_[tile_index];

            std::size_t vertex_index = tile_info.vertex_index + tile_info.vertex_count;
            std::size_t total_vertices = vertices_.size();

            vertices_.insert(vertices_.begin() + vertex_index, it, end);
            std::size_t vertex_count = vertices_.size() - total_vertices;

            tile_info_[tile_index].vertex_count += vertex_count;

            auto begin = tile_info_.begin() + tile_index + 1, end = tile_info_.end();
            std::transform(begin, end, begin,
                [vertex_count](Tile tile)
            {
                tile.vertex_index += vertex_count;
                return tile;
            });

            insert_component_vertices(vertex_index, vertex_count, texture);
        }
    }
}

#endif