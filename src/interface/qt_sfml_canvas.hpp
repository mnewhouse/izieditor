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

#ifndef QT_SFML_CANVAS_HPP
#define QT_SFML_CANVAS_HPP

#include <SFML/Graphics.hpp>
#include <QtWidgets/qwidget.h>
#include <QtCore/qtimer.h>

#include "core/vector2.hpp"

namespace interface
{
    class QtSFMLCanvas
        : public QWidget, public sf::RenderWindow
    {
    public:
        QtSFMLCanvas(QWidget* parent);

        virtual ~QtSFMLCanvas() = default;

        using CursorId = std::size_t;
        enum : CursorId
        {
            InvalidCursorId = CursorId(-1)
        };

        CursorId create_cursor(const sf::Image& image, sf::IntRect rect = sf::IntRect());

        void set_active_cursor(CursorId);

    protected:
        void set_prioritized_cursor(CursorId);

        virtual void showEvent(QShowEvent*) override;
        virtual void paintEvent(QPaintEvent*) override;
        virtual void resizeEvent(QResizeEvent*) override;
        virtual void closeEvent(QCloseEvent*) override;

        virtual void leaveEvent(QEvent*) override;
        virtual void enterEvent(QEvent*) override;


    private:
        void render();
        void draw_cursor();
        void update_default_cursor_visibility();

        CursorId active_cursor() const;
        bool cursor_override_enabled() const;

        virtual QPaintEngine* paintEngine() const override;


        virtual void onInitialize() {}
        virtual void onRender() {}        

        bool initialized_ = false;
        bool cursor_visible_ = false;
        bool was_cursor_visible_ = false;

        QTimer timer_;

        std::map<CursorId, sf::Texture> cursor_map_;
        CursorId active_cursor_ = InvalidCursorId;
        CursorId prioritized_cursor_ = InvalidCursorId;

        CursorId last_cursor_ = InvalidCursorId;
        core::Vector2i mouse_position_;

    };
}

#endif