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

#ifndef TRACK_PROPERTIES_DIALOG_HPP
#define TRACK_PROPERTIES_DIALOG_HPP

#include "ui_track_properties_dialog.h"

namespace components
{
    class Track;
}

namespace interface
{
    struct TrackProperties;

    class TrackPropertiesDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        TrackPropertiesDialog(QWidget* parent);

        void show_dialog(const components::Track& track);

    signals:
        void commit_changes(const TrackProperties& track_properties);

    private slots:
        void dispatch_commit_signal();
        void start_direction_override_checkbox_state_changed(int state);

        void gravity_direction_dial_value_changed(int value);
        void gravity_direction_spinbox_value_changed(int value);

        void start_direction_dial_value_changed(int value);
        void start_direction_spinbox_value_changed(int value);

    private:
        Ui::TrackPropertiesDialog ui_;
    };
}

#endif