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

#include "layer_properties.hpp"

namespace interface
{
    LayerPropertiesDialog::LayerPropertiesDialog(QWidget* parent)
        : QDialog(parent)
    {
        ui_.setupUi(this);

        ui_.visibility_checkBox->setStyleSheet("alignment: right;");

        connect(ui_.buttonBox, SIGNAL(accepted()), this, SLOT(dispatch_signals()));
    }

    void LayerPropertiesDialog::display_layer_properties(components::ConstLayerHandle layer, std::size_t num_levels)
    {
        layer_ = layer;

        std::size_t max_level = (num_levels != 0 ? num_levels - 1 : 0);
        
        ui_.level_spinBox->setValue(static_cast<int>(layer->level));
        ui_.level_spinBox->setRange(0, static_cast<int>(max_level));

        ui_.layerName_edit->setText(QString::fromStdString(layer->name));
        ui_.visibility_checkBox->setChecked(layer->visible);

        show();
    }

    void LayerPropertiesDialog::dispatch_signals()
    {
        std::string new_name = ui_.layerName_edit->text().toStdString();
        std::size_t new_level = ui_.level_spinBox->value();
        bool new_visibility = ui_.visibility_checkBox->isChecked();

        std::size_t layer_id = layer_.id();

        if (new_name != layer_->name)
        {
            rename_layer(layer_id, new_name);
        }

        if (new_level != layer_->level)
        {
            set_layer_level(layer_id, new_level);
        }

        if (new_visibility != layer_->visible)
        {
            if (new_visibility) show_layer(layer_id);
            else hide_layer(layer_id);
        }
    }
}