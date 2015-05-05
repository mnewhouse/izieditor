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

#include "loading_dialog.hpp"
#include "track_essentials.hpp"

#include "components/track.hpp"
#include "components/track_loader.hpp"
#include "components/tile_library.hpp"

#include "scene/scene.hpp"
#include "scene/track_display.hpp"
#include "scene/tile_partitioner.hpp"

#include <qmessagebox.h>

#include <iostream>



namespace interface
{
    static QString to_string(scene::LoadingState loading_state)
    {
        using scene::LoadingState;

        switch (loading_state)
        {
        case LoadingState::Preprocessing:
            return "Preprocessing...";

        case LoadingState::BuildingScene:
            return "Building Scene...";
            
        case LoadingState::MappingTiles:
            return "Mapping Tiles...";
            
        case LoadingState::LoadingImages:
            return "Loading Images...";

        case LoadingState::LoadingPattern:
            return "Loading Patterns...";

        default:
            return "";
        }
    }

    LoadingDialog::LoadingDialog(QWidget* parent)
        : QDialog(parent)
    {
        ui_.setupUi(this);

        auto flags = windowFlags();
        flags &= ~Qt::WindowCloseButtonHint;
        flags &= ~Qt::WindowMinimizeButtonHint;
        flags &= ~Qt::WindowContextHelpButtonHint;

        setWindowFlags(flags);

        connect(&timer_, SIGNAL(timeout()), this, SLOT(poll()));
    }

    void LoadingDialog::poll()
    {
        auto progress = static_cast<int>(scene_loader_.loading_progress() / scene_loader_.max_progress() * 100.0);
        ui_.loadingProgress->setValue(progress);

        auto loading_state = scene_loader_.loading_state();
        ui_.loadingState->setText(to_string(loading_state));

        if (scene_loader_.is_finished())
        {
            timer_.stop();
            hide();

            try
            {
                auto scene = scene_loader_.get_result();
                scene_ready(scene);
            }

            catch (const std::exception& e)
            {
                QMessageBox::critical(this, "Loading Error", "Error loading track: " + QString(e.what()));
            }
        }
    }

    void LoadingDialog::load_track(const QString& track_path)
    {
        auto loading_func = [=]()
        {
            components::TrackLoader track_loader;
            track_loader.load_from_file(track_path.toStdString());
            return track_loader.get_result();
        };

        scene_loader_.async_load_scene(loading_func);
        timer_.start(10);

        show();
    }

    void LoadingDialog::create_track(const TrackEssentials& essentials)    
    {
        auto loading_func = [=]()
        {
            components::TrackLoader track_loader;

            for (const QString& asset : essentials.assets)
            {
                track_loader.include(asset.toStdString());
            }

            auto track = track_loader.get_result();
            track.set_path(essentials.path.toStdString());
            track.set_name(essentials.name.toStdString());
            track.set_author(essentials.author.toStdString());
            track.set_size(core::Vector2u(essentials.size.width(), essentials.size.height()));
            track.set_num_levels(6);
            track.create_layer("Layer 1", 0);
            
            return track;
        };

        scene_loader_.async_load_scene(loading_func);

        timer_.start(10);
        show();
    }
}