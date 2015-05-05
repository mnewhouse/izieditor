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

#ifndef LOADING_DIALOG_HPP
#define LOADING_DIALOG_HPP

#include "scene/scene_loader.hpp"

#include "ui_loading_dialog.h"

#include <qdialog.h>
#include <qtimer.h>

#include <memory>

namespace scene
{
    class Scene;
}

namespace interface
{
    struct TrackEssentials;

    class LoadingDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        LoadingDialog(QWidget* parent);

    public slots:
        void create_track(const TrackEssentials& essentials);
        void load_track(const QString& file_path);

        void poll();

    signals:
        void scene_ready(std::unique_ptr<scene::Scene>&);

    private:
        Ui::LoadingDialog ui_;

        scene::SceneLoader scene_loader_;
        QTimer timer_;
    };
};

#endif