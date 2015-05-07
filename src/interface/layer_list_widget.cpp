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

#include "layer_list_widget.hpp"

#include "scene/scene.hpp"

#include <qevent.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <qmimedata.h>

namespace interface
{
    static const QString visibility_checkbox_style = "QCheckBox::indicator{ width: 18px; height: 18px; }"
        "QCheckBox::indicator:unchecked { image: url(:/Icons/resources/eye-close.png); }"
        "QCheckBox::indicator:checked { image: url(:/Icons/resources/eye.png); }";

    LayerListView::LayerListView(QWidget* parent)
        : QListView(parent)
    {
    }

    void LayerListView::scene_loaded(const scene::Scene* scene_ptr)
    {
        model_ = new LayerListModel(this, scene_ptr);
        connect(model_, SIGNAL(move_layer(std::size_t, std::size_t)),
            this, SLOT(dispatch_layer_move_signal(std::size_t, std::size_t)));

        setModel(model_);

        populate_view();
    }

    void LayerListView::scene_unloaded()
    {
        setItemDelegate(nullptr);
        setModel(nullptr);
    }

    void LayerListView::populate_view()
    {
        const auto& track = model_->scene()->track();
        const auto& layers = track.layers();

        reset();

        int row = 0;
        for (std::size_t idx = layers.size(); idx-- != 0; ++row)
        {
            const auto& layer = layers[idx];
            auto index = model_->index(row, 0);

            auto widget = new LayerListItemWidget(this, layer);
            setIndexWidget(index, widget);

            connect(widget, SIGNAL(hide_layer(std::size_t)), this, SLOT(dispatch_layer_hide_signal(std::size_t)));
            connect(widget, SIGNAL(show_layer(std::size_t)), this, SLOT(dispatch_layer_show_signal(std::size_t)));
            connect(widget, SIGNAL(rename_layer(std::size_t, const std::string&)),
                this, SLOT(dispatch_layer_rename_signal(std::size_t, const std::string&)));
        }

        request_selected_layer_update();
    }

    void LayerListView::dragMoveEvent(QDragMoveEvent* event)
    {
        auto indices = selectedIndexes();

        // If layer order would become invalid - reject the event.
        // Otherwise, move the widgets around
        if (indices.size() == 1)
        {
            auto source_row = indices.front().row();
            auto dest_row = indexAt(event->pos()).row();

            if (model_->moveRow(QModelIndex(), source_row, QModelIndex(), dest_row))
            {
                event->accept();

                QListView::dragMoveEvent(event);
                return;
            }
        }

        event->ignore();
    }

    void LayerListView::dropEvent(QDropEvent* event)
    {
        auto indices = selectedIndexes();

        if (indices.size() == 1)
        {
            std::size_t source_row = indices.front().row();
            std::size_t dest_row = indexAt(event->pos()).row();

            const auto& layers = model_->scene()->track().layers();
            std::size_t source_index = layers.size() - source_row - 1;
            std::size_t dest_index = layers.size() - dest_row - 1;
            
            model_->move_layer(layers[source_index].id(), dest_index);

            populate_view();
        }

        QListView::dropEvent(event);
    }

    void LayerListView::dispatch_layer_move_signal(std::size_t layer_id, std::size_t index)
    {
        move_layer(layer_id, index);
    }

    void LayerListView::dispatch_layer_hide_signal(std::size_t layer_id)
    {
        hide_layer(layer_id);
    }

    void LayerListView::dispatch_layer_show_signal(std::size_t layer_id)
    {
        show_layer(layer_id);
    }

    void LayerListView::dispatch_layer_rename_signal(std::size_t layer_id, const std::string& new_name)
    {
        rename_layer(layer_id, new_name);
    }

    void LayerListView::layer_created(std::size_t layer_id, std::size_t index)
    {
        populate_view();
    }

    void LayerListView::layer_moved(std::size_t layer_id, std::size_t index)
    {
        populate_view();
    }

    void LayerListView::layer_selected(std::size_t layer_id)
    {
        update_layer_selection(layer_id);
    }

    void LayerListView::layer_deleted(std::size_t layer_id)
    {
        populate_view();
    }

    void LayerListView::layer_renamed(std::size_t layer_id, const std::string& new_name)
    {
        populate_view();
    }

    void LayerListView::layer_level_changed(std::size_t layer_id, std::size_t new_level)
    {
        populate_view();
    }

    void LayerListView::layer_visibility_changed(std::size_t layer_id, bool visibility)
    {
        populate_view();
    }

