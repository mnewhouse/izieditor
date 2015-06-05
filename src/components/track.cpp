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

#include "track.hpp"

#include "terrain_library.hpp"
#include "tile_library.hpp"
#include "track_loader.hpp"
#include "start_point.hpp"
#include "control_point.hpp"

#include "core/transform.hpp"

#include <boost/optional.hpp>

namespace components
{
    struct Track::TrackFeatures
    {
        Vector2u size_;
        std::size_t num_levels_ = 1;

        std::vector<std::string> assets_;

        std::map<std::size_t, TrackLayer> layers_;
        std::vector<ConstLayerHandle> layer_order_;

        std::vector<StartPoint> start_points_;
        boost::optional<std::int32_t> start_direction_override_;
        boost::optional<core::IntRect> pit_;

        std::int32_t gravity_strength_ = 0;
        std::int32_t gravity_direction_ = 0;
        TrackType track_type_ = TrackType::Racing;

        std::vector<ControlPoint> control_points_;

        TerrainLibrary terrain_library_;
        TileLibrary tile_library_;

        std::string track_name_;
        std::string track_path_;
        std::string track_author_;
        std::string track_pattern_;

        std::vector<TileDefinition> contained_tiles_;
        std::vector<TileGroupDefinition> contained_tile_groups_;
        std::vector<TerrainDefinition> contained_terrains_;
        std::vector<SubTerrain> contained_sub_terrains_;
        std::vector<TerrainId> contained_kill_terrains_;



        void remove_contained_tile_definition(components::TileId tile_id);

        std::size_t allocate_layer_id();
    };

    Track::Track()
        : track_features_(std::make_unique<TrackFeatures>())
    {
    }

    Track::~Track()
    {
    }

    Track::Track(Track&& other)
        : track_features_(std::move(other.track_features_))
    {
    }

    Track& Track::operator=(Track&& other)
    {
        track_features_ = std::move(other.track_features_);
        return *this;
    }

    const TileLibrary& Track::tile_library() const
    {
        return track_features_->tile_library_;
    }

    const TerrainLibrary& Track::terrain_library() const
    {
        return track_features_->terrain_library_;
    }

    const std::vector<std::string>& Track::assets() const
    {
        return track_features_->assets_;
    }

    void Track::add_asset(std::string asset)
    {
        track_features_->assets_.push_back(std::move(asset));
    }

    LayerHandle Track::create_layer(std::string name, std::size_t level)
    {
        auto& layer_order = track_features_->layer_order_;
        auto& layers = track_features_->layers_;

        std::size_t layer_id = 0;
        if (!layers.empty())
        {
            layer_id = std::prev(layers.end())->first + 1;
        }

        TrackLayer new_layer;
        new_layer.level = level;
        new_layer.name = std::move(name);

        auto insertion = layers.emplace(layer_id, std::move(new_layer));
        if (insertion.second)
        {
            LayerHandle layer_handle(&insertion.first->second, layer_id);

            auto position = std::upper_bound(layer_order.begin(), layer_order.end(), level,
                [](std::size_t level, const ConstLayerHandle& layer)
            {
                return level < layer->level;
            });

            layer_order.emplace(position, layer_handle);
            return layer_handle;
        }

        return LayerHandle();
    }

    void Track::disable_layer(LayerHandle layer)
    {
        if (layer)
        {
            auto& order = track_features_->layer_order_;
            order.erase(std::remove(order.begin(), order.end(), layer), order.end());
        }
    }

    void Track::disable_layer(std::size_t layer_id)
    {
        disable_layer(layer_by_id(layer_id));
    }

    void Track::restore_layer(std::size_t layer_id, std::size_t index)
    {
        if (auto layer = layer_by_id(layer_id))
        {
            auto& order = track_features_->layer_order_;

            index = std::min(order.size(), index);
            order.insert(order.begin() + index, layer);
        }
    }

    const std::vector<StartPoint>& Track::start_points() const
    {
        return track_features_->start_points_;
    }

    const std::vector<ControlPoint>& Track::control_points() const
    {
        return track_features_->control_points_;
    }

    const std::vector<ConstLayerHandle>& Track::layers() const
    {
        return track_features_->layer_order_;
    }

    std::size_t Track::layer_count() const
    {
        return track_features_->layers_.size();
    }

    LayerHandle Track::layer_by_id(std::size_t index)
    {
        auto it = track_features_->layers_.find(index);
        if (it == track_features_->layers_.end())
        {
            return LayerHandle();
        }

        return LayerHandle(&it->second, index);
    }

    ConstLayerHandle Track::layer_by_id(std::size_t index) const
    {
        auto it = track_features_->layers_.find(index);
        if (it == track_features_->layers_.end())
        {
            return ConstLayerHandle();
        }

        return ConstLayerHandle(&it->second, index);
    }

