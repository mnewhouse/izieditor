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

#include "track_properties_dialog.hpp"
#include "track_properties.hpp"

#include "components/track.hpp"

namespace interface
{
    TrackPropertiesDialog::TrackPropertiesDialog(QWidget* parent)
        : QDialog(parent)
    {
        ui_.setupUi(this);

        connect(ui_.buttonBox, SIGNAL(accepted()), this, SLOT(dispatch_commit_signal()));
        connect(ui_.startDirectionOverride_checkBox, SIGNAL(stateChanged(int)),
            this, SLOT(start_direction_override_checkbox_state_changed(int)));

        connect(ui_.startDirection_dial, SIGNAL(valueChanged(int)),
            this, SLOT(start_direction_dial_value_changed(int)));

        connect(ui_.startDirection_spinBox, SIGNAL(valueChanged(int)),
            this, SLOT(start_direction_spinbox_value_changed(int)));

        connect(ui_.gravity_slider, SIGNAL(valueChanged(int)), ui_.gravity_spinBox, SLOT(setValue(int)));
        connect(ui_.gravity_spinBox, SIGNAL(valueChanged(int)), ui_.gravity_slider, SLOT(setValue(int)));

        connect(ui_.gravityDirection_dial, SIGNAL(valueChanged(int)),
            this, SLOT(gravity_direction_dial_value_changed(int)));

        connect(ui_.gravityDirection_spinBox, SIGNAL(valueChanged(int)),
            this, SLOT(gravity_direction_spinbox_value_changed(int)));
    }

    void TrackPropertiesDialog::dispatch_commit_signal()
    {
        TrackProperties track_properties{};

        using components::TrackType;
        track_properties.track_type = [](std::size_t index)
        {
            const components::TrackType types[] =
            {
                TrackType::Racing,
                TrackType::PunaBall,
                TrackType::Battle,
                TrackType::XBumpz,
                TrackType::SingleLap
            };

            return types[index];
        }(ui_.trackType_comboBox->currentIndex());

        track_properties.author = ui_.trackAuthor_editBox->text().toStdString();
        track_properties.height_levels = ui_.heightLevels_spinBox->value();
        track_properties.override_start_direction = ui_.startDirectionOverride_checkBox->isChecked();
        track_properties.start_direction = ui_.startDirection_spinBox->value();

        track_properties.gravity_strength = ui_.gravity_spinBox->value();
        track_properties.gravity_direction = ui_.gravityDirection_spinBox->value();

        commit_changes(track_properties);
    }
    
    void TrackPropertiesDialog::start_direction_override_checkbox_state_changed(int checkState)
    {
        bool checked = (checkState == Qt::Checked);
        ui_.startDirection_dial->setEnabled(checked);
        ui_.startDirection_spinBox->setEnabled(checked);
    }

    void TrackPropertiesDialog::start_direction_dial_value_changed(int value)
    {
        ui_.startDirection_spinBox->setValue((value + 90) % 360);
    }

    void TrackPropertiesDialog::start_direction_spinbox_value_changed(int value)
    {
        ui_.startDirection_dial->setValue((value + 270) % 360);
    }

    void TrackPropertiesDialog::gravity_direction_dial_value_changed(int value)
    {
        ui_.gravityDirection_spinBox->setValue((value + 90) % 360);
    }

    void TrackPropertiesDialog::gravity_direction_spinbox_value_changed(int value)
    {
        ui_.gravityDirection_dial->setValue((value + 270) % 360);
    }

    void TrackPropertiesDialog::show_dialog(const components::Track& track)
    {
        ui_.trackName_label->setText(QString::fromStdString(track.name()));
        ui_.trackAuthor_editBox->setText(QString::fromStdString(track.author()));

        QString size_text = QString::number(track.size().x) + " x " + QString::number(track.size().y);
        ui_.trackSize_label->setText(size_text);

        ui_.heightLevels_spinBox->setValue(static_cast<int>(track.num_levels()));

        using components::TrackType;

        int track_type = [](TrackType track_type)
        {
            switch (track_type)
            {
            case TrackType::PunaBall: return 1;
            case TrackType::Battle: return 2;
            case TrackType::XBumpz: return 3;
            case TrackType::SingleLap: return 4;
            default: return 0;
            }
        }(track.track_type());

        ui_.trackType_comboBox->setCurrentIndex(track_type);

        bool override_start_direction = track.is_start_direction_overridden();
        ui_.startDirectionOverride_checkBox->setChecked(override_start_direction);
        ui_.startDirection_dial->setEnabled(override_start_direction);
        ui_.startDirection_spinBox->setEnabled(override_start_direction);

        ui_.startDirection_dial->setValue((track.start_direction() + 270) % 360);
        ui_.startDirection_spinBox->setValue(track.start_direction());

        ui_.gravity_slider->setValue(track.gravity_strength());
        ui_.gravity_spinBox->setValue(track.gravity_strength());

        ui_.gravityDirection_dial->setValue((track.gravity_direction() + 270) % 360);
        ui_.gravityDirection_spinBox->setValue(track.gravity_direction());        

        show();
    }

}