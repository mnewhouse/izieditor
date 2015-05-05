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

#include "track_display.hpp"

#include "tile_mapping.hpp"

#include "components/track.hpp"
#include "components/tile_group_expansion.hpp"

#include "core/transform.hpp"

#include <cmath>

namespace scene
{
    DisplayLayerMap create_track_layer_map(const components::Track& track, const TileMapping& tile_mapping,
        std::function<void(double)> update_progress)
    {
        DisplayLayerMap layer_map;

        std::size_t num_tiles = 0;
        for (const auto& layer_handle : track.layers())
        {
            num_tiles += layer_handle->tiles.size();
        }

        double progress = 0.0;
        auto tile_callback = [&]()
        {
            progress += num_tiles != 0 ? 1.0 / num_tiles : 0.0;

            if (update_progress)
            {
                update_progress(progress);
            }
        };

        const auto& tile_library = track.tile_library();
        for (const auto& layer_handle : track.layers())
        {
            const auto& tiles = layer_handle->tiles;

            layer_map.emplace(layer_handle.id(), create_display_layer(tiles.begin(), tiles.end(), tile_library, tile_mapping, tile_callback));
        }

        return layer_map;
    }

    void DisplayLayer::hide()
    {
        visible_ = false;
    }

    void DisplayLayer::show()
    {
        visible_ = true;
    }

    bool DisplayLayer::visible() const
    {
        return visible_;
    }


    void DisplayLayer::clear()
    {
        component_info_.clear();
        tile_info_.clear();
        vertices_.clear();
    }
    
    const std::vector<sf::Vertex>& DisplayLayer::vertices() const
    {
        return vertices_;
    }

    void draw(const DisplayLayer& layer, sf::RenderTarget& render_target, sf::RenderStates render_states)
    {
        layer.draw(render_target, render_states);
    }

    void DisplayLayer::draw(sf::RenderTarget& render_target, sf::RenderStates render_states) const
    {
        if (!visible()) return;

        const sf::Vertex* vertices = vertices_.data();

        for (const Component& component : component_info_)
        {
            render_states.texture = component.texture;
            render_target.draw(vertices + component.vertex_index, static_cast<unsigned int>(component.vertex_count),
                sf::Quads, render_states);
        }
    }

    void DisplayLayer::insert_tile(std::size_t tile_index)
    {
        if (tile_index >= tile_info_.size())
        {
            Tile tile_initializer;
            tile_initializer.vertex_index = vertices_.size();
            tile_initializer.vertex_count = 0;

            tile_info_.resize(tile_index + 1, tile_initializer);
        }

        else
        {
            auto tile_info = tile_info_[tile_index];
            tile_info.vertex_count = 0;
            tile_info_.insert(tile_info_.begin() + tile_index, tile_info);
        }
    }

    void DisplayLayer::insert_component_vertices(std::size_t vertex_index, std::size_t vertex_count, 
        const sf::Texture* texture)
    {
        auto component_it = std::lower_bound(component_info_.begin(), component_info_.end(), vertex_index,
            [](const Component& component, std::size_t vertex_index)
        {
            return component.vertex_index + component.vertex_count < vertex_index;
        });

        auto shift_components = [vertex_count](Component component)
        {
            component.vertex_index += vertex_count;
            return component;
        };

        if (component_it != component_info_.end())
        {
            if (component_it->texture == texture)
            {
                component_it->vertex_count += vertex_count;
                std::transform(component_it + 1, component_info_.end(), component_it + 1, shift_components);
            }

            else
            {
                std::size_t component_offset = vertex_index - component_it->vertex_index;
                
                Component inserted_components[2];
                inserted_components[0].vertex_index = vertex_index;
                inserted_components[0].vertex_count = vertex_count;
                inserted_components[0].texture = texture;

                inserted_components[1].vertex_index = vertex_index + vertex_count;
                inserted_components[1].vertex_count = component_it->vertex_count - component_offset;
                inserted_components[1].texture = component_it->texture;

                std::size_t insertion_count = inserted_components[1].vertex_count != 0 ? 2 : 1;
                component_it = component_info_.insert(component_it + 1, inserted_components, 
                    inserted_components + insertion_count);

                std::transform(component_it + insertion_count, component_info_.end(), component_it + 1, 
                    shift_components);
            }
        }

        else
        {
            if (!component_info_.empty() && component_info_.back().texture == texture)
            {
                component_info_.back().vertex_count += vertex_count;
            }

            else
            {
                Component component;                
                component.texture = texture;
                component.vertex_index = vertex_index;
                component.vertex_count = vertex_count;
                component_info_.push_back(component);                
            }
        }
    }

    void DisplayLayer::erase_tile(std::size_t tile_index)
    {
        if (tile_index < tile_info_.size())
        {
            erase_tile_vertices(tile_index);

            tile_info_.erase(tile_info_.begin() + tile_index);
        }
    }

    void DisplayLayer::erase_tile_vertices(std::size_t tile_index)
    {
        if (tile_index < tile_info_.size())
        {
            auto& tile_info = tile_info_[tile_index];
            std::size_t vertex_index = tile_info.vertex_index;
            std::size_t vertex_count = tile_info.vertex_count;

            vertices_.erase(vertices_.begin() + vertex_index, vertices_.begin() + vertex_index + vertex_count);

            auto shift_it = tile_info_.begin() + tile_index;
            std::transform(shift_it, tile_info_.end(), shift_it,
                [vertex_count](Tile tile)
            {
                tile.vertex_index -= vertex_count;
                return tile;
            });

            auto component_it = std::lower_bound(component_info_.begin(), component_info_.end(), vertex_index,
                [](Component component, std::size_t vertex_index)
            {
                return component.vertex_index + component.vertex_count < vertex_index;
            });

            component_it->vertex_count -= vertex_count;
            if (component_it->vertex_count == 0)
            {
                component_it = component_info_.erase(component_it);
                std::transform(component_it, component_info_.end(), component_it,
                    [vertex_count](Component component)
                {
                    component.vertex_index -= vertex_count;
                    return component;
                });                  
            }

            tile_info.vertex_count = 0;
        }
    }

    void DisplayLayer::replace_tile_vertices(std::size_t tile_index, const DisplayLayer& layer)
    {
        if (tile_index < tile_info_.size())
        {
            const auto& tile_info = tile_info_[tile_index];
            std::size_t vertex_index = tile_info.vertex_index;

            auto component_it = std::lower_bound(component_info_.begin(), component_info_.end(), vertex_index,
                [](const Component& component, std::size_t vertex_index)
            {
                return component.vertex_index + component.vertex_count < vertex_index;
            });

            if (component_it != component_info_.end() && layer.component_info_.size() == 1 && 
                layer.component_info_.front().texture == component_it->texture && tile_info.vertex_count == layer.vertices_.size())
            {
                std::copy(layer.vertices_.begin(), layer.vertices_.end(), vertices_.begin() + vertex_index);
            }            

            else
            {
                erase_tile_vertices(tile_index);

                for (const auto& component : layer.component_info_)
                {
                    auto vertex_it = layer.vertices_.begin() + component.vertex_index;
                    append_tile_vertices(tile_index, vertex_it, vertex_it + component.vertex_count, component.texture);
                }                
            }
        }
    }

    void DisplayLayer::translate_vertices(core::Vector2<double> offset)
    {
        const auto x = static_cast<float>(offset.x);
        const auto y = static_cast<float>(offset.y);

        for (auto& vertex : vertices_)
        {
            vertex.position.x += x;
            vertex.position.y += y;
        }
    }
}