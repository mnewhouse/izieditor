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

#ifndef TRACK_HPP
#define TRACK_HPP

#include "core/vector2.hpp"
#include "core/rotation.hpp"

#include "tile_definition.hpp"
#include "terrain_definition.hpp"
#include "track_layer.hpp"
#include "track_type.hpp"

#include <string>
#include <memory>
#include <cstddef>
#include <vector>

namespace components
{
    // The Track class provides abstractions for loading and representing
    // a track in memory, keeping track of tile definitions, terrain definitions,
    // placed tiles, control points, and all else that is needed.

    using core::Vector2u;
    
    class Track;

    class TileLibrary;
    class TerrainLibrary;
    
    struct TileDefinition;
    struct TileGroupDefinition;
    struct LevelTile;
    struct PlacedTile;

    struct StartPoint;
    struct ControlPoint;

    class Track
    {
    public:
        Track();
        Track(Track&& other);
        ~Track();

        Track& operator=(Track&& other);

        const TerrainLibrary& terrain_library() const;
        const TileLibrary& tile_library() const;

        const std::vector<std::string>& assets() const;
        void add_asset(std::string asset);

        void define_tile(const TileDefinition& tile_definition);
        void define_tile_group(const TileGroupDefinition& tile_group_definition);

        void define_terrain(const TerrainDefinition& terrain_definition);
        void define_sub_terrain(const SubTerrain& sub_terrain);
        void define_kill_terrain(TerrainId terrain_id);

        void define_contained_tile(TileDefinition tile_definition, 
            const std::string& pattern, const std::string& image);

        void define_contained_tile_group(const TileGroupDefinition& tile_group_definition);
        
        void define_contained_terrain(const TerrainDefinition& terrain);
        void define_contained_sub_terrain(const SubTerrain& sub_terrain);
        void define_contained_kill_terrain(TerrainId terrain_id);

        const std::vector<TileDefinition>& contained_tile_definitions() const;
        const std::vector<TileGroupDefinition>& contained_tile_group_definitions() const;

        const std::vector<TerrainDefinition>& contained_terrain_definitions() const;
        const std::vector<SubTerrain>& contained_sub_terrain_definitions() const;
        const std::vector<TerrainId>& contained_kill_terrains() const;

        void append_control_point(const ControlPoint& control_point);
        void insert_control_point(std::size_t index, const ControlPoint& control_point);
        void update_control_point(std::size_t index, ControlPoint point);
        void delete_control_point(std::size_t index);
        void delete_last_control_point();

        void append_start_point(const StartPoint& start_point);
        void insert_start_point(std::size_t index, const StartPoint& start_point);
        void update_start_points(const std::vector<StartPoint>& start_points);
        void delete_start_point(std::size_t index);
        void delete_last_start_point();

        const std::vector<ConstLayerHandle>& layers() const;
        std::size_t layer_count() const;

        LayerHandle create_layer(std::string name, std::size_t level);

        void disable_layer(LayerHandle layer);
        void disable_layer(std::size_t layer_id);

        void restore_layer(std::size_t layer_id, std::size_t index);

        LayerHandle layer_by_id(std::size_t id);
        ConstLayerHandle layer_by_id(std::size_t id) const;        

        void move_layer(ConstLayerHandle layer, std::size_t new_index);
        void move_layer(std::size_t layer_id, std::size_t new_index);

        void set_layer_level(LayerHandle layer, std::size_t new_level);
        void set_layer_level(std::size_t layer_id, std::size_t new_level);

        const std::vector<StartPoint>& start_points() const;

        const std::vector<ControlPoint>& control_points() const;

        bool is_start_direction_overridden() const;
        void use_default_start_direction();

        std::int32_t start_direction() const;
        void set_start_direction(std::int32_t start_direction);

        void set_gravity_strength(std::int32_t gravity_strength);
        std::int32_t gravity_strength() const;

        void set_gravity_direction(std::int32_t gravity_direction);
        std::int32_t gravity_direction() const;

        void define_pit(core::IntRect pit);
        void undefine_pit();

        const core::IntRect* pit() const;

        void set_size(Vector2u size);
        void set_num_levels(std::size_t levels);

        void set_name(std::string track_name);
        void set_path(std::string file_path);
        void set_author(std::string author);
        void set_pattern(std::string pattern);

        Vector2u size() const;
        std::size_t num_levels() const;

        TrackType track_type() const;
        void set_track_type(TrackType track_type);

        const std::string& name() const;
        const std::string& path() const;
        const std::string& author() const;
        const std::string& pattern() const;

    private:
        struct TrackFeatures;
        std::unique_ptr<TrackFeatures> track_features_;
    };
}

#endif