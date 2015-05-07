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

#include "qt_sfml_canvas.hpp"

#include <qevent.h>

namespace interface
{
    QtSFMLCanvas::QtSFMLCanvas(QWidget* parent)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_PaintOnScreen);
        setAttribute(Qt::WA_OpaquePaintEvent);
        setAttribute(Qt::WA_NoSystemBackground);

        setFocusPolicy(Qt::StrongFocus);       
    }

    void QtSFMLCanvas::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        if (!initialized_)
        {
            sf::RenderWindow::create(reinterpret_cast<sf::WindowHandle>(winId()));

            onInitialize();

            timer_.start(10);

            connect(&timer_, SIGNAL(timeout()), this, SLOT(repaint()));

            initialized_ = true;
        }
    }

    void QtSFMLCanvas::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);

        auto new_size = event->size();
    }

    void QtSFMLCanvas::closeEvent(QCloseEvent* event)
    {
        QWidget::closeEvent(event);
        initialized_ = false;

        timer_.stop();

        sf::RenderWindow::close();
    }

    void QtSFMLCanvas::paintEvent(QPaintEvent* event)
    {
        QWidget::paintEvent(event);
        std::int32_t screen_width = getSize().x, screen_height = getSize().y;

        // Process the SFML events
        for (sf::Event event; pollEvent(event); )
        {
            if (event.type == sf::Event::MouseMoved)
            {
                mouse_position_.x = event.mouseMove.x;
                mouse_position_.y = event.mouseMove.y;

                bool in_area = mouse_position_.x >= 0 && mouse_position_.y >= 0 &&
                    mouse_position_.x < screen_width && mouse_position_.y < screen_height;

                cursor_visible_ = cursor_visible_ && in_area;
                setMouseCursorVisible(!cursor_override_enabled() || !in_area);
            }
        }

        render();
    }

    QPaintEngine* QtSFMLCanvas::paintEngine() const
    {
        return nullptr;
    }

    void QtSFMLCanvas::render()
    {
        onRender();

        draw_cursor();

        display();
    }

    QtSFMLCanvas::CursorId QtSFMLCanvas::create_cursor(const sf::Image& image, sf::IntRect rect)
    {
        CursorId cursor_id = 0;
        if (!cursor_map_.empty())
        {
            cursor_id = std::prev(cursor_map_.end())->first + 1;
        }

        auto result = cursor_map_.emplace(std::piecewise_construct, 
            std::forward_as_tuple(cursor_id), std::forward_as_tuple());

        if (result.second)
        {           
            if (result.first->second.loadFromImage(image, rect))
            {
                return result.first->first;
            }

            cursor_map_.erase(result.first);
        }

        return InvalidCursorId;
    }

    void QtSFMLCanvas::set_active_cursor(CursorId cursor)
    {
        active_cursor_ = cursor;

        setMouseCursorVisible(!cursor_override_enabled() || !cursor_visible_);
    }

    void QtSFMLCanvas::set_prioritized_cursor(CursorId cursor)
    {
        prioritized_cursor_ = cursor;

        setMouseCursorVisible(!cursor_override_enabled() || !cursor_visible_);
    }

    bool QtSFMLCanvas::cursor_override_enabled() const
    {
        return prioritized_cursor_ != InvalidCursorId || active_cursor_ != InvalidCursorId;
    }

    void QtSFMLCanvas::draw_cursor()
    {
        auto cursor_id = prioritized_cursor_ != InvalidCursorId ? prioritized_cursor_ : active_cursor_;

        auto map_it = cursor_map_.find(cursor_id);
        if (cursor_override_enabled() && cursor_visible_ && map_it != cursor_map_.end())
        {
            auto old_view = getView();

            sf::Vector2u my_size = getSize();
            sf::FloatRect view_rect(0.0f, 0.0f, static_cast<float>(my_size.x), static_cast<float>(my_size.y));
            sf::View view(view_rect);
            setView(view);

            sf::Sprite sprite(map_it->second);
            sprite.setPosition(mouse_position_.x, mouse_position_.y);
            draw(sprite);

            setView(old_view);
        }
    }

    void QtSFMLCanvas::leaveEvent(QEvent* event)
    {
        QWidget::leaveEvent(event);
        cursor_visible_ = false;

        setMouseCursorVisible(true);
    }

    void QtSFMLCanvas::enterEvent(QEvent* event)
    {
        QWidget::leaveEvent(event);
        cursor_visible_ = true;

        setMouseCursorVisible(!cursor_override_enabled());
    }
}