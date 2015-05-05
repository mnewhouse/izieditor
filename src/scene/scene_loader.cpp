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

#include "scene_loader.hpp"
#include "scene.hpp"

#include "tile_partitioner.hpp"
#include "track_display.hpp"

#include "graphics/image_loader.hpp"

#include "components/track.hpp"
#include "components/tile_definition.hpp"

#include <unordered_set>

namespace scene
{
    void SceneLoader::async_load_scene(std::function<components::Track()> load_track)
    {
        auto loading_function = [=]()
        {
            return load_scene(load_track);
        };

        future_ = std::async(std::launch::async, loading_function);
    }

    std::unique_ptr<Scene> SceneLoader::load_scene(std::function<components::Track()> load_track)
    {
        loading_progress_ = 0.0;
        max_progress_ = 1.0;
        loading_state_ = LoadingState::Preprocessing;
        components::Track track = load_track();
        const auto& tile_library = track.tile_library();

        loading_state_ = LoadingState::LoadingImages;
        std::unordered_set<std::string> distinct_images;
        for (auto tile = tile_library.first_tile(); tile; tile = tile_library.next_tile(tile))
        {
            distinct_images.insert(tile->image_file());
        }

        loading_progress_ = 0.0;
        graphics::ImageLoader image_loader;
        std::size_t num_images = distinct_images.size(), images_loaded = 0;
        for (const auto& image : distinct_images)
        {
            image_loader.load_from_file(image);
            ++images_loaded;
            loading_progress_ = images_loaded / static_cast<double>(num_images);
        }        

        std::function<void(double)> update_progress = [=](double progress)
        {
            loading_progress_ = progress;
        };
        
        auto tile_mapping = create_tile_mapping(track.tile_library(), std::move(image_loader), update_progress);
        loading_state_ = LoadingState::MappingTiles;

        loading_progress_ = 0.0;
        loading_state_ = LoadingState::BuildingScene;
        auto track_display = create_track_layer_map(track, tile_mapping, update_progress);        

        return std::unique_ptr<Scene>(new Scene(std::move(track), std::move(tile_mapping), std::move(track_display)));
    }

    bool SceneLoader::is_finished() const
    {
        return future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    bool SceneLoader::is_loading() const
    {
        return future_.valid();
    }

    double SceneLoader::loading_progress() const
    {
        return loading_progress_;
    }

    double SceneLoader::max_progress() const
    {
        return max_progress_;
    }

    LoadingState SceneLoader::loading_state() const
    {
        return loading_state_;
    }

    std::unique_ptr<Scene> SceneLoader::get_result()
    {
        return future_.get();
    }
}