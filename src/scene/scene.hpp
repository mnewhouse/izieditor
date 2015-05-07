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

#ifndef SCENE_HPP
#define SCENE_HPP

#include "track_display.hpp"
#include "tile_mapping.hpp"

#include "components/track.hpp"
#include "components/pattern_store.hpp"

namespace components
{
    struct Tile;
    struct TileGroupDefinition;

    struct FillProperties;
}

namespace scene
{
    using components::LayerHandle;

    class Scene
    {
    public:
        Scene() = default;
        Scene(components::Track&& track);

        void resize_track(core::Vector2i new_size);

        void append_tile(std::size_t layer_id, const components::Tile& tile);

        template <typename TileIt>
        void append_tiles(std::size_t layer_id, TileIt it, TileIt end);

        void insert_tile(std::size_t layer_id, std::size_t tile_index, const components::Tile& tile);

        void update_tile(std::size_t layer_id, std::size_t tile_id, const components::Tile& tile);
        void update_tile_preview(std::size_t layer_id, std::size_t tile_id, const components::Tile& tile);

        void move_all_tiles(core::Vector2<double> offset);

        void move_tile(std::size_t layer_id, std::size_t tile_id, core::Vector2<double> offset);
        
        void rotate_tile(std::size_t layer_id, std::size_t tile_id, 
            core::Rotation<double> rotation_delta, core::Vector2<double> origin);

        void delete_tile(std::size_t layer_id, std::size_t tile_index);

        void delete_last_tile(std::size_t layer_id);
        void delete_last_tiles(std::size_t layer_id, std::size_t tile_count);

        void append_control_point(const components::ControlPoint& point);
        void delete_last_control_point();

        void append_start_point(const components::StartPoint& point);
        void delete_last_start_point();

        void define_pit(core::IntRect rect);
        void undefine_pit();

        const components::Track& track() const;
        const components::TileLibrary& tile_library() const;
        const components::PatternStore& pattern_store() const;

        const TileMapping& tile_mapping() const;
        const DisplayLayerMap& display_layers() const;

        components::ConstLayerHandle create_layer(const std::string& layer_name = std::string(), std::size_t level = 0);
        void delete_layer(std::size_t layer_id);

        void hide_layer(std::size_t layer_id);
        void show_layer(std::size_t layer_id);
        void move_layer(std::size_t layer_id, std::size_t new_index);
        void rename_layer(std::size_t layer_id, const std::string& new_name);
        void set_layer_level(std::size_t layer_id, std::size_t new_level);

        void restore_layer(std::size_t layer_id, std::size_t index);

        const std::vector<components::ConstLayerHandle>& layers() const;
        std::size_t layer_count() const;

        // Returns layer_count() if not found.
        std::size_t find_layer_index(std::size_t layer_id) const;

        std::vector<components::Tile> fill_area(std::size_t layer_id, const components::TileGroupDefinition& tile_group,
            const components::FillProperties& fill_properties);

        void draw(sf::RenderTarget& render_target, sf::RenderStates render_states) const;

    private:
        friend class SceneLoader;
        Scene(components::Track&& track, components::PatternStore&& pattern_loader,
            TileMapping&& tile_mapping, DisplayLayerMap&& display_layer_map);

        void rebuild_tile_vertices(DisplayLayer& layer, std::size_t tile_id, const components::Tile& tile);

        components::Track track_;
        components::PatternStore pattern_store_;
        TileMapping tile_mapping_;
        DisplayLayerMap track_display_;        

        std::vector<components::PlacedTile> tile_cache_;
        std::vector<sf::Vertex> vertex_cache_;
        DisplayLayer layer_cache_;
    };

    template <typename TileIt>
    void Scene::append_tiles(std::size_t layer_id, TileIt it, TileIt end)
    {
        for (; it != end; ++it)
        {
            append_tile(layer_id, *it);
        }
    }

    void draw(const Scene& scene, sf::RenderTarget& render_target, sf::RenderStates render_states);
}



#endif