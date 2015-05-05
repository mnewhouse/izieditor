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

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel.h>

#include "ui_main_window.h"

namespace scene
{
    class Scene;
}

namespace interface
{
    class NewTrackDialog;
    class LoadingDialog;
    class LayerPropertiesDialog;
    class FillDialog;
    class ResizeTrackDialog;

    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        MainWindow(QWidget *parent = nullptr);
        virtual ~MainWindow();       

    public slots:
        void open_track();
        void save_track();

        void scene_loaded(const scene::Scene* scene_ptr);

        void resize_track();

        void mouse_move(const QPoint& point);

        void tool_changed(EditorTool tool);
        void mode_changed(EditorMode mode);

        void tool_enabled(EditorTool tool);
        void tool_disabled(EditorTool tool);

        void placement_tile_changed(const components::TileGroupDefinition* tile_group);
        void placement_tile_rotated(std::int32_t rotation);

        void selection_area_changed(core::IntRect area);

        void tile_selection_changed(std::size_t selected_tile_count);
        void tile_selection_hover_changed(const components::Tile* tile);

        void tiles_moved(core::Vector2i offset);
        void tiles_rotated(core::Rotation<double> rotation);

        void tiles_movement_finished();
        void tiles_rotation_finished();

        void show_fill_dialog();

        void layer_selected(std::size_t layer_id);
        void layer_deselected();
        
        void show_layer_properties();

        void update_layer_selection();
        void delete_layer_confirmation();

        void enable_undo();
        void disable_undo();

        void enable_redo();
        void disable_redo();

        void enable_pasting();
        void disable_pasting();

        void toggle_layer_view();
        void toggle_history_view();

    private:
        void set_tool_enabled(EditorTool tool, bool enabled);

        QAction* tool_action(EditorTool tool) const;
        QAction* mode_action(EditorMode mode) const;

        Ui::MainWindowClass ui_;

        NewTrackDialog* new_dialog_ = nullptr;
        LoadingDialog* loading_dialog_ = nullptr;
        LayerPropertiesDialog* layer_properties_dialog_ = nullptr;
        FillDialog* fill_dialog_ = nullptr;
        ResizeTrackDialog* resize_track_dialog_ = nullptr;

        QLabel* tool_info_label_ = nullptr;
        QLabel* secondary_tool_info_label_ = nullptr;
        QLabel* position_label_ = nullptr;
    };
}

#endif // MAINWINDOW_H
