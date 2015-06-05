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

#include "track_saving.hpp"
#include "track.hpp"

#include "terrain_library.hpp"

#include "control_point.hpp"
#include "start_point.hpp"

#include "pattern_store.hpp"
#include "pattern_builder.hpp"

#include "track_hash.hpp"

#include <boost/filesystem.hpp>

#include <fstream>

namespace components
{
    SaveError::SaveError(const std::string& file_name)
        : std::runtime_error("could not open " + file_name + " for writing")
    {
    }

    void save_track(const Track& track, const PatternStore& pattern_store)
    {
        save_track(track, pattern_store, track.path());
    }

    void save_tile_definitions(std::ostream& stream, std::vector<TileDefinition> tile_definitions)
    {
        std::sort(tile_definitions.begin(), tile_definitions.end(), 
            [](const TileDefinition& a, const TileDefinition& b)
        {
            using helper_pair = std::pair<const std::string&, const std::string&>;
            return helper_pair(a.pattern_file, a.image_file) < helper_pair(b.pattern_file, b.image_file);
        });

        for (auto it = tile_definitions.begin(); it != tile_definitions.end();)
        {
            const std::string& pattern = it->pattern_file;
            const std::string& image = it->image_file;

            auto last = std::find_if_not(std::next(it), tile_definitions.end(), 
                [&](const TileDefinition& tile_def)
            {
                return tile_def.pattern_file == pattern && tile_def.image_file == image;
            });

            stream << "TileDefinition " << pattern << " " << image << "\n";

            for (; it != last; ++it)
            {
                const TileDefinition& tile_def = *it;
                core::IntRect pat_rect = tile_def.pattern_rect;
                core::IntRect img_rect = tile_def.image_rect;

                stream << "  Tile " << tile_def.id << " " <<
                    pat_rect.left << " " << pat_rect.top << " " << pat_rect.width << " " << pat_rect.height << " " <<
                    img_rect.left << " " << img_rect.top << " " << img_rect.width << " " << img_rect.height << "\n";
            }
            
            stream << "End\n";
        }
    }

    void save_tile_group_definitions(std::ostream& stream, const std::vector<TileGroupDefinition>& tile_group_definitions)
    {
        for (const auto& tile_group : tile_group_definitions)
        {
            const auto& sub_tiles = tile_group.sub_tiles();
            stream << "TileGroup " << tile_group.id() << " " << sub_tiles.size() << "\n";

            for (const auto& sub_tile : sub_tiles)
            {
                core::Vector2i pos = sub_tile.position;

                double degrees = sub_tile.rotation.normalize().degrees(core::rotation::absolute);
                std::int32_t rotation = static_cast<std::int32_t>(std::round(degrees));
                
                if (sub_tile.level == 0)
                {
                    stream << "  A " << sub_tile.id << " " << pos.x << " " << pos.y << " " << rotation << "\n";
                }

                else
                {
                    stream << "  LevelTile " << sub_tile.level << " " << sub_tile.id << " " <<
                        pos.x << " " << pos.y << " " << rotation << "\n";
                }
            }

            stream << "End\n";
        }
    }

    void save_terrain_definitions(std::ostream& stream, const std::vector<TerrainDefinition>& terrain_definitions)
    {
        TerrainDefinition default_terrain;
        
        for (const auto& terrain : terrain_definitions)
        {
            stream << "Terrain " << terrain.name << "\n";
            stream << "  id " << +terrain.id << "\n";
            stream << "  isWall " << +terrain.is_wall << "\n";

            if (terrain.is_wall)
            {
                stream << "  bounciness " << terrain.bounciness << "\n";

                if (terrain.viscosity != default_terrain.viscosity) 
                    stream << "  viscosity " << terrain.viscosity << "\n";

                if (terrain.acceleration != default_terrain.acceleration)
                    stream << "  acceleration " << terrain.acceleration << "\n";

                if (terrain.braking != default_terrain.braking)
                    stream << "  braking " << terrain.braking << "\n";

                if (terrain.grip != default_terrain.grip)
                    stream << "  grip " << terrain.grip << "\n";

                if (terrain.steering != default_terrain.steering)
                {
                    stream << "  steering " << terrain.steering << "\n";
                }
            }

            else
            {
                if (terrain.bounciness != default_terrain.bounciness)
                    stream << "  bounciness " << terrain.bounciness << "\n";

                stream << "  viscosity " << terrain.viscosity << "\n";
                stream << "  acceleration " << terrain.acceleration << "\n";
                stream << "  braking " << terrain.braking << "\n";
                stream << "  grip " << terrain.grip << "\n";
                stream << "  steering " << terrain.steering << "\n";
            }

            if (terrain.slowing != default_terrain.slowing)
                stream << "  slowing " << terrain.slowing << "\n";

            if (terrain.jump != default_terrain.jump)
                stream << "  jump " << terrain.jump << "\n";

            if (terrain.maxjumpspeed != default_terrain.maxjumpspeed)
                stream << "  maxjumpspeed " << terrain.maxjumpspeed << "\n";

            if (terrain.energyloss != default_terrain.energyloss)
                stream << "  energyloss " << terrain.energyloss << "\n";

            if (terrain.gravity != default_terrain.gravity)
            {
                stream << "  gravity " << terrain.gravity << "\n";
                stream << "  gravitydirection " << terrain.gravitydirection << "\n";
            }

            if (terrain.size != default_terrain.size)
                stream << "  size " << terrain.size << "\n";

            if (!terrain.is_wall)
            {
                stream << "  skidMark " << +terrain.skid_mark << "\n";
                stream << "  tyreMark " << +terrain.tyre_mark << "\n";
            }            

            stream << "  red " << +terrain.red << "\n";
            stream << "  green " << +terrain.green << "\n";
            stream << "  blue " << +terrain.blue << "\n";

            stream << "End";
        }
    }
    
