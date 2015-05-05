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

#ifndef TRACK_DISPLAY_HPP
#define TRACK_DISPLAY_HPP

#include "core/vector2.hpp"

#include <vector>
#include <unordered_map>
#include <functional>

#include <SFML/Graphics.hpp>

namespace components
{
    class Track;
    class TileLibrary;

    struct PlacedTile;    
}

namespace scene
{
    class TileMapping;
    struct TilePlacement;

    class DisplayLayer
    {
    public:
        void clear();
        void hide();
        void show();
        bool visible() const;

        const std::vector<sf::Vertex>& vertices() const;
        
        void insert_tile(std::size_t index);

        template <typename VertexIt>
        void insert_tile(std::size_t index, VertexIt it, VertexIt end, const sf::Texture* texture);

        void erase_tile(std::size_t index);

        void append_layer(const DisplayLayer& layer);

        template <typename VertexIt>
        void append_tile_vertices(std::size_t tile_index, VertexIt it, VertexIt end, const sf::Texture* texture);        

        std::size_t append_tile();

        template <typename VertexIt>
        std::size_t append_tile(VertexIt it, VertexIt end, const sf::Texture* texture);

        void erase_tile_vertices(std::size_t tile_id);

        template <typename VertexIt>
        void replace_tile_vertices(std::size_t tile_id, VertexIt it, VertexIt end, const sf::Texture* texture);

        void replace_tile_vertices(std::size_t tile_id, const DisplayLayer& layer);

        void translate_vertices(core::Vector2<double> offset);

        void draw(sf::RenderTarget& render_target, sf::RenderStates render_states) const;

    private:
        void insert_component_vertices(std::size_t vertex_index, std::size_t vertex_count, const sf::Texture* texture);

        struct Tile
        {
            std::size_t vertex_index = 0;
            std::size_t vertex_count = 0;
        };

        struct Component
        {
            std::size_t vertex_index = 0;
            std::size_t vertex_count = 0;
            const sf::Texture* texture = nullptr;
        };

        std::vector<Component> component_info_;
        std::vector<Tile> tile_info_;
        std::vector<sf::Vertex> vertices_;
        bool visible_ = true;
    };

    using DisplayLayerMap = std::unordered_map<std::size_t, DisplayLayer>;

    void draw(const DisplayLayer& layer, sf::RenderTarget& render_target, sf::RenderStates render_states);

    template <typename OutIt>
    void generate_tile_vertices(const components::PlacedTile& placed_tile, const TilePlacement& placement, OutIt out);

    DisplayLayerMap create_track_layer_map(const components::Track& track, const TileMapping& tile_mapping,
        std::function<void(double)> update_progress = {});


    template <typename TileIt>
    DisplayLayer create_display_layer(TileIt tile_it, TileIt tile_end, 
        const components::TileLibrary& tile_library, const TileMapping& tile_mapping);

    template <typename TileIt, typename TileCallback>
    DisplayLayer create_display_layer(TileIt tile_it, TileIt tile_end, const components::TileLibrary& tile_library, 
        const TileMapping& tile_mapping, TileCallback callback);
}

#include "track_display.inl"

#endif