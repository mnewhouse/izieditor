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

#include "control_point.hpp"
#include "start_point.hpp"

#include "pattern_store.hpp"
#include "pattern_builder.hpp"

#include <boost/filesystem/path.hpp>

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

    void save_track_pattern(const Track& track, const PatternStore& pattern_store, const std::string& file_name)
    {
        PatternBuilder pattern_builder(track, pattern_store);

        auto pattern = pattern_builder();
        save_pattern(pattern, track.terrain_library(), file_name);
    }

    void save_track(const Track& track, const PatternStore& pattern_store, const std::string& file_name)
    {
        std::ofstream out(file_name);

        if (!out)
        {
            throw SaveError(file_name);
        }

        out << "# This is a Turbo Sliders track file\n";
        out << "# Do not change the order of the following lines!\n";
        out << "# This track was saved with IziEditor.\n";

        auto track_size = track.size();
        out << "Size td " << track.num_levels() << " " << track_size.x << " " << track_size.y << "\n";
        out << "Hash 0 0 0 0\n"; // TODO
        out << "Maker " << (track.author().empty() ? "Anonymous" : track.author()) << "\n";
        out << "FormatVersion 2\n";

        auto pattern = track.pattern();
        if (pattern.empty())
        {
            pattern = track.name() + "-pat.png";
        }
        out << "Pattern " << pattern << "\n";

        for (const auto& asset : track.assets())
        {
            out << "Include " << asset << "\n";
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
                auto degrees = static_cast<std::int32_t>(point.rotation.degrees(core::rotation::absolute));
                out << "  Point " << point.position.x << " " << point.position.y << " " << degrees << "\n";
            }

            out << "End\n";
        }

        for (const auto& layer : track.layers())
        {
            out << "Layer " << layer->level << " " << static_cast<int>(layer->visible) << " " << layer->name << "\n";
            for (const auto& tile : layer->tiles)
            {
                int x = static_cast<int>(tile.position.x);
                int y = static_cast<int>(tile.position.y);
                int rotation = static_cast<int>(tile.rotation.degrees(core::rotation::absolute));

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
        
        boost::filesystem::path pattern_path = track.path();
        pattern_path = pattern_path.parent_path() / pattern;
        save_track_pattern(track, pattern_store, pattern_path.string());
    }
}