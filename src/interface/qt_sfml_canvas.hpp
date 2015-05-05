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

namespace interface
{
    class QtSFMLCanvas
        : public QWidget, public sf::RenderWindow
    {
    public:
        QtSFMLCanvas(QWidget* parent);

        virtual ~QtSFMLCanvas() = default;

    private:
        virtual QPaintEngine* paintEngine() const override;
        virtual void showEvent(QShowEvent*) override;
        virtual void paintEvent(QPaintEvent*) override;
        virtual void resizeEvent(QResizeEvent*) override;
        virtual void closeEvent(QCloseEvent*) override;

        virtual void onInitialize() {}
        virtual void onRender() {}        

        bool initialized_ = false;
        QTimer timer_;
    };
}

#endif