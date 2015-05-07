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

#include "scene.hpp"
#include "tile_partitioner.hpp"
#include "track_display.hpp"

#include "components/component_algorithms.hpp"

#include <random>
#include <chrono>

namespace scene
{
    Scene::Scene(components::Track&& track)
        : track_(std::move(track)),
          pattern_store_(components::load_pattern_files(track_.tile_library())),
          tile_mapping_(create_tile_mapping(track_.tile_library())),
          track_display_(create_track_layer_map(track_, tile_mapping_))
    {
    }

    Scene::Scene(components::Track&& track, components::PatternStore&& pattern_store,
        TileMapping&& tile_mapping, DisplayLayerMap&& track_display)
        : track_(std::move(track)),
          pattern_store_(std::move(pattern_store)),
          tile_mapping_(std::move(tile_mapping)),
          track_display_(std::move(track_display))
    {
    }

    const components::Track& Scene::track() const
    {
        return track_;
    }

    const components::PatternStore& Scene::pattern_store() const
    {
        return pattern_store_;
    }

    const components::TileLibrary& Scene::tile_library() const
    {
        return track_.tile_library();
    }

    const TileMapping& Scene::tile_mapping() const
    {
        return tile_mapping_;
    }

    const DisplayLayerMap& Scene::display_layers() const
    {
        return track_display_;
    }

    void Scene::resize_track(core::Vector2i new_size)
    {
        track_.set_size(new_size);
    }

    void Scene::update_tile(std::size_t layer_id, std::size_t tile_index, const components::Tile& tile)
    {
        if (auto layer = track_.layer_by_id(layer_id))
        {
            if (tile_index < layer->tiles.size())
            {
                layer->tiles[tile_index] = tile;
            }

            update_tile_preview(layer_id, tile_index, tile);
        }
    }

    void Scene::update_tile_preview(std::size_t layer_id, std::size_t tile_index, const components::Tile& tile)
    {
        rebuild_tile_vertices(track_display_[layer_id], tile_index, tile);
    }
   
    void Scene::append_tile(std::size_t layer_id, const components::Tile& tile)
    {
        if (auto layer = track_.layer_by_id(layer_id))
        {
            std::size_t tile_index = layer->tiles.size();
            layer->tiles.push_back(tile);

            tile_cache_.clear();
            components::expand_tile_groups(&tile, &tile + 1, track_.tile_library(), std::back_inserter(tile_cache_));

            const sf::Texture* texture_hint = nullptr;

            auto& display_layer = track_display_[layer_id];

            for (const auto& tile : tile_cache_)
            {
                auto placement_range = tile_mapping_.find_tile(tile.tile_def->id, texture_hint);
                for (const auto& placement : placement_range)
                {
                    vertex_cache_.clear();
                    generate_tile_vertices(tile, placement, std::back_inserter(vertex_cache_));

                    display_layer.append_tile_vertices(tile_index, vertex_cache_.begin(), vertex_cache_.end(),
                        placement.texture);

                    texture_hint = placement.texture;
                }
            }            
        }
    }

    void Scene::insert_tile(std::size_t layer_id, std::size_t tile_index, const components::Tile& tile)
    {
        if (auto layer = track_.layer_by_id(layer_id))
        {
            tile_index = std::min(tile_index, layer->tiles.size());

            layer->tiles.insert(layer->tiles.begin() + tile_index, tile);
            
            auto& display_layer = track_display_[layer_id];
            display_layer.insert_tile(tile_index);
            rebuild_tile_vertices(display_layer, tile_index, tile);
        }
    }

    void Scene::rebuild_tile_vertices(DisplayLayer& display_layer, std::size_t tile_index, const components::Tile& tile)
    {
        tile_cache_.clear();
        components::expand_tile_groups(&tile, &tile + 1, track_.tile_library(), std::back_inserter(tile_cache_));

        const sf::Texture* texture_hint = nullptr;
        layer_cache_.clear();        

        for (const auto& tile : tile_cache_)
        {
            auto placement_range = tile_mapping_.find_tile(tile.tile_def->id, texture_hint);
            for (const auto& placement : placement_range)
            {
                vertex_cache_.clear();
                generate_tile_vertices(tile, placement, std::back_inserter(vertex_cache_));

                layer_cache_.append_tile_vertices(tile_index, vertex_cache_.begin(), vertex_cache_.end(),
                    placement.texture);

                texture_hint = placement.texture;
            }
        }

        display_layer.replace_tile_vertices(tile_index, layer_cache_);
    }

    void Scene::move_all_tiles(core::Vector2<double> offset)
    {
        for (std::size_t layer_id = 0; layer_id != track_.layer_count(); ++layer_id)
        {
            if (auto layer = track_.layer_by_id(layer_id))
            {
                auto& display_layer = track_display_[layer_id];

                for (auto& tile : layer->tiles)
                {
                    tile.position += offset;
                }

                display_layer.translate_vertices(offset);
            }
        }
    }

    void Scene::move_tile(std::size_t layer_id, std::size_t tile_id, core::Vector2<double> offset)
    {
        if (auto layer = track_.layer_by_id(layer_id))
        {
            auto& tile = layer->tiles[tile_id];
            tile.position += offset;

            rebuild_tile_vertices(track_display_[layer_id], tile_id, tile);
        }
    }

