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

#include "fill_dialog.hpp"
#include "fill_properties.hpp"

namespace interface
{
    FillDialog::FillDialog(QWidget* parent)
        : QDialog(parent)
    {
        ui_.setupUi(this);

        connect(ui_.densitySlider, SIGNAL(valueChanged(int)), ui_.densitySpinBox, SLOT(setValue(int)));
        connect(ui_.densitySpinBox, SIGNAL(valueChanged(int)), ui_.densitySlider, SLOT(setValue(int)));

        connect(ui_.posJitterSlider, SIGNAL(valueChanged(int)), ui_.posJitterSpinBox, SLOT(setValue(int)));
        connect(ui_.posJitterSpinBox, SIGNAL(valueChanged(int)), ui_.posJitterSlider, SLOT(setValue(int)));

        ui_.densitySlider->setValue(100);
        ui_.densitySpinBox->setValue(100);

        ui_.posJitterSlider->setValue(0);
        ui_.posJitterSpinBox->setValue(0);

        connect(ui_.buttonBox, SIGNAL(accepted()), this, SLOT(dispatch_fill_signal()));

        connect(ui_.randomizeRotationCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(randomize_rotation_state_change(int)));
    }

    void FillDialog::dispatch_fill_signal()
    {
        FillProperties properties;
        properties.area = FillProperties::WholeTrack;
        
        if (ui_.fillAreaComboBox->currentIndex() == 1)
        {
            properties.area = FillProperties::Selection;
        }

        properties.density = ui_.densitySpinBox->value();
        properties.position_jitter = ui_.posJitterSpinBox->value();
        properties.randomize_rotation = ui_.randomizeRotationCheckBox->isChecked();
        properties.rotation = ui_.rotationSpinBox->value();

        fill_area(properties);
    }

    void FillDialog::randomize_rotation_state_change(int state)
    {
        ui_.rotationSpinBox->setEnabled(state != Qt::Checked);
    }

    void FillDialog::set_fill_rotation(std::int32_t rotation)
    {
        ui_.rotationSpinBox->setValue(rotation);
    }

    void FillDialog::set_selection_fill_enabled(bool enable)
    {
        ui_.fillAreaComboBox->setItemData(1, enable ? Qt::ItemIsEnabled : 0, Qt::UserRole - 1);

        if (!enable)
        {
            ui_.fillAreaComboBox->setCurrentIndex(0);
        }
    }
}