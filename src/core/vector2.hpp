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

#ifndef VECTOR2_HPP
#define VECTOR2_HPP

#include <cmath>
#include <cstdint>
#include <utility>

namespace core
{
    template <typename T>
    struct Vector2
    {
    public:
        Vector2()
            : x(), y()
        {}

        Vector2(T x, T y)
            : x(x), y(y)
        {}

        template <typename U>
        Vector2(Vector2<U> other)
            : x(static_cast<T>(other.x)), y(static_cast<T>(other.y))
        {}

        template <typename U>
        Vector2<T>& operator=(Vector2<U> other)
        {
            x = static_cast<T>(other.x);
            y = static_cast<T>(other.y);
            return *this;
        }


        Vector2<T>& operator+=(Vector2<T> other)
        {
            x += other.x;
            y += other.y;
            return *this;
        }

        Vector2<T>& operator+=(T num)
        {
            x += num;
            y += num;
            return *this;
        }

        Vector2<T>& operator-=(Vector2<T> other)
        {
            x -= other.x;
            y -= other.y;
            return *this;
        }

        Vector2<T>& operator-=(T num)
        {
            x -= num;
            y -= num;
            return *this;
        }

        Vector2<T>& operator*=(Vector2<T> other)
        {
            x *= other.x;
            y *= other.y;
            return *this;
        }

        Vector2<T>& operator*=(T num)
        {
            x *= num;
            y *= num;
            return *this;
        }

        Vector2<T>& operator/=(Vector2<T> other)
        {
            x /= other.x;
            y /= other.y;
            return *this;
        }


        Vector2<T>& operator/=(T num)
        {
            x /= num;
            y /= num;
            return *this;
        }

        T x;
        T y;
    };

    template <typename T>
    Vector2<T> operator+(Vector2<T> a, Vector2<T> b)
    {
        return a += b;
    }

    template <typename T>
    Vector2<T> operator-(Vector2<T> a, Vector2<T> b)
    {
        return a -= b;
    }

    template <typename T>
    Vector2<T> operator*(Vector2<T> a, Vector2<T> b)
    {
        return a *= b;
    }

    template <typename T>
    Vector2<T> operator/(Vector2<T> a, Vector2<T> b)
    {
        return a /= b;
    }

    template <typename T>
    Vector2<T> operator+(Vector2<T> vec, T num)
    {
        return vec += num;
    }

    template <typename T>
    Vector2<T> operator+(T num, Vector2<T> vec)
    {
        return vec += num;
    }

    template <typename T>
    Vector2<T> operator-(Vector2<T> vec, T num)
    {
        return vec -= num;
    }

    template <typename T>
    Vector2<T> operator-(T num, Vector2<T> vec)
    {
        return vec -= num;
    }

    template <typename T>
    Vector2<T> operator*(Vector2<T> vec, T num)
    {
        return vec *= num;
    }

    template <typename T>
    Vector2<T> operator*(T num, Vector2<T> vec)
    {
        return vec *= num;
    }

    template <typename T>
    Vector2<T> operator/(Vector2<T> vec, T num)
    {
        return vec /= num;
    }

    template <typename T>
    Vector2<T> operator/(T num, Vector2<T> vec)
    {
        return vec /= num;
    }

    template <typename T>
    Vector2<T> operator-(Vector2<T> vec)
    {
        vec.x = -vec.x;
        vec.y = -vec.y;
        return vec;
    }

    template <typename T, typename U>
    bool operator==(Vector2<T> a, Vector2<U> b)
    {
        return a.x == b.x && a.y == b.y;
    }

    template <typename T, typename U>
    bool operator!=(Vector2<T> a, Vector2<U> b)
    {
        return !(a == b);
    }

    template <typename T>
    T magnitude(Vector2<T> vec)
    {
        using std::abs;
        using std::hypot;

        if (vec.x == 0) return abs(vec.y);
        if (vec.y == 0) return abs(vec.x);

        return hypot(vec.x, vec.y);
    }

    template <typename T, typename U>
    decltype(std::declval<T>() * std::declval<U>()) dot_product(Vector2<T> a, Vector2<U> b)
    {
        return a.x * b.x + a.y * b.y;
    }

    using Vector2u = Vector2<std::uint32_t>;
    using Vector2i = Vector2<std::int32_t>;
    using Vector2f = Vector2<float>;
    using Vector2d = Vector2<double>;

    template <typename T>
    Vector2<T> normalize(Vector2<T> vec)
    {
        auto mag = magnitude(vec);
        if (mag == 0) return vec;

        return vec /= mag;
    }
}


#endif