    void save_sub_terrain_definitions(std::ostream& stream, const std::vector<SubTerrain>& sub_terrains)
    {
        for (const auto& sub_terrain : sub_terrains)
        {
            stream << "SubTerrain " << +sub_terrain.terrain_id << " " << +sub_terrain.component_id << " " <<
                sub_terrain.level_start << " " << sub_terrain.level_count << "\n";
        }
    }

    void save_kill_terrains(std::ostream& stream, const std::vector<TerrainId>& kill_terrains)
    {
        for (TerrainId terrain : kill_terrains)
        {
            stream << "KillTerrain" << +terrain << "\n";
        }
    }

    void save_track(const Track& track, const PatternStore& pattern_store, const std::string& file_name)
    {
        namespace bfs = boost::filesystem;
        bfs::path path = bfs::path(file_name).parent_path();
        bfs::create_directories(path);

        std::ofstream out(file_name);

        if (!out)
        {
            throw SaveError(file_name);
        }


        PatternBuilder pattern_builder(track, pattern_store);
        auto pattern = pattern_builder();

        out << "# This is a Turbo Sliders track file\n";
        out << "# Do not change the order of the following lines!\n";
        out << "# This track was saved with IziEditor.\n";

        auto track_size = track.size();
        out << "Size td " << track.num_levels() << " " << track_size.x << " " << track_size.y << "\n";

        auto track_hash = calculate_track_hash(track, pattern);
        out << "Hash " << std::hex << track_hash[0] << " " << track_hash[1] << " " << 
            track_hash[2] << " " << track_hash[3] << std::dec << "\n";

        out << "Maker " << (track.author().empty() ? "Anonymous" : track.author()) << "\n";
        out << "FormatVersion 2\n";

        auto pattern_file = track.pattern();
        if (pattern_file.empty())
        {
            pattern_file = track.name() + "-pat.png";
        }
        out << "Pattern " << pattern_file << "\n";

        auto track_type = track.track_type();
        if (track_type == TrackType::PunaBall) out << "PunaBallTrack\n";
        else if (track_type == TrackType::Battle) out << "BattleTrack\n";
        else if (track_type == TrackType::XBumpz) out << "BattleTrack Bumpz\n";
        else if (track_type == TrackType::SingleLap) out << "SingleLapTrack\n";

        for (const auto& asset : track.assets())
        {
            out << "Include " << asset << "\n";
        }

        save_tile_definitions(out, track.contained_tile_definitions());
        save_tile_group_definitions(out, track.contained_tile_group_definitions());
        save_terrain_definitions(out, track.contained_terrain_definitions());
        save_sub_terrain_definitions(out, track.contained_sub_terrain_definitions());
        save_kill_terrains(out, track.contained_kill_terrains());

        std::int32_t gravity = track.gravity_strength();
        if (gravity > 0)
        {
            out << "Gravity " << gravity << "\n";
            out << "GravityDirection " << track.gravity_direction() << "\n";
        }

        const auto& control_points = track.control_points();
        out << "ControlPoints " << control_points.size() << "\n";

        for (const auto& point : control_points)
        {
            int direction = point.direction == ControlPoint::Vertical ? 0 : 1;

            out << "  Point " << point.start.x << " " << point.start.y << " " << point.length << " " << direction << "\n";
        }
        out << "End\n";

        if (auto pit = track.pit())
        {
            out << "Pit " << pit->left << " " << pit->top << " " << pit->width << " " << pit->height << "\n";
        }

        const auto& start_points = track.start_points();
        if (!start_points.empty())
        {
            out << "StartPoints " << start_points.size() << "\n";

            for (const auto& point : start_points)
            {
                out << "  Point " << point.position.x << " " << point.position.y << " " << 
                    point.rotation << " " << point.level << "\n";
            }

            out << "End\n";
        }

        for (const auto& layer : track.layers())
        {
            out << "Layer " << layer->level << " " << static_cast<int>(layer->visible) << " " << layer->name << "\n";
            for (const auto& tile : layer->tiles)
            {
                std::int32_t x = tile.position.x;
                std::int32_t y = tile.position.y;

                double degrees = tile.rotation.normalize().degrees(core::rotation::absolute);
                std::int32_t rotation = static_cast<std::int32_t>(std::round(degrees));

                if (layer->level == 0)
                {
                    out << "A " << tile.id << " " << x << " " << y << " " << rotation << "\n";
                }

                else
                {
                    out << "LevelTile " << layer->level << " " << tile.id << " " << x << " " << y << " " << rotation << "\n";
                }
            }
        }

        out << "End\n";
        
        bfs::path pattern_path = path / pattern_file;
        save_pattern(pattern, track.terrain_library(), pattern_path.string());
    }
}