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

#include "track_loader.hpp"
#include "track.hpp"
#include "terrain_definition.hpp"
#include "tile_definition.hpp"
#include "control_point.hpp"
#include "start_point.hpp"

#include "include_path.hpp"
#include "component_readers.hpp"

#include "core/config.hpp"
#include "core/directive_reader.hpp"

#include <unordered_set>
#include <sstream>
#include <fstream>
#include <iostream>

#include <boost/filesystem.hpp>

namespace components
{
    using namespace readers;

    using core::Vector2i;

    struct TrackLoader::Impl
    {
        void load_from_file(const std::string& file_name);
        void load_from_stream(std::istream& stream, std::string working_directory);

        void include(const std::string& file_name, std::size_t num_levels = 0);
        void include(std::istream& stream, std::size_t num_levels = 0);

        void process_tile_group_definition(std::istream& stream, TileId group_id, std::size_t group_size);
        void process_tile_definition(std::istream& stream, const std::string& pattern_name, const std::string& image_name);
        void process_terrain_definition(std::istream& stream);
        void process_control_points(std::istream& stream, std::size_t num_points);
        void process_start_points(std::istream& stream, std::size_t num_points);

        std::string resolve_asset_path(const std::string& file_name);
        void add_asset(std::string file_path);

        void place_tile(const LevelTile& tile);

        std::unordered_set<std::string> included_files_;
        std::vector<std::string> assets_;
        std::string working_directory_;

        Track track_;
        LayerHandle current_layer_;

        std::istringstream line_stream_;
        std::string directive_;
        std::string line_;
    };


    BrokenTrackException::BrokenTrackException(std::string missing_file)
        : std::runtime_error("broken track (missing file '" + missing_file + "')"),
        missing_file_(std::move(missing_file))
    {
    }

    std::string TrackLoader::Impl::resolve_asset_path(const std::string& file_name)
    {
        auto include_directory = find_include_directory(file_name, { working_directory_, config::data_directory });

        boost::filesystem::path path = include_directory;
        path /= file_name;

        return path.string();
    }

    void TrackLoader::Impl::add_asset(std::string file_path)
    {
        if (std::find(assets_.begin(), assets_.end(), file_path) == assets_.end())
        {
            assets_.push_back(std::move(file_path));
        }
    }

    void TrackLoader::Impl::load_from_file(const std::string& file_name)
    {
        track_ = Track();        

        auto track_name = boost::filesystem::basename(file_name);
        if (!track_name.empty())
        {
            track_name.front() = std::toupper(track_name.front(), std::locale());
        }

        track_.set_path(file_name);
        track_.set_name(track_name);

        working_directory_ = boost::filesystem::path(file_name).parent_path().string();

        include(file_name);
    }

    void TrackLoader::Impl::load_from_stream(std::istream& stream, std::string working_directory)
    {
        working_directory_ = std::move(working_directory);

        include(stream);
    }

    void TrackLoader::Impl::include(const std::string& file_name, std::size_t num_levels)
    {
        // Test if the file has not been included before
        if (included_files_.find(file_name) == included_files_.end())
        {
            auto include_path = resolve_asset_path(file_name);

            std::ifstream stream(include_path, std::istream::in);
            if (!stream)
            {
                throw BrokenTrackException(file_name);
            }

            add_asset(include_path);

            included_files_.insert(std::move(include_path));
            include(stream, num_levels);
        }
    }

    void TrackLoader::Impl::include(std::istream& stream, std::size_t num_levels)
    {
        std::istringstream line_stream;

        std::string params[2];
        for (std::string line, directive; directive != "end" && std::getline(stream, line);)
        {
            line_stream.clear();
            line_stream.str(line);

            core::read_directive(line_stream, directive);

            if (directive == "a")
            {
                Tile tile;
                if (line_stream >> tile)
                {
                    place_tile(tile);
                }
            }

            else if (directive == "tiledefinition" && line_stream >> params[0] >> params[1])
            {
                process_tile_definition(stream, params[0], params[1]);
            }

            else if (directive == "terrain")
            {
                process_terrain_definition(stream);
            }

            else if (directive == "subterrain")
            {
                SubTerrain sub_terrain;
                if (line_stream >> sub_terrain)
                {
                    track_.define_sub_terrain(sub_terrain);
                }
            }

            else if (directive == "tilegroup" || directive == "norottilegroup")
            {
                std::size_t group_size;
                TileId group_id;
                if (line_stream >> group_id >> group_size)
                {
                    process_tile_group_definition(stream, group_id, group_size);
                }
            }

            else if (directive == "leveltile")
            {
                LevelTile level_tile;

                if (line_stream >> level_tile)
                {
                    place_tile(level_tile);
                }
            }

            else if (directive == "layer")
            {
                std::size_t level;
                int visible;
                auto& layer_name = params[0];
                if (line_stream >> level >> visible >> std::ws && std::getline(line_stream, layer_name))
                {
                    current_layer_ = track_.create_layer(layer_name, level);
                    current_layer_->visible = (visible != 0);
                }
            }

            else if (directive == "include")
            {
                auto& include_path = params[0];

                if (std::getline(line_stream, include_path))
                {
                    boost::trim(include_path);        

                    include(include_path, num_levels + 1);

                    if (num_levels == 0)
                    {
                        track_.add_asset(include_path);                        
                    }
                }
            }

            else if (directive == "size")
            {
                Vector2u size;
                auto line_pos = line_stream.tellg();

                if (line_stream >> params[0] && params[0] == "td")
                {
                    std::size_t num_levels;
                    if (line_stream >> num_levels >> size.x >> size.y)
                    {
                        track_.set_size(size);
                        track_.set_num_levels(num_levels);
                    }
                }

                else
                {
                    line_stream.seekg(line_pos);
                    if (line_stream >> size.x >> size.y)
                    {
                        track_.set_size(size);
                        track_.set_num_levels(1);
                    }
                }
            }

            else if (directive == "controlpoints")
            {
                std::size_t num_points;
                if (line_stream >> num_points)
                {
                    process_control_points(stream, num_points);
                }
            }

            else if (directive == "startpoints")
            {
                std::size_t num_points;
                if (line_stream >> num_points)
                {
                    process_start_points(stream, num_points);
                }
            }

            else if (directive == "pattern")
            {
                auto& pattern_file = params[0];

                if (std::getline(line_stream, pattern_file))
                {
                    boost::trim(pattern_file);

                    auto pattern_path = resolve_asset_path(pattern_file);
                    add_asset(std::move(pattern_path));

                    track_.set_pattern(pattern_file);
                }
            }

            else if (directive == "maker")
            {
                auto& author = params[0];

                if (std::getline(line_stream, author))
                {
                    boost::trim(author);

                    track_.set_author(author);
                }
            }

            else if (directive == "pit")
            {
                core::IntRect pit;
                if (line_stream >> pit.left >> pit.top >> pit.width >> pit.height)
                {
                    track_.define_pit(pit);
                }
            }
        }
    }

