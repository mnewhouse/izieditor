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

#include "main_window.hpp"
#include "new_track_dialog.hpp"
#include "loading_dialog.hpp"
#include "layer_properties.hpp"
#include "fill_dialog.hpp"
#include "resize_track_dialog.hpp"

#include "components/track_saving.hpp"

#include <qfiledialog.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qevent.h>

#include <memory>

namespace interface
{
    MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent)
    {
        ui_.setupUi(this);

        ui_.newLayerButton->setDefaultAction(ui_.actionNew_Layer);
        ui_.deleteLayerButton->setDefaultAction(ui_.actionDelete_Layer);
        ui_.layerPropertiesButton->setDefaultAction(ui_.actionLayer_Properties);
        ui_.mergeLayersButton->setDefaultAction(ui_.actionMergeLayerWithPrevious);

        ui_.history_undoButton->setDefaultAction(ui_.actionUndo);
        ui_.history_redoButton->setDefaultAction(ui_.actionRedo);

        position_label_ = new QLabel(ui_.statusBar);
        position_label_->setMinimumWidth(100);
        position_label_->setAlignment(Qt::AlignHCenter);

        tool_info_label_ = new QLabel(ui_.statusBar);
        tool_info_label_->setMinimumWidth(50);
        secondary_tool_info_label_ = new QLabel(ui_.statusBar);
        secondary_tool_info_label_->setMinimumWidth(50);

        ui_.statusBar->addPermanentWidget(position_label_);     
        ui_.statusBar->addWidget(tool_info_label_);
        ui_.statusBar->addWidget(secondary_tool_info_label_);

        new_dialog_ = new NewTrackDialog(this);
        loading_dialog_ = new LoadingDialog(this);
        layer_properties_dialog_ = new LayerPropertiesDialog(this);
        fill_dialog_ = new FillDialog(this);
        resize_track_dialog_ = new ResizeTrackDialog(this);

        connect(ui_.actionNew, SIGNAL(triggered()), new_dialog_, SLOT(show()));
        connect(ui_.actionOpen, SIGNAL(triggered()), this, SLOT(open_track()));
        connect(ui_.actionSave, SIGNAL(triggered()), this, SLOT(save_track()));
        
        connect(ui_.actionHistoryView, SIGNAL(triggered()), this, SLOT(toggle_history_view()));
        connect(ui_.actionLayerView, SIGNAL(triggered()), this, SLOT(toggle_layer_view()));

        connect(ui_.historyWindow, SIGNAL(visibilityChanged(bool)),
            ui_.actionHistoryView, SLOT(setChecked(bool)));

        connect(ui_.layerWindow, SIGNAL(visibilityChanged(bool)),
            ui_.actionLayerView, SLOT(setChecked(bool)));

        connect(new_dialog_, SIGNAL(create_track(const TrackEssentials&)),
            loading_dialog_, SLOT(create_track(const TrackEssentials&)));

        connect(loading_dialog_, SIGNAL(scene_ready(std::unique_ptr<scene::Scene>&)),
            ui_.editorCanvas, SLOT(adopt_scene(std::unique_ptr<scene::Scene>&)));

        connect(ui_.actionZoom_In, SIGNAL(triggered()), ui_.editorCanvas, SLOT(zoom_in()));
        connect(ui_.actionZoom_Out, SIGNAL(triggered()), ui_.editorCanvas, SLOT(zoom_out()));
        connect(ui_.actionZoom_to_fit, SIGNAL(triggered()), ui_.editorCanvas, SLOT(zoom_to_fit()));

        connect(ui_.actionResize, SIGNAL(triggered()), this, SLOT(resize_track()));
        connect(resize_track_dialog_, SIGNAL(resize_track(std::int32_t, std::int32_t, HorizontalAnchor, VerticalAnchor)),
            ui_.editorCanvas, SLOT(resize_track(std::int32_t, std::int32_t, HorizontalAnchor, VerticalAnchor)));

        connect(ui_.actionDeleteLast, SIGNAL(triggered()), ui_.editorCanvas, SLOT(delete_last()));
        connect(ui_.actionDelete, SIGNAL(triggered()), ui_.editorCanvas, SLOT(delete_selection()));

        connect(ui_.actionCopy, SIGNAL(triggered()), ui_.editorCanvas, SLOT(copy_selection()));
        connect(ui_.actionCut, SIGNAL(triggered()), ui_.editorCanvas, SLOT(cut_selection()));
        connect(ui_.actionPaste, SIGNAL(triggered()), ui_.editorCanvas, SLOT(paste_clipboard()));

        connect(ui_.editorCanvas, SIGNAL(clipboard_filled()), this, SLOT(enable_pasting()));
        connect(ui_.editorCanvas, SIGNAL(clipboard_emptied()), this, SLOT(disable_pasting()));

        connect(ui_.editorCanvas, SIGNAL(mouse_move(const QPoint&)),
            this, SLOT(mouse_move(const QPoint&)));
        
        connect(ui_.editorCanvas, SIGNAL(scene_loaded(const scene::Scene*)), this, SLOT(scene_loaded(const scene::Scene*)));

        connect(ui_.editorCanvas, SIGNAL(scene_loaded(const scene::Scene*)), ui_.actionHistoryList, SLOT(clear()));

        connect(ui_.actionPlacement, SIGNAL(triggered()), ui_.editorCanvas, SLOT(activate_placement_tool()));
        connect(ui_.actionSingleSelectionTool, SIGNAL(triggered()), ui_.editorCanvas, SLOT(activate_tile_selection_tool()));
        connect(ui_.actionAreaSelectionTool, SIGNAL(triggered()), ui_.editorCanvas, SLOT(activate_area_selection_tool()));

        connect(ui_.actionMoveTool, SIGNAL(triggered()), ui_.editorCanvas, SLOT(activate_move_tool()));
        connect(ui_.actionRotation_Tool, SIGNAL(triggered()), ui_.editorCanvas, SLOT(activate_rotation_tool()));

        connect(ui_.actionTiles, SIGNAL(triggered()), ui_.editorCanvas, SLOT(activate_tiles_mode()));
        connect(ui_.actionControl_Points, SIGNAL(triggered()), ui_.editorCanvas, SLOT(activate_control_points_mode()));
        connect(ui_.actionStart_Points, SIGNAL(triggered()), ui_.editorCanvas, SLOT(activate_start_points_mode()));
        connect(ui_.actionPattern, SIGNAL(triggered()), ui_.editorCanvas, SLOT(activate_pattern_mode()));
        connect(ui_.actionPit, SIGNAL(triggered()), ui_.editorCanvas, SLOT(activate_pit_mode()));

        connect(ui_.editorCanvas, SIGNAL(tool_changed(EditorTool)), this, SLOT(tool_changed(EditorTool)));
        connect(ui_.editorCanvas, SIGNAL(mode_changed(EditorMode)), this, SLOT(mode_changed(EditorMode)));

        connect(ui_.editorCanvas, SIGNAL(tool_enabled(EditorTool)), this, SLOT(tool_enabled(EditorTool)));
        connect(ui_.editorCanvas, SIGNAL(tool_disabled(EditorTool)), this, SLOT(tool_disabled(EditorTool)));

        connect(ui_.editorCanvas, SIGNAL(clear_tool_info()), this, SLOT(clear_tool_info()));

        connect(ui_.editorCanvas, SIGNAL(selection_area_changed(core::IntRect)),
            this, SLOT(selection_area_changed(core::IntRect)));
        
        connect(ui_.editorCanvas, SIGNAL(area_selected(core::IntRect)), 
                this, SLOT(area_selected(core::IntRect)));

        connect(ui_.editorCanvas, SIGNAL(tile_selection_changed(std::size_t)),
            this, SLOT(tile_selection_changed(std::size_t)));

        connect(ui_.editorCanvas, SIGNAL(tile_selection_hover_changed(const components::Tile*)),
            this, SLOT(tile_selection_hover_changed(const components::Tile*)));

        connect(ui_.editorCanvas, SIGNAL(tiles_moved(core::Vector2i)),
            this, SLOT(tiles_moved(core::Vector2i)));

        connect(ui_.editorCanvas, SIGNAL(tiles_movement_finished()), this, SLOT(tiles_movement_finished()));

        connect(ui_.editorCanvas, SIGNAL(tiles_rotated(core::Rotation<double>)),
            this, SLOT(tiles_rotated(core::Rotation<double>)));

        connect(ui_.editorCanvas, SIGNAL(tiles_rotation_finished()), this, SLOT(tiles_rotation_finished()));

        connect(ui_.editorCanvas, SIGNAL(placement_tile_changed(const components::TileGroupDefinition*)),
            this, SLOT(placement_tile_changed(const components::TileGroupDefinition*)));

        connect(ui_.editorCanvas, SIGNAL(placement_tile_rotated(std::int32_t)),
            this, SLOT(placement_tile_rotated(std::int32_t)));

        connect(ui_.actionDeselect, SIGNAL(triggered()), ui_.editorCanvas, SLOT(deselect()));

        connect(ui_.layerList, SIGNAL(hide_layer(std::size_t)), ui_.editorCanvas, SLOT(hide_layer(std::size_t)));
        connect(ui_.layerList, SIGNAL(show_layer(std::size_t)), ui_.editorCanvas, SLOT(show_layer(std::size_t)));

        connect(ui_.layerList, SIGNAL(move_layer(std::size_t, std::size_t)),
            ui_.editorCanvas, SLOT(move_layer(std::size_t, std::size_t)));

        connect(ui_.layerList, SIGNAL(rename_layer(std::size_t, const std::string&)),
            ui_.editorCanvas, SLOT(rename_layer(std::size_t, const std::string&)));

        connect(ui_.actionNew_Layer, SIGNAL(triggered()), ui_.editorCanvas, SLOT(create_layer()));
        connect(ui_.actionDelete_Layer, SIGNAL(triggered()), this, SLOT(delete_layer_confirmation()));

        connect(ui_.layerList, SIGNAL(select_layer(std::size_t)), ui_.editorCanvas, SLOT(select_layer(std::size_t)));
        connect(ui_.layerList, SIGNAL(deselect_layer()), ui_.editorCanvas, SLOT(deselect_layer()));

        connect(ui_.layerList, SIGNAL(request_selected_layer_update()), this, SLOT(update_layer_selection()));

        connect(ui_.editorCanvas, SIGNAL(layer_created(std::size_t, std::size_t)),
            ui_.layerList, SLOT(layer_created(std::size_t, std::size_t)));

        connect(ui_.editorCanvas, SIGNAL(layer_deleted(std::size_t)),
            ui_.layerList, SLOT(layer_deleted(std::size_t)));

        connect(ui_.editorCanvas, SIGNAL(layer_moved(std::size_t, std::size_t)),
            ui_.layerList, SLOT(layer_moved(std::size_t, std::size_t)));

        connect(ui_.editorCanvas, SIGNAL(layer_renamed(std::size_t, const std::string&)),
            ui_.layerList, SLOT(layer_renamed(std::size_t, const std::string&)));

        connect(ui_.editorCanvas, SIGNAL(layer_level_changed(std::size_t, std::size_t)),
            ui_.layerList, SLOT(layer_level_changed(std::size_t, std::size_t)));
        
        connect(ui_.editorCanvas, SIGNAL(layer_selected(std::size_t)), this, SLOT(layer_selected(std::size_t)));
        connect(ui_.editorCanvas, SIGNAL(layer_deselected()), this, SLOT(layer_deselected()));

        connect(ui_.editorCanvas, SIGNAL(layer_merging_enabled(bool)),
            ui_.actionMergeLayerWithPrevious, SLOT(setEnabled(bool)));

        connect(ui_.actionMergeLayerWithPrevious, SIGNAL(triggered()),
            ui_.editorCanvas, SLOT(merge_selected_layer_with_previous()));

        connect(ui_.actionLayer_Properties, SIGNAL(triggered()), this, SLOT(show_layer_properties()));

        connect(layer_properties_dialog_, SIGNAL(rename_layer(std::size_t, const std::string&)),
            ui_.editorCanvas, SLOT(rename_layer(std::size_t, const std::string&)));

        connect(layer_properties_dialog_, SIGNAL(hide_layer(std::size_t)),
            ui_.editorCanvas, SLOT(hide_layer(std::size_t)));

        connect(layer_properties_dialog_, SIGNAL(show_layer(std::size_t)),
            ui_.editorCanvas, SLOT(show_layer(std::size_t)));

        connect(layer_properties_dialog_, SIGNAL(set_layer_level(std::size_t, std::size_t)),
            ui_.editorCanvas, SLOT(set_layer_level(std::size_t, std::size_t)));

        connect(ui_.actionFill_Area, SIGNAL(triggered()), this, SLOT(show_fill_dialog()));
        connect(fill_dialog_, SIGNAL(fill_area(const FillProperties&)),
            ui_.editorCanvas, SLOT(fill_area(const FillProperties&)));

        connect(ui_.editorCanvas, SIGNAL(pit_defined(core::IntRect)),
            this, SLOT(pit_defined(core::IntRect)));

        connect(ui_.editorCanvas, SIGNAL(pit_undefined()), this, SLOT(pit_undefined()));

        connect(ui_.editorCanvas, SIGNAL(perform_action(const Action&)),
            ui_.actionHistoryList, SLOT(push_action(const Action&)));

        connect(ui_.editorCanvas, SIGNAL(perform_action(const Action&)), this, SLOT(action_performed()));

        connect(ui_.actionUndo, SIGNAL(triggered()), ui_.actionHistoryList, SLOT(undo()));
        connect(ui_.actionRedo, SIGNAL(triggered()), ui_.actionHistoryList, SLOT(redo()));

        connect(ui_.actionHistoryList, SIGNAL(enable_undo()), this, SLOT(enable_undo()));
        connect(ui_.actionHistoryList, SIGNAL(disable_undo()), this, SLOT(disable_undo()));
        connect(ui_.actionHistoryList, SIGNAL(enable_redo()), this, SLOT(enable_redo()));
        connect(ui_.actionHistoryList, SIGNAL(disable_redo()), this, SLOT(disable_redo()));
    }

    MainWindow::~MainWindow()
    {
    }

    void MainWindow::open_track()
    {
        QString file_path = QFileDialog::getOpenFileName(this, "Open Track", "./tracks",
            "Turbo Sliders Tracks (*.trk)");

        if (!file_path.isEmpty())
        {
            loading_dialog_->load_track(file_path);
        }        
    }

    void MainWindow::save_track()
    {
        saved_ = true;

        try
        {
            components::save_track(ui_.editorCanvas->track());            
        }
        
        catch (const std::exception& e)
        {
            QMessageBox::critical(this, "Save Error", "Error saving track: " + QString(e.what()));
        }
    }

    void MainWindow::scene_loaded(const scene::Scene* scene_ptr)
    {
        ui_.layerList->scene_loaded(scene_ptr);
        
        ui_.editorCanvas->set_active_mode(EditorMode::Tiles);
        ui_.editorCanvas->set_active_tool(EditorTool::Placement);

        ui_.menuLayer->setEnabled(true);
        ui_.menuEdit->setEnabled(true);
        ui_.menuMode->setEnabled(true);
        ui_.menuView->setEnabled(true);

        ui_.actionPit->setEnabled(true);
        ui_.actionTiles->setEnabled(true);
        ui_.actionPattern->setEnabled(true);
        ui_.actionControl_Points->setEnabled(true);
        ui_.actionStart_Points->setEnabled(true);

        ui_.actionPlacement->setEnabled(true);
        ui_.actionSingleSelectionTool->setEnabled(true);
        ui_.actionAreaSelectionTool->setEnabled(true);

        ui_.actionZoom_In->setEnabled(true);
        ui_.actionZoom_Out->setEnabled(true);
        ui_.actionZoom_to_fit->setEnabled(true);

        ui_.actionResize->setEnabled(true);
        ui_.actionSave->setEnabled(true);

        ui_.actionDeleteLast->setEnabled(true);
        ui_.actionDeselect->setEnabled(true);

        ui_.actionNew_Layer->setEnabled(true);
        ui_.actionDelete_Layer->setEnabled(false);
        ui_.actionLayer_Properties->setEnabled(false);
        ui_.actionMergeLayerWithPrevious->setEnabled(false);

        ui_.actionHistoryView->setEnabled(true);
        ui_.actionLayerView->setEnabled(true);
        saved_ = true;
    }

    void MainWindow::closeEvent(QCloseEvent* event)
    {
        if (!saved_)
        {
            auto result = QMessageBox::question(this, "Exit", "Do you want to save the track before exiting?",
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

            if (result == QMessageBox::Cancel)
            {
                event->ignore();
            }

            if (result == QMessageBox::Yes)
            {
                ui_.actionSave->trigger();
            }
        }

        event->accept();
    }

    void MainWindow::tool_enabled(EditorTool tool)
    {
        set_tool_enabled(tool, true);
    }

    void MainWindow::tool_disabled(EditorTool tool)
    {
        set_tool_enabled(tool, false);
    }

    void MainWindow::set_tool_enabled(EditorTool tool, bool enabled)
    {
        if (auto action = tool_action(tool))
        {
            action->setEnabled(enabled);
        }
    }

    void MainWindow::mode_changed(EditorMode mode)
    {
        std::initializer_list<QAction*> modes =
        {
            ui_.actionTiles,
            ui_.actionControl_Points,
            ui_.actionStart_Points,
            ui_.actionPit,
            ui_.actionPattern
        };

        for (auto action : modes)
        {
            action->setChecked(false);
        }

        if (auto action = mode_action(mode))
        {
            action->setChecked(true);
        }

        tool_changed(ui_.editorCanvas->active_tool());
    }

    QAction* MainWindow::mode_action(EditorMode mode) const
    {
        switch (mode)
        {
        case EditorMode::Tiles:
            return ui_.actionTiles;
        case EditorMode::ControlPoints:
            return ui_.actionControl_Points;
        case EditorMode::StartPoints:
            return ui_.actionStart_Points;
        case EditorMode::Pit:
            return ui_.actionPit;
        case EditorMode::Pattern:
            return ui_.actionPattern;
        }

        return nullptr;
    }

    QAction* MainWindow::tool_action(EditorTool tool) const
    {
        switch (tool)
        {
        case EditorTool::Placement:
            return ui_.actionPlacement;
        case EditorTool::Movement:
            return ui_.actionMoveTool;
        case EditorTool::Rotation:
            return ui_.actionRotation_Tool;
        case EditorTool::TileSelection:
            return ui_.actionSingleSelectionTool;
        case EditorTool::AreaSelection:
            return ui_.actionAreaSelectionTool;
        }

        return nullptr;
    }

    void MainWindow::clear_tool_info()
    {
        tool_info_label_->setText({});
        secondary_tool_info_label_->setText({});
    }

    void MainWindow::tool_changed(EditorTool tool)
    {
        ui_.actionFill_Area->setEnabled(tool == EditorTool::Placement &&
            ui_.editorCanvas->active_mode() == EditorMode::Tiles);

        std::initializer_list<QAction*> tools = 
        {
            ui_.actionMoveTool,
            ui_.actionPlacement,
            ui_.actionSingleSelectionTool,
            ui_.actionAreaSelectionTool,
            ui_.actionRotation_Tool
        };

        for (QAction* action : tools)
        {
            action->setChecked(false);
        }
                
        if (auto action = tool_action(tool))
        {
            action->setChecked(true);
        }
    }

    void MainWindow::tile_selection_changed(std::size_t selected_tile_count)
    {
        bool enable_tools = selected_tile_count != 0;

        ui_.actionMoveTool->setEnabled(enable_tools);
        ui_.actionRotation_Tool->setEnabled(enable_tools);

        ui_.actionDelete->setEnabled(enable_tools);
        ui_.actionCut->setEnabled(enable_tools);
        ui_.actionCopy->setEnabled(enable_tools);
    }

    void MainWindow::layer_selected(std::size_t layer_id)
    {
        ui_.actionDelete_Layer->setEnabled(true);
        ui_.actionLayer_Properties->setEnabled(true);

        if (ui_.editorCanvas->active_mode() == EditorMode::Tiles && 
            ui_.editorCanvas->active_tool() == EditorTool::Placement)
        {
            ui_.actionFill_Area->setEnabled(true);
        }

        update_layer_selection();
    }

    void MainWindow::layer_deselected()
    {
        ui_.actionDelete_Layer->setEnabled(false);
        ui_.actionLayer_Properties->setEnabled(false);

        ui_.actionFill_Area->setEnabled(false);

        update_layer_selection();
    }

    void MainWindow::update_layer_selection()
    {
        auto selected_layer = ui_.editorCanvas->selected_layer();
        ui_.layerList->update_layer_selection(selected_layer.id());
    }

    void MainWindow::show_layer_properties()
    {
        if (auto layer = ui_.editorCanvas->selected_layer())
        {
            layer_properties_dialog_->display_layer_properties(layer, ui_.editorCanvas->num_levels());
        }
    }

    void MainWindow::show_fill_dialog()
    {
        fill_dialog_->show();
    }

    void MainWindow::delete_layer_confirmation()
    {
        if (auto layer = ui_.editorCanvas->selected_layer())
        {
            auto answer = QMessageBox::question(this, "Confirm", "Are you sure you want to delete the selected layer?",
                QMessageBox::Ok | QMessageBox::Cancel);

            if (answer == QMessageBox::Ok)
            {
                ui_.editorCanvas->delete_layer(layer.id());
            }
        }
    }
    
    void MainWindow::resize_track()
    {
        auto track_size = ui_.editorCanvas->track_size();
        resize_track_dialog_->show(track_size.x, track_size.y);
    }

    void MainWindow::enable_pasting()
    {
        ui_.actionPaste->setEnabled(true);
    }

    void MainWindow::disable_pasting()
    {
        ui_.actionPaste->setEnabled(false);
    }

    void MainWindow::action_performed()
    {
        saved_ = false;
    }

    void MainWindow::enable_undo()
    {
        ui_.actionUndo->setEnabled(true);
    }

    void MainWindow::disable_undo()
    {
        ui_.actionUndo->setDisabled(true);
    }

    void MainWindow::enable_redo()
    {
        ui_.actionRedo->setEnabled(true);
    }

    void MainWindow::disable_redo()
    {
        ui_.actionRedo->setDisabled(true);
    }

    void MainWindow::tile_selection_hover_changed(const components::Tile* tile)
    {
        if (tile)
        {
            QString text = "Tile " + QString::number(tile->id);
            tool_info_label_->setText(text);

            QString secondary_text = "x=";
            secondary_text += QString::number(static_cast<int>(tile->position.x));
            secondary_text += " y=";
            secondary_text += QString::number(static_cast<int>(tile->position.y));
            secondary_text += " r=";
            secondary_text += QString::number(static_cast<int>(tile->rotation.degrees(core::rotation::absolute)));
            secondary_text += L'°';
            secondary_tool_info_label_->setText(secondary_text);
        }

        else
        {
            tool_info_label_->setText({});
            secondary_tool_info_label_->setText({});
        }
    }

    void MainWindow::placement_tile_changed(const components::TileGroupDefinition* tile_group)
    {
        if (tile_group)
        {
            auto text = "Tile " + QString::number(tile_group->id());
            tool_info_label_->setText(text);            
        }

        else
        {
            tool_info_label_->setText({});
        }

        secondary_tool_info_label_->setText({});
    }

    void MainWindow::placement_tile_rotated(std::int32_t rotation)
    {
        QString text = QString::number(rotation);
        text += L'°';

        secondary_tool_info_label_->setText(text);

        fill_dialog_->set_fill_rotation(rotation);
    }

    void MainWindow::selection_area_changed(core::IntRect area)
    {
        if (area.width != 0 && area.height != 0)
        {
            QString text = "Selection: [x=";
            text += QString::number(area.left);
            text += " y=";
            text += QString::number(area.top);
            text += " w=";
            text += QString::number(area.width);
            text += " h=";
            text += QString::number(area.height);
            text += "]";

            tool_info_label_->setText(text);
        }

        else
        {
            tool_info_label_->setText({});
        }

        secondary_tool_info_label_->setText({});
    }

    void MainWindow::area_selected(core::IntRect area)
    {
        fill_dialog_->set_selection_fill_enabled(area.width != 0 && area.height != 0);
    }

    void MainWindow::tiles_moved(core::Vector2i offset)
    {
        QString text = "Offset: ";
        text += QString::number(offset.x);
        text += ", ";
        text += QString::number(offset.y);

        tool_info_label_->setText(text);
    }

    void MainWindow::tiles_rotated(core::Rotation<double> delta)
    {
        QString text = "Rotation: ";
        text += QString::number(static_cast<int>(delta.degrees()));
        text += L'°';

        tool_info_label_->setText(text);
    }

    void MainWindow::tiles_movement_finished()
    {
        tool_info_label_->setText({});
        secondary_tool_info_label_->setText({});
    }

    void MainWindow::tiles_rotation_finished()
    {
        tool_info_label_->setText({});
        secondary_tool_info_label_->setText({});
    }
    
    void MainWindow::pit_defined(core::IntRect pit)
    {
        QString text = "Pit defined from (";
        text += QString::number(pit.left);
        text += ", ";
        text += QString::number(pit.top);
        text += ") to (";
        text += QString::number(pit.right());
        text += ", ";
        text += QString::number(pit.bottom());
        text += ").";

        tool_info_label_->setText(text);
        secondary_tool_info_label_->setText({});
    }

    void MainWindow::pit_undefined()
    {
        tool_info_label_->setText("No pit defined.");
        secondary_tool_info_label_->setText({});
    }

    void MainWindow::toggle_layer_view()
    {
        ui_.layerWindow->setVisible(!ui_.layerWindow->isVisible());
    }

    void MainWindow::toggle_history_view()
    {
        ui_.historyWindow->setVisible(!ui_.historyWindow->isVisible());
    }

    void MainWindow::mouse_move(const QPoint& point)
    {
        auto label_text = std::to_string(point.x()) + ", " + std::to_string(point.y());

        position_label_->setText(QString::fromStdString(label_text));
    }
}