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

#ifndef LAYER_PROPERTIES_HPP
#define LAYER_PROPERTIES_HPP

#include "components/track_layer.hpp"

#include "ui_layer_properties.h"

#include <qdialog.h>

namespace interface
{
    class LayerPropertiesDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        LayerPropertiesDialog(QWidget* parent = nullptr);

    signals:
        void rename_layer(std::size_t, const std::string&);
        void set_layer_level(std::size_t, std::size_t);
        void hide_layer(std::size_t);
        void show_layer(std::size_t);

    public slots:
        void display_layer_properties(components::ConstLayerHandle layer, std::size_t num_levels);
        void dispatch_signals();

    private:
        Ui::layerProperties ui_;
        components::ConstLayerHandle layer_;
    };
};

#endif