    void LayerListView::layer_deselected()
    {
        selectionModel()->select(QModelIndex(), QItemSelectionModel::Clear);
    }

    void LayerListView::update_layer_selection(std::size_t selected_layer)
    {
        if (model_)
        {
            const auto& layers = model_->scene()->layers();
            auto index = model_->scene()->find_layer_index(selected_layer);

            if (index != layers.size())
            {
                std::size_t row = layers.size() - index - 1;
                auto model_index = model_->index(row);
                selectionModel()->select(model_index, QItemSelectionModel::ClearAndSelect);
            }

            else
            {
                selectionModel()->select(QModelIndex(), QItemSelectionModel::Clear);
            }            
        }
    }

    void LayerListView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        if (model_)
        {
            const auto& indices = selectedIndexes();
            const auto& layers = model_->scene()->track().layers();
            if (indices.size() == 1)
            {
                std::size_t row = indices.front().row();
                const auto& layer = layers[layers.size() - row - 1];

                select_layer(layer.id());
            }

            else if (indices.size() == 0)
            {
                deselect_layer();
            }
        }

        QListView::selectionChanged(selected, deselected);
    }

    LayerListModel::LayerListModel(QObject* parent, const scene::Scene* scene_ptr)
        : QAbstractListModel(parent),
          scene_(scene_ptr)
    {
    }

    const scene::Scene* LayerListModel::scene() const
    {
        return scene_;
    }

    int LayerListModel::rowCount(const QModelIndex& index) const
    {
        return static_cast<int>(scene_->track().layers().size());
    }

    QVariant LayerListModel::data(const QModelIndex& index, int role) const
    {
        if (role == Qt::SizeHintRole)
        {
            return QSize(100, 32);
        }

        return QVariant::Invalid;
    }

    Qt::ItemFlags LayerListModel::flags(const QModelIndex& index) const
    {
        return QAbstractItemModel::flags(index) | Qt::ItemIsDragEnabled | Qt::ItemFlag::ItemIsDropEnabled | Qt::ItemIsSelectable;
    }

    Qt::DropActions LayerListModel::supportedDragActions() const
    {
        return Qt::MoveAction;
    }

    Qt::DropActions LayerListModel::supportedDropActions() const
    {
        return Qt::MoveAction;
    }

    bool LayerListModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
        const QModelIndex& destinationParent, int destinationChild)
    {
        if (scene_ == nullptr) return false;

        const auto& layers = scene_->track().layers();

        std::size_t source_index = layers.size() - sourceRow - 1;
        std::size_t dest_index = layers.size() - destinationChild - 1;

        if (source_index >= layers.size() || dest_index >= layers.size() || count != 1) return false;

        std::size_t level = layers[source_index]->level;
        for (std::size_t idx = source_index; idx++ < dest_index; )
        {
            if (layers[idx]->level > level) return false;
        }

        for (std::size_t idx = dest_index; idx < source_index; ++idx)
        {
            if (layers[idx]->level < level) return false;
        }

        return true;
    }

    LayerListItemWidget::LayerListItemWidget(QWidget* parent, components::ConstLayerHandle layer)
        : QWidget(parent),
          layer_(layer)
    {

        auto layout = new QHBoxLayout(this);
        setLayout(layout);

        visibility_checkbox_ = new QCheckBox(this);
        visibility_checkbox_->setChecked(true);
        visibility_checkbox_->setStyleSheet(visibility_checkbox_style);
        layout->addWidget(visibility_checkbox_);

        layer_name_edit_ = new QLineEdit(QString::fromStdString(layer->name), this);
        layer_name_edit_->setStyleSheet("border: none;");
        layout->addWidget(layer_name_edit_);

        setAutoFillBackground(false);
        setAcceptDrops(1);

        connect(visibility_checkbox_, SIGNAL(stateChanged(int)), this, SLOT(visibility_change(int)));
        connect(layer_name_edit_, SIGNAL(editingFinished()), this, SLOT(name_change()));
    }

    void LayerListItemWidget::visibility_change(int state)
    {
        if (state == Qt::Checked)
        {
            show_layer(layer_.id());
        }

        else
        {
            hide_layer(layer_.id());
        }
    }

    void LayerListItemWidget::name_change()
    {
        std::string new_name = layer_name_edit_->text().toStdString();
        if (layer_->name != new_name)
        {
            rename_layer(layer_.id(), new_name);
        }
    }
}