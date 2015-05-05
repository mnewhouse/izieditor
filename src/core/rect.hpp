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

#ifndef RECT_HPP
#define RECT_HPP

#include "vector2.hpp"

#include <algorithm>
#include <type_traits>
#include <cstdint>
#include <ostream>

namespace core
{
    namespace rect
    {
        struct from_points_t {};
        static const from_points_t from_points;
    }

    template <typename T>
    struct Rect
    {
    public:
        Rect()
            : left(), top(), width(), height()
        {}

        template <typename T1, typename T2>
        Rect(Vector2<T1> point_a, Vector2<T2> point_b, rect::from_points_t)
            : left(std::min(point_a.x, point_b.x)),
              top(std::min(point_a.y, point_b.y)),
              width(std::max(point_a.x, point_b.x) - left),
              height(std::max(point_a.y, point_b.y) - top)
        {
        }

        template <typename T1, typename T2>
        Rect(Vector2<T1> point, Vector2<T2> size)
            : left(point.x), top(point.y), width(size.x), height(size.y)
        {}

        Rect(T left, T top, T width, T height)
            : left(left), top(top), width(width), height(height)
        {}

        T right() const { return left + width; }
        T bottom() const { return top + height; }

        T left;
        T top;
        T width;
        T height;
    };

    template <typename T, typename U>
    Rect<typename std::common_type<T, U>::type> combine(Rect<T> a, Rect<U> b)
    {
        using result_type = typename std::common_type<T, U>::type;

        auto min_x = std::min<result_type>(a.left, b.left);
        auto max_x = std::max<result_type>(a.right(), b.right());

        auto min_y = std::min<result_type>(a.top, b.top);
        auto max_y = std::max<result_type>(a.bottom(), b.bottom());

        auto width = max_x - min_x;
        auto height = max_y - min_y;

        return Rect<result_type>(min_x, min_y, width, height);
    }

    template <typename T, typename U>
    bool contains(Rect<T> rect, Vector2<U> point)
    {
        return point.x >= rect.left && point.y >= rect.top &&
            point.x < rect.right() && point.y < rect.bottom();
    }

    template <typename T, typename U, typename Comparator = std::less<>>
    bool intersects(Rect<T> a, Rect<U> b, Comparator cmp = Comparator())
    {
        auto ax = std::minmax(a.left, a.right());
        auto ay = std::minmax(a.top, a.bottom());

        auto bx = std::minmax(b.left, b.right());
        auto by = std::minmax(b.top, b.bottom());

        return cmp(ax.first, bx.second) && cmp(ay.first, by.second) &&
            cmp(bx.first, ax.second) && cmp(by.first, ay.second);
    }

    template <typename T, typename U>
    Rect<typename std::common_type<T, U>::type> intersection(Rect<T> a, Rect<U> b)
    {
        auto a_min_x = std::min(a.left, a.right());
        auto a_max_x = std::max(a.left, a.right());
        auto a_min_y = std::min(a.top, a.bottom());
        auto a_max_y = std::max(a.top, a.bottom());

        auto b_min_x = std::min(b.left, b.right());
        auto b_max_x = std::max(b.left, b.right());
        auto b_min_y = std::min(b.top, b.bottom());
        auto b_max_y = std::max(b.top, b.bottom());

        Rect<typename std::common_type<T, U>::type> result;
        result.left = std::max(a_min_x, b_min_x);
        result.top = std::max(a_min_y, b_min_y);

        auto right = std::min(a_max_x, b_max_x);
        auto bottom = std::min(a_max_y, b_max_y);

        if (result.left < right && result.top < bottom) {
            result.width = right - result.left;
            result.height = bottom - result.top;
        }

        else {
            result.left = 0;
            result.top = 0;
        }

        return result;
    }

    template <typename T>
    bool operator==(const Rect<T>& lhs, const Rect<T>& rhs)
    {
        return lhs.left == rhs.left && lhs.top == rhs.top && lhs.width == rhs.width && lhs.height == rhs.height;
    }

    template <typename T>
    bool operator!=(const Rect<T>& lhs, const Rect<T>& rhs)
    {
        return !(lhs == rhs);
    }

    typedef Rect<float> FloatRect;
    typedef Rect<double> DoubleRect;
    typedef Rect<std::int32_t> IntRect;

    template <typename T>
    std::ostream& operator<<(std::ostream& stream, const Rect<T>& rect)
    {
        return stream << rect.left << " " << rect.top << " " << rect.width << " " << rect.height;
    }

}

#endif