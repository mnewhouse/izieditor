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

#ifndef ACTION_HISTORY_HPP
#define ACTION_HISTORY_HPP

#include "action.hpp"

#include <qlistwidget>

#include <functional>
#include <deque>

namespace interface
{
    class ActionHistoryList
        : public QListWidget
    {
        Q_OBJECT

    public:
        ActionHistoryList(QWidget* parent);

        void set_max_stack_size(std::size_t max_size);
        std::size_t max_stack_size() const;

        bool has_performed_any_actions() const;

    public slots:
        void undo(std::size_t num_actions = 1);
        void redo(std::size_t num_actions = 1);

        void push_action(const Action& action);
        void clear();

    signals:
        void enable_undo();
        void disable_undo();

        void enable_redo();
        void disable_redo();
        

    private:
        virtual void selectionChanged(const QItemSelection&, const QItemSelection&) override;

        std::size_t current_index_ = 0;
        std::size_t max_stack_size_ = 50;
        std::deque<Action> actions_;
    };
};

#endif