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
        boost::optional<core::Rotation<double>> start_direction_override_;

        std::vector<ControlPoint> control_points_;

        TerrainLibrary terrain_library_;
        TileLibrary tile_library_;

        std::string track_name_;
        std::string track_path_;
        std::string track_author_;
        std::string track_pattern_;

        void sort_tile_list();
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

    void Track::define_tile(const TileDefinition& tile_def)
    {
        track_features_->tile_library_.define_tile(tile_def);
    }

    void Track::define_tile_group(const TileGroupDefinition& tile_group_def)
    {
        track_features_->tile_library_.define_tile_group(tile_group_def);
    }

    void Track::define_terrain(const TerrainDefinition& terrain_def)
    {
        track_features_->terrain_library_.define_terrain(terrain_def);
    }

    void Track::define_sub_terrain(const SubTerrain& sub_terrain)
    {
        track_features_->terrain_library_.define_sub_terrain(sub_terrain);
    }

    void Track::append_control_point(const ControlPoint& control_point)
    {
        std::uint32_t id = track_features_->control_points_.size();

        track_features_->control_points_.push_back(control_point);
        track_features_->control_points_.back().id = id;
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

    void Track::delete_last_start_point()
    {
        auto& start_points = track_features_->start_points_;
        if (!start_points.empty())
        {
            track_features_->start_points_.pop_back();
        }
    }

    core::Rotation<double> Track::start_direction() const
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
                return Rotation<double>::degrees(90.0);
            }

            return Rotation<double>::degrees(0.0);
        }

        return Rotation<double>::degrees(0.0);
    }
}