    void TrackLoader::Impl::place_tile(const LevelTile& tile)
    {
        if (!current_layer_ || current_layer_->level != tile.level)
        {
            current_layer_ = track_.create_layer("Level " + std::to_string(tile.level), tile.level);            
        }

        current_layer_->tiles.push_back(tile);
    }

    void TrackLoader::Impl::process_tile_definition(std::istream& stream, const std::string& pattern_file, const std::string& image_file)
    {
        auto pattern_path = resolve_asset_path(pattern_file);
        auto image_path = resolve_asset_path(image_file);

        if (!pattern_path.empty() && !image_path.empty())
        {
            TileDefinition tile_def(pattern_path, image_path);

            add_asset(std::move(pattern_path));
            add_asset(std::move(image_path));

            for (directive_.clear(); directive_ != "end" && std::getline(stream, line_); )
            {
                line_stream_.clear();
                line_stream_.str(line_);

                core::read_directive(line_stream_, directive_);

                if ((directive_ == "tile" || directive_ == "norottile") && line_stream_ >> tile_def)
                {
                    track_.define_tile(tile_def);
                }

                else if (!line_stream_ && tile_def.id == 515)
                {
                    for (char ch : line_)
                        printf("%c ", ch);
                }
            }
        }
    }

    void TrackLoader::Impl::process_tile_group_definition(std::istream& stream, TileId group_id, std::size_t group_size)
    {
        TileGroupDefinition tile_group(group_id, group_size);
        for (directive_.clear(); directive_ != "end" && std::getline(stream, line_);)
        {
            line_stream_.clear();
            line_stream_.str(line_);

            core::read_directive(line_stream_, directive_);

            if (directive_ == "a")
            {
                Tile tile;
                if (line_stream_ >> tile)
                {
                    tile_group.add_sub_tile(tile);
                }
            }

            else if (directive_ == "leveltile")
            {
                LevelTile tile;
                if (line_stream_ >> tile)
                {
                    tile_group.add_sub_tile(tile);
                }
            }
        }

        track_.define_tile_group(tile_group);
    }

    void TrackLoader::Impl::process_terrain_definition(std::istream& stream)
    {
        TerrainDefinition terrain_def;
        if (stream >> terrain_def)
        {
            track_.define_terrain(terrain_def);
        }
    }

    void TrackLoader::Impl::process_control_points(std::istream& stream, std::size_t num_points)
    {
        for (directive_.clear(); directive_ != "end" && std::getline(stream, line_);)
        {
            line_stream_.clear();
            line_stream_.str(line_);

            core::read_directive(line_stream_, directive_);

            if (directive_ == "point")
            {
                Vector2i point;
                std::int32_t length;
                std::int32_t direction;
                if (line_stream_ >> point.x >> point.y >> length >> direction)
                {
                    ControlPoint control_point;
                    control_point.start = point;
                    control_point.length = length;
                    control_point.direction = direction != 0 ? ControlPoint::Horizontal : ControlPoint::Vertical;

                    track_.append_control_point(control_point);
                }
            }
        }
    }

    void TrackLoader::Impl::process_start_points(std::istream& stream, std::size_t num_points)
    {
        StartPoint start_point;

        for (directive_.clear(); directive_ != "end" && std::getline(stream, line_);)
        {
            line_stream_.clear();
            line_stream_.str(line_);

            core::read_directive(line_stream_, directive_);

            double degrees = 0.0;
            if (line_stream_ >> start_point.position.x >> start_point.position.y >> start_point.rotation)
            {
                track_.append_start_point(start_point);
            }
        }
    }

    TrackLoader::TrackLoader()
        : impl_(std::make_unique<Impl>())
    {
    }

    TrackLoader::~TrackLoader()
    {
    }

    void TrackLoader::load_from_file(const std::string& file_name)
    {
        impl_->load_from_file(file_name);
    }

    void TrackLoader::include(const std::string& file_name)
    {
        impl_->include(file_name);
    }

    void TrackLoader::load_from_stream(std::istream& stream, std::string working_directory)
    {
        impl_->load_from_stream(stream, std::move(working_directory));
    }

    Track TrackLoader::get_result()
    {
        return std::move(impl_->track_);
    }

    const std::vector<std::string>& TrackLoader::assets() const
    {
        return impl_->assets_;
    }
}
