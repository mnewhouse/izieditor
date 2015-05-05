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

#ifndef LAYER_LIST_WIDGET_HPP
#define LAYER_LIST_WIDGET_HPP

#include <qlistview.h>
#include <qlistwidget.h>
#include <qcheckbox.h>
#include <qspinbox.h>

#include "components/track_layer.hpp"

#include <string>

namespace scene
{
    class Scene;
}

namespace interface
{
    class LayerListModel
        : public QAbstractListModel
    {
        Q_OBJECT

    public:
        LayerListModel(QObject* parent, const scene::Scene* scene_ptr);
        
        virtual int rowCount(const QModelIndex& index) const override;
        virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
        
        virtual Qt::DropActions supportedDragActions() const override;
        virtual Qt::DropActions supportedDropActions() const override;

        virtual bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
            const QModelIndex& destinationParent, int destinationChild) override;

        const scene::Scene* scene() const;

    signals:
        void move_layer(std::size_t, std::size_t);

    private:
        const scene::Scene* scene_ = nullptr;
    };

    class LayerListItemWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        LayerListItemWidget(QWidget* parent, components::ConstLayerHandle layer);

        void set_layer(components::ConstLayerHandle layer);
        components::ConstLayerHandle layer() const;

    signals:
        void hide_layer(std::size_t);
        void show_layer(std::size_t);
        void rename_layer(std::size_t, const std::string&);        

    private slots:
        void visibility_change(int state);
        void name_change();

    private:
        QLineEdit* layer_name_edit_;
        QCheckBox* visibility_checkbox_;

        components::ConstLayerHandle layer_;
    };

    class LayerListView
        : public QListView
    {
        Q_OBJECT

    public:
        LayerListView(QWidget* parent);

        virtual void dragMoveEvent(QDragMoveEvent*) override;
        virtual void dropEvent(QDropEvent*) override;

        void populate_view();

    signals:
        void move_layer(std::size_t, std::size_t);
        void hide_layer(std::size_t);
        void show_layer(std::size_t);
        void rename_layer(std::size_t, const std::string&);

        void select_layer(std::size_t);
        void deselect_layer();

        void request_selected_layer_update();

    public slots:
        void scene_loaded(const scene::Scene* scene_ptr);
        void scene_unloaded();

        void dispatch_layer_move_signal(std::size_t layer_id, std::size_t new_index);
        void dispatch_layer_hide_signal(std::size_t layer_id);
        void dispatch_layer_show_signal(std::size_t layer_id);
        void dispatch_layer_rename_signal(std::size_t layer_id, const std::string& new_name);

        /*
        void new_layer_request();
        void new_layer_request(const std::string& layer_name, std::size_t level);

        void delete_layer_request(std::size_t layer_id);
        */

        void update_layer_selection(std::size_t layer_id);
        
        void layer_created(std::size_t layer_id, std::size_t index);
        void layer_moved(std::size_t layer_id, std::size_t new_index);
        void layer_selected(std::size_t layer_id);
        void layer_deleted(std::size_t layer_id);
        void layer_renamed(std::size_t layer_id, const std::string& new_name);
        void layer_deselected();

        void layer_level_changed(std::size_t, std::size_t);

    private:
        virtual void selectionChanged(const QItemSelection&, const QItemSelection&) override;
        
        LayerListModel* model_ = nullptr;
    };
};

#endif