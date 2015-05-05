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

    void QtSFMLCanvas::showEvent(QShowEvent*)
    {
        if (!initialized_)
        {
            sf::RenderWindow::create(reinterpret_cast<sf::WindowHandle>(winId()));

            onInitialize();

            timer_.start(15);

            connect(&timer_, SIGNAL(timeout()), this, SLOT(repaint()));

            initialized_ = true;
        }
    }

    void QtSFMLCanvas::resizeEvent(QResizeEvent* event)
    {
        auto new_size = event->size();
    }

    void QtSFMLCanvas::closeEvent(QCloseEvent*)
    {
        initialized_ = false;

        timer_.stop();

        sf::RenderWindow::close();
    }

    void QtSFMLCanvas::paintEvent(QPaintEvent*)
    {
        // Process the SFML events
        for (sf::Event event; pollEvent(event);)
        {
        }

        onRender();

        display();
    }

    QPaintEngine* QtSFMLCanvas::paintEngine() const
    {
        return nullptr;
    }
}