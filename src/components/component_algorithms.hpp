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

#ifndef COMPONENT_ALGORITHMS_HPP
#define COMPONENT_ALGORITHMS_HPP

#include <cstdint>

#include "core/rect.hpp"
#include "core/rotation.hpp"

#include "tile_library.hpp"
#include "tile_definition.hpp"

namespace components
{
    struct FillProperties
    {
        core::IntRect area;
        core::Rotation<double> rotation;
        bool randomize_rotation = false;
        
        double position_jitter = 0.0;
        double density = 1.0;
    };

    template <typename RNG, typename OutIt>
    void fill_area(const TileGroupDefinition& tile_group, const TileLibrary& tile_library, 
        const FillProperties& properties, RNG&& rng, OutIt out);

    core::IntRect tile_group_bounding_box(const TileGroupDefinition& tile_group, const TileLibrary& tile_library);
}

#include "component_algorithms.inl"

#endif