    void Scene::rotate_tile(std::size_t layer_id, std::size_t tile_id, 
        core::Rotation<double> rotation_delta, core::Vector2<double> origin)
    {
        if (auto layer = track_.layer_by_id(layer_id))
        {
            auto& tile = layer->tiles[tile_id];
            tile.rotation += rotation_delta;

            auto offset = core::transform_point(tile.position - origin, rotation_delta);
            tile.position = origin + offset;

            rebuild_tile_vertices(track_display_[layer_id], tile_id, tile);
        }
    }

    void Scene::delete_tile(std::size_t layer_id, std::size_t tile_index)
    {
        if (auto layer = track_.layer_by_id(layer_id))
        {
            if (tile_index < layer->tiles.size())
            {
                layer->tiles.erase(layer->tiles.begin() + tile_index);

                track_display_[layer_id].erase_tile(tile_index);
            }
        }
    }

    void Scene::delete_last_tile(std::size_t layer_id)
    {
        if (auto layer = track_.layer_by_id(layer_id))
        {
            layer->tiles.pop_back();

            std::size_t tile_index = layer->tiles.size();
            track_display_[layer_id].erase_tile_vertices(tile_index);
        }
    }

    void Scene::delete_last_tiles(std::size_t layer_id, std::size_t tile_count)
    {
        if (auto layer = track_.layer_by_id(layer_id))
        {
            std::size_t tile_index = layer->tiles.size();
            auto& display_layer = track_display_[layer_id];
            
            for (std::size_t n = 0; n != tile_count; ++n)
            {
                layer->tiles.pop_back();
                display_layer.erase_tile_vertices(--tile_index);
            }
        }
    }

    void Scene::append_control_point(const components::ControlPoint& control_point)
    {
        track_.append_control_point(control_point);
    }

    void Scene::delete_last_control_point()
    {
        track_.delete_last_control_point();
    }

    void Scene::append_start_point(const components::StartPoint& start_point)
    {
        track_.append_start_point(start_point);
    }

    void Scene::delete_last_start_point()
    {
        track_.delete_last_start_point();
    }

    components::ConstLayerHandle Scene::create_layer(const std::string& name, std::size_t level)
    {
        return track_.create_layer(name, level);
    }

    void Scene::delete_layer(std::size_t layer_id)
    {
        track_.disable_layer(layer_id);
    }

    void Scene::restore_layer(std::size_t layer_id, std::size_t index)
    {
        track_.restore_layer(layer_id, index);
    }

    void Scene::hide_layer(std::size_t layer_id)
    {
        if (auto layer = track_.layer_by_id(layer_id))
        {
            layer->visible = false;
        }

        auto map_it = track_display_.find(layer_id);
        if (map_it != track_display_.end())
        {
            map_it->second.hide();
        }
    }

    void Scene::show_layer(std::size_t layer_id)
    {
        if (auto layer = track_.layer_by_id(layer_id))
        {
            layer->visible = true;
        }

        auto map_it = track_display_.find(layer_id);
        if (map_it != track_display_.end())
        {
            map_it->second.show();
        }
    }

    void Scene::move_layer(std::size_t layer_id, std::size_t new_index)
    {
        track_.move_layer(layer_id, new_index);
    }

    void Scene::rename_layer(std::size_t layer_id, const std::string& new_name)
    {
        if (auto layer = track_.layer_by_id(layer_id))
        {
            layer->name = new_name;
        }
    }

    void Scene::set_layer_level(std::size_t layer_id, std::size_t new_level)
    {
        track_.set_layer_level(layer_id, new_level);
    }

    const std::vector<components::ConstLayerHandle>& Scene::layers() const
    {
        return track_.layers();
    }

    std::size_t Scene::layer_count() const
    {
        return layers().size();
    }

    std::size_t Scene::find_layer_index(std::size_t layer_id) const
    {
        auto it = std::find_if(layers().begin(), layers().end(), 
            [layer_id](components::ConstLayerHandle layer)
        {
            return layer.id() == layer_id;
        });

        return std::distance(layers().begin(), it);
    }

    void Scene::draw(sf::RenderTarget& render_target, sf::RenderStates render_states) const
    {
        for (const auto& layer_handle : track_.layers())
        {
            auto map_it = track_display_.find(layer_handle.id());
            if (map_it != track_display_.end())
            {
                scene::draw(map_it->second, render_target, render_states);
            }
        }
    }

    void Scene::define_pit(core::IntRect pit)
    {
        track_.define_pit(pit);
    }

    void Scene::undefine_pit()
    {
        track_.undefine_pit();
    }

    void draw(const Scene& scene, sf::RenderTarget& render_target, sf::RenderStates render_states)
    {
        scene.draw(render_target, render_states);
    }

    std::vector<components::Tile> Scene::fill_area(std::size_t layer_id, const components::TileGroupDefinition& tile_group,
        const components::FillProperties& properties)
    {
        auto time = std::chrono::high_resolution_clock::now().time_since_epoch();        
        std::mt19937_64 rng(std::chrono::duration_cast<std::chrono::milliseconds>(time).count());

        std::vector<components::Tile> tiles;
        components::fill_area(tile_group, track_.tile_library(), properties,
            rng, std::back_inserter(tiles));

        for (const auto& tile : tiles)
        {
            append_tile(layer_id, tile);
        }

        return tiles;
    }
}