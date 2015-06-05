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

#ifndef CORE_ROTATION_HPP
#define CORE_ROTATION_HPP

#include <cmath>

namespace core
{
    namespace rotation
    {
        struct Absolute {};
        static const Absolute absolute;
    }

    template <typename T>
    class Rotation
    {
    public:
        Rotation();

        static Rotation<T> degrees(T degrees);
        static Rotation<T> radians(T radians);

        template <typename U>
        Rotation<T>& operator+=(Rotation<U> right);

        template <typename U>
        Rotation<T>& operator-=(Rotation<U> right);

        Rotation<T> operator-() const;

        T degrees() const;
        T radians() const;

        T degrees(rotation::Absolute) const;
        T radians(rotation::Absolute) const;

        Rotation<T> normalize() const;

    private:
        Rotation(T radians)
            : radians_(radians)
        {
        }

        T radians_;
        static const T pi;
    };

    template <typename T>
    const T Rotation<T>::pi = T(3.141592653589793238462643383279502884197169399375105820974944);

    template <typename T>
    Rotation<T>::Rotation()
        : radians_(0)
    {
    }

    
    template <typename T>
    Rotation<T> Rotation<T>::normalize() const
    {
        const auto range = pi * T(2.0);

        using std::fmod;

        if (radians_ >= pi)
        {
            return Rotation<T>::radians(fmod(radians_ + pi, range) - pi);
        }

        else if (radians_ < -pi)
        {
            return Rotation<T>::radians(fmod(radians_ - pi, range) + pi);
        }

        return *this;
    }

    template <typename T>
    Rotation<T> operator+(Rotation<T> left, Rotation<T> right)
    {
        return left += right;
    }

    template <typename T>
    Rotation<T> operator-(Rotation<T> left, Rotation<T> right)
    {
        return left -= right;
    }

    template <typename T>
    Rotation<T> Rotation<T>::operator-() const
    {
        return Rotation<T>::radians(-radians_);
    }


    template <typename T>
    T Rotation<T>::degrees() const
    {
        return radians_ * T(180.0) / pi;
    }

    template <typename T>
    T Rotation<T>::degrees(rotation::Absolute) const
    {
        auto result = degrees();
        if (result < 0.0)
        {
            result += 360.0;
        }

        return result;
    }

    template <typename T>
    T Rotation<T>::radians(rotation::Absolute) const
    {
        auto result = radians();
        if (result < 0.0)
        {
            result += pi * 2.0;
        }

        return result;
    }

    template <typename T>
    T Rotation<T>::radians() const
    {
        return radians_;
    }

    template <typename T>
    Rotation<T> Rotation<T>::degrees(T value)
    {
        return Rotation<T>(value / T(360.0) * pi * T(2.0));
    }

    template <typename T>
    Rotation<T> Rotation<T>::radians(T value)
    {
        return Rotation<T>(value);
    }

    template <typename T>
    template <typename U>
    Rotation<T>& Rotation<T>::operator+=(Rotation<U> rotation)
    {
        radians_ += rotation.radians();

        return *this;
    }

    template <typename T>
    template <typename U>
    Rotation<T>& Rotation<T>::operator-=(Rotation<U> rotation)
    {
        radians_ -= rotation.radians();

        return *this;
    }
}

#endif