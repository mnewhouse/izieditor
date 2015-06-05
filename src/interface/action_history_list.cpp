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

#include "action_history_list.hpp"

#include <memory>

namespace interface
{
    ActionHistoryList::ActionHistoryList(QWidget* parent)
        : QListWidget(parent)
    {
    }

    void ActionHistoryList::set_max_stack_size(std::size_t max_size)
    {
        max_stack_size_ = max_size;

        while (actions_.size() > max_size)
        {
            actions_.pop_front();
        }
    }

    std::size_t ActionHistoryList::max_stack_size() const
    {
        return max_stack_size_;
    }

    void ActionHistoryList::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        // WOW HACK
        if (is_updating_) return;

        auto selection = selectedIndexes();
        int row_count = model()->rowCount();

        if (selection.size() == 1)
        {
            QModelIndex model_index = selection.front();
            std::size_t index = model_index.row() + 1;

            while (current_index_ < index)
            {
                actions_[current_index_++].execute();
            }

            while (current_index_ > index)
            {
                actions_[--current_index_].undo();
            }

            if (auto item = itemFromIndex(model_index))
            {
                scrollToItem(item);
            }            
        }

        else if (selection.size() == 0)
        {
            while (current_index_ != 0)
            {
                actions_[--current_index_].undo();
            }
        }

        if (current_index_ == actions_.size()) disable_redo();
        else enable_redo();

        if (current_index_ == 0) disable_undo();
        else enable_undo();

        for (std::size_t row = 0; row != current_index_; ++row)
        {
            if (auto item = itemFromIndex(model()->index(row, 0)))
            {
                item->setTextColor(QColor(0, 0, 0));
            }
        }

        for (std::size_t row = current_index_, row_count = model()->rowCount(); row != row_count; ++row)
        {
            if (auto item = itemFromIndex(model()->index(row, 0)))
            {
                item->setTextColor(QColor(0, 0, 0, 100));
            }
        }

        QListWidget::selectionChanged(selected, deselected);
    }

    void ActionHistoryList::push_action(const Action& action)
    {
        {
            is_updating_ = true;
            auto update_guard_deleter = [](bool* is_updating)
            {
                *is_updating = false;
            };

            std::unique_ptr<bool, decltype(update_guard_deleter)> update_guard(&is_updating_);

            model()->removeRows(current_index_, actions_.size() - current_index_);
            addItem(QString::fromStdString(action.description()));

            actions_.resize(current_index_);
            actions_.push_back(action);

            if (actions_.size() > max_stack_size_)
            {
                model()->removeRows(0, 1);
                actions_.pop_front();
            }

            current_index_ = actions_.size();
        }

        selectionModel()->select(model()->index(current_index_ - 1, 0), QItemSelectionModel::ClearAndSelect);

        // Current index == 0 -> undo not possible
        // Current index > 0 -> undo possible

        // index < action_count -> redo possible
        // index == action_count -> redo not possible

        // If selection index == 0 -> undo all except first action
        // If selection index == size() - 1 -> undo nothing
    }

    void ActionHistoryList::undo(std::size_t num_actions)
    {
        if (current_index_ > num_actions)
        {
            auto index = model()->index(current_index_ - num_actions - 1, 0);

            selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
        }

        else
        {
            selectionModel()->clearSelection();
        }        
    }

    void ActionHistoryList::redo(std::size_t num_actions)
    {
        num_actions = std::max<std::size_t>(num_actions, 1);
        std::size_t row_index = std::min(current_index_ + num_actions, actions_.size()) - 1;

        auto model_index = model()->index(row_index, 0);
        selectionModel()->select(model_index, QItemSelectionModel::ClearAndSelect);
    }

    void ActionHistoryList::clear()
    {
        model()->removeRows(0, model()->rowCount());

        actions_.clear();
    }

    bool ActionHistoryList::has_performed_any_actions() const
    {
        return current_index_ != 0;
    }
};