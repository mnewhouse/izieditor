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

#ifndef SCENE_LOADER_HPP
#define SCENE_LOADER_HPP

#include <memory>
#include <atomic>
#include <future>
#include <functional>

namespace components
{
    class Track;
}

namespace scene
{
    class Scene;

    enum class LoadingState
    {
        Preprocessing,
        LoadingImages,
        LoadingPattern,
        MappingTiles,
        BuildingScene
    };

    class SceneLoader
    {
    public:
        void async_load_scene(std::function<components::Track()> load_track);

        bool is_loading() const;
        bool is_finished() const;

        LoadingState loading_state() const;
        double loading_progress() const;
        double max_progress() const;

        std::unique_ptr<Scene> get_result();

    private:
        std::unique_ptr<Scene> load_scene(std::function<components::Track()> load_track);

        std::future<std::unique_ptr<Scene>> future_;

        std::atomic<LoadingState> loading_state_;
        std::atomic<double> loading_progress_;
        std::atomic<double> max_progress_;
    };
}

#endif