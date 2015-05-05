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

#ifndef TRACK_LAYER_HPP
#define TRACK_LAYER_HPP

#include "tile_definition.hpp"

#include <string>
#include <cstddef>
#include <vector>

namespace components
{
    class Track;

    struct TrackLayer
    {
        std::string name;
        std::size_t level = 0;
        bool visible = true;
        std::vector<Tile> tiles;        
    };

    static const std::size_t InvalidLayerId = -1;

    template <typename LayerType>
    struct BasicLayerHandle
    {
    public:
        BasicLayerHandle(std::nullptr_t) {}

        template <typename OtherLayerType>
        BasicLayerHandle(const BasicLayerHandle<OtherLayerType>& other,
            std::enable_if_t<std::is_convertible<OtherLayerType*, LayerType*>::value>* = nullptr)
            : layer_(other.layer_),
            layer_id_(other.layer_id_)
        {
        }

        BasicLayerHandle() = default;
        explicit operator bool() const
        {
            return layer_ != nullptr;
        }

        LayerType* operator->() const
        {
            return layer_;
        }

        std::size_t id() const
        {
            return layer_id_;
        }

    private:
        BasicLayerHandle(LayerType* layer, std::size_t layer_id)
            : layer_(layer), layer_id_(layer_id)
        {}

        friend Track;

        template <typename OtherType>
        friend struct BasicLayerHandle;

        LayerType* layer_ = nullptr;
        std::size_t layer_id_ = InvalidLayerId;
    };

    using LayerHandle = BasicLayerHandle < TrackLayer > ;
    using ConstLayerHandle = BasicLayerHandle < const TrackLayer > ;

    template <typename T1, typename T2>
    bool operator==(const BasicLayerHandle<T1>& a, const BasicLayerHandle<T2>& b)
    {
        return a.id() == b.id();
    }

    template <typename T1, typename T2>
    bool operator!=(const BasicLayerHandle<T1>& a, const BasicLayerHandle<T2>& b)
    {
        return a.id() != b.id();
    }
}

#endif