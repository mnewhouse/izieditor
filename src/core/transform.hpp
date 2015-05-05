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

#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include "vector2.hpp"
#include "rect.hpp"
#include "rotation.hpp"

namespace core
{
    inline Vector2<double> transform_point(const Vector2<double>& point, double sin, double cos)
    {
        return{ point.x * cos - sin * point.y, point.y * cos + sin * point.x };
    }

    inline Vector2<double> transform_point(const Vector2<double>& point, Rotation<double> rotation)
    {
        auto rad = rotation.radians();
        return transform_point(point, std::sin(rad), std::cos(rad));
    }

    inline Rect<double> transform_rect(const Rect<double>& rect, double sin, double cos)
    {
        core::Vector2<double> center(rect.left + rect.width * 0.5, rect.top + rect.height * 0.5);
        double left = rect.left - center.x;
        double top = rect.top - center.y;
        double right = left + rect.width;
        double bottom = top + rect.height;

        core::Vector2<double> points[4] =
        {
            transform_point({ left, top }, sin, cos) + center,
            transform_point({ left, bottom }, sin, cos) + center,
            transform_point({ right, bottom }, sin, cos) + center,
            transform_point({ right, top }, sin, cos) + center
        };

        left = points[0].x;
        right = points[0].x;
        top = points[0].y;
        bottom = points[0].y;
        for (int i = 1; i != 4; ++i)
        {
            if (points[i].x < left) left = points[i].x;
            else if (points[i].x > right) right = points[i].x;

            if (points[i].y < top) top = points[i].y;
            else if (points[i].y > bottom) bottom = points[i].y;
        }

        return Rect<double>(left, top, right - left, bottom - top);
    }

    inline Rect<double> transform_rect(const Rect<double>& rect, Rotation<double> rotation)
    {
        auto rad = rotation.radians();
        return transform_rect(rect, std::sin(rad), std::cos(rad));
    }
}

#endif