    void Track::move_layer(ConstLayerHandle layer, std::size_t new_index)
    {
        auto& order = track_features_->layer_order_;
        auto it = std::find(order.begin(), order.end(), layer);

        if (it != order.end())
        {
            auto value = *it;
            order.erase(it);

            new_index = std::min(order.size(), new_index);
            order.insert(order.begin() + new_index, value);
            std::stable_sort(order.begin(), order.end(), 
                [](const ConstLayerHandle& a, const ConstLayerHandle& b)
            {
                return a->level < b->level;
            });
        }
    }

    void Track::move_layer(std::size_t layer_id, std::size_t new_index)
    {
        move_layer(layer_by_id(layer_id), new_index);
    }

    void Track::set_layer_level(LayerHandle layer, std::size_t new_level)
    {
        layer->level = new_level;

        auto& order = track_features_->layer_order_;
        std::stable_sort(order.begin(), order.end(), 
            [](const ConstLayerHandle& a, const ConstLayerHandle& b)
        {
            return a->level < b->level;
        });
    }

    void Track::set_layer_level(std::size_t layer_id, std::size_t new_level)
    {
        set_layer_level(layer_by_id(layer_id), new_level);
    }

    core::Vector2u Track::size() const
    {
        return track_features_->size_;
    }

    void Track::set_size(Vector2u size)
    {
        track_features_->size_ = size;
    }

    std::size_t Track::num_levels() const
    {
        return track_features_->num_levels_;
    }

    void Track::set_num_levels(std::size_t num_levels)
    {
        track_features_->num_levels_ = num_levels;
    }

    void Track::set_track_type(TrackType track_type)
    {
        track_features_->track_type_ = track_type;
    }

    TrackType Track::track_type() const
    {
        return track_features_->track_type_;
    }

    const std::string& Track::name() const
    {
        return track_features_->track_name_;
    }

    void Track::set_name(std::string name)
    {
        track_features_->track_name_ = std::move(name);
    }

    const std::string& Track::path() const
    {
        return track_features_->track_path_;
    }

    void Track::set_path(std::string path)
    {
        track_features_->track_path_ = std::move(path);
    }

    const std::string& Track::author() const
    {
        return track_features_->track_author_;
    }

    void Track::set_author(std::string author)
    {
        track_features_->track_author_ = std::move(author);
    }

    const std::string& Track::pattern() const
    {
        return track_features_->track_pattern_;
    }

    void Track::set_pattern(std::string pattern)
    {
        track_features_->track_pattern_ = std::move(pattern);
    }

    void Track::TrackFeatures::remove_contained_tile_definition(components::TileId tile_id)
    {
        auto& tiles = contained_tiles_;
        tiles.erase(std::remove_if(tiles.begin(), tiles.end(),
            [tile_id](const TileDefinition& tile_def)
        {
            return tile_def.id == tile_id;
        }), tiles.end());

        auto& tile_groups = contained_tile_groups_;
        tile_groups.erase(std::remove_if(tile_groups.begin(), tile_groups.end(),
            [tile_id](const TileGroupDefinition& tile_group)
        {
            return tile_group.id() == tile_id;
        }), tile_groups.end());
    }

    void Track::define_tile(const TileDefinition& tile_def)
    {
        track_features_->remove_contained_tile_definition(tile_def.id);
        track_features_->tile_library_.define_tile(tile_def);
    }

    void Track::define_tile_group(const TileGroupDefinition& tile_group_def)
    {
        track_features_->remove_contained_tile_definition(tile_group_def.id());

        track_features_->tile_library_.define_tile_group(tile_group_def);
    }

    void Track::define_terrain(const TerrainDefinition& terrain_def)
    {
        auto terrain_id = terrain_def.id;
        auto& contained = track_features_->contained_terrains_;
        contained.erase(std::remove_if(contained.begin(), contained.end(),
            [terrain_id](const TerrainDefinition& terrain_def)
        {
            return terrain_def.id == terrain_id;
        }), contained.end());

        track_features_->terrain_library_.define_terrain(terrain_def);
    }

    void Track::define_sub_terrain(const SubTerrain& sub_terrain)
    {
        track_features_->terrain_library_.define_sub_terrain(sub_terrain);
    }

    void Track::define_kill_terrain(TerrainId terrain_id)
    {
        auto& kill_terrains = track_features_->contained_kill_terrains_;
        kill_terrains.erase(std::remove(kill_terrains.begin(), kill_terrains.end(), terrain_id));

        track_features_->terrain_library_.define_kill_terrain(terrain_id);
    }

    void Track::define_contained_tile(TileDefinition tile_def,
        const std::string& pattern, const std::string& image)
    {
        define_tile(tile_def);

        tile_def.pattern_file = pattern;
        tile_def.image_file = image;
        track_features_->contained_tiles_.push_back(tile_def);
    }

    void Track::define_contained_tile_group(const TileGroupDefinition& tile_group_definition)
    {
        define_tile_group(tile_group_definition);

        track_features_->contained_tile_groups_.push_back(tile_group_definition);
    }

    void Track::define_contained_terrain(const TerrainDefinition& terrain_definition)
    {
        define_terrain(terrain_definition);

        track_features_->contained_terrains_.push_back(terrain_definition);
    }

