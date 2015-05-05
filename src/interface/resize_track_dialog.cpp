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

#include "resize_track_dialog.hpp"

namespace interface
{
    ResizeTrackDialog::ResizeTrackDialog(QWidget* parent)
        : QDialog(parent)
    {
        ui_.setupUi(this);

        connect(ui_.widthSlider, SIGNAL(valueChanged(int)), ui_.widthSpinBox, SLOT(setValue(int)));
        connect(ui_.heightSlider, SIGNAL(valueChanged(int)), ui_.heightSpinBox, SLOT(setValue(int)));

        connect(ui_.widthSpinBox, SIGNAL(valueChanged(int)), ui_.widthSlider, SLOT(setValue(int)));
        connect(ui_.heightSpinBox, SIGNAL(valueChanged(int)), ui_.heightSlider, SLOT(setValue(int)));

        connect(ui_.buttonBox, SIGNAL(accepted()), this, SLOT(dispatch_resize_signal()));
    }

    void ResizeTrackDialog::show(std::int32_t width, std::int32_t height)
    {
        ui_.widthSlider->setValue(width);
        ui_.heightSlider->setValue(height);

        ui_.widthSpinBox->setValue(width);
        ui_.heightSpinBox->setValue(height);

        ui_.horizontalAnchorComboBox->setCurrentIndex(0);
        ui_.verticalAnchorComboBox->setCurrentIndex(0);

        QDialog::show();
    }

    void ResizeTrackDialog::dispatch_resize_signal()
    {
        std::size_t width = ui_.widthSpinBox->value();
        std::size_t height = ui_.heightSpinBox->value();

        HorizontalAnchor horizontal_anchor = [](int value)
        {
            switch (value)
            {
            case 1: return HorizontalAnchor::Center;
            case 2: return HorizontalAnchor::Right;

            default:return HorizontalAnchor::Left;
            }
        }(ui_.horizontalAnchorComboBox->currentIndex());

        VerticalAnchor vertical_anchor = [](int value)
        {
            switch (value)
            {
            case 1: return VerticalAnchor::Center;
            case 2: return VerticalAnchor::Bottom;
            default: return VerticalAnchor::Top;                
            }
        }(ui_.verticalAnchorComboBox->currentIndex());

        resize_track(width, height, horizontal_anchor, vertical_anchor);
    }
};