    void Track::define_contained_sub_terrain(const SubTerrain& sub_terrain)
    {
        define_sub_terrain(sub_terrain);

        track_features_->contained_sub_terrains_.push_back(sub_terrain);
    }

    void Track::define_contained_kill_terrain(TerrainId terrain_id)
    {
        track_features_->contained_kill_terrains_.push_back(terrain_id);
    }


    void Track::append_control_point(const ControlPoint& control_point)
    {
        std::uint32_t id = track_features_->control_points_.size();

        track_features_->control_points_.push_back(control_point);
        track_features_->control_points_.back().id = id;
    }

    void Track::insert_control_point(std::size_t index, const ControlPoint& control_point)
    {
        auto& control_points = track_features_->control_points_;
        if (index < control_points.size())
        {
            control_points.insert(control_points.begin() + index, control_point);
            for (; index != control_points.size(); ++index)
            {
                control_points[index].id = index;
            }
        }
    }

    void Track::update_control_point(std::size_t index, ControlPoint point)
    {
        point.id = index;
        if (index < track_features_->control_points_.size())
        {
            track_features_->control_points_[index] = point;
        }
    }
    
    void Track::delete_control_point(std::size_t index)
    {
        auto& control_points = track_features_->control_points_;
        if (index < control_points.size())
        {
            control_points.erase(control_points.begin() + index);
            
            for (; index != control_points.size(); ++index)
            {
                control_points[index].id = index;
            }
        }
    }

    void Track::delete_last_control_point()
    {
        auto& control_points = track_features_->control_points_;
        
        if (!control_points.empty())
        {
            control_points.pop_back();
        }
    }

    void Track::append_start_point(const StartPoint& start_point)
    {
        track_features_->start_points_.push_back(start_point);
    }

    void Track::insert_start_point(std::size_t index, const StartPoint& start_point)
    {
        auto& start_points = track_features_->start_points_;
        if (index < start_points.size())
        {
            start_points.insert(start_points.begin() + index, start_point);
        }
    }

    void Track::update_start_points(const std::vector<StartPoint>& start_points)
    {
        track_features_->start_points_ = start_points;

        if (start_points.size() > 20)
        {
            track_features_->start_points_.resize(20);
        }
    }

    void Track::delete_start_point(std::size_t index)
    {
        auto& start_points = track_features_->start_points_;
        if (index < start_points.size())
        {
            start_points.erase(start_points.begin() + index);
        }
    }

    void Track::delete_last_start_point()
    {
        auto& start_points = track_features_->start_points_;
        if (!start_points.empty())
        {
            track_features_->start_points_.pop_back();
        }
    }

    bool Track::is_start_direction_overridden() const
    {
        return track_features_->start_direction_override_.is_initialized();
    }

    void Track::use_default_start_direction()
    {
        track_features_->start_direction_override_ = boost::none;
    }

    void Track::set_start_direction(std::int32_t start_direction)
    {
        track_features_->start_direction_override_ = start_direction;
    }

    std::int32_t Track::start_direction() const
    {
        if (track_features_->start_direction_override_)
        {
            return *track_features_->start_direction_override_;
        }

        if (!control_points().empty())
        {
            const auto& finish_line = control_points().front();
            if (finish_line.direction == ControlPoint::Horizontal)
            {
                return 90;
            }
        }

        return 0;
    }

    void Track::set_gravity_strength(std::int32_t gravity_strength)
    {
        track_features_->gravity_strength_ = std::min(std::max(gravity_strength, 0), 10000);
    }

    std::int32_t Track::gravity_strength() const
    {
        return track_features_->gravity_strength_;
    }

    void Track::set_gravity_direction(std::int32_t gravity_direction)
    {
        track_features_->gravity_direction_ = std::min(std::max(gravity_direction, 0), 359);
    }

    std::int32_t Track::gravity_direction() const
    {
        return track_features_->gravity_direction_;
    }

    void Track::define_pit(core::IntRect pit)
    {
        track_features_->pit_ = pit;
    }

    void Track::undefine_pit()
    {
        track_features_->pit_ = boost::none;
    }

    const core::IntRect* Track::pit() const
    {
        return track_features_->pit_.get_ptr();
    }

    const std::vector<TileDefinition>& Track::contained_tile_definitions() const
    {
        return track_features_->contained_tiles_;
    }

    const std::vector<TileGroupDefinition>& Track::contained_tile_group_definitions() const
    {
        return track_features_->contained_tile_groups_;
    }

    const std::vector<TerrainDefinition>& Track::contained_terrain_definitions() const
    {
        return track_features_->contained_terrains_;
    }

    const std::vector<SubTerrain>& Track::contained_sub_terrain_definitions() const
    {
        return track_features_->contained_sub_terrains_;
    }

    const std::vector<TerrainId>& Track::contained_kill_terrains() const
    {
        return track_features_->contained_kill_terrains_;
    }
}