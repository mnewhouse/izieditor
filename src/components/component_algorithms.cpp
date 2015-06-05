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

#include "component_algorithms.hpp"

#include "tile_definition.hpp"
#include "tile_library.hpp"

#include "core/transform.hpp"

#include <boost/optional.hpp>

namespace components
{
    core::IntRect tile_group_bounding_box(const TileGroupDefinition& tile_group, const TileLibrary& tile_lib)
    {
        boost::optional<core::IntRect> result;
        for (const auto& sub_tile : tile_group.sub_tiles())
        {
            if (auto tile = tile_lib.tile(sub_tile.id))
            {
                core::IntRect pat_rect = tile->pattern_rect;

                core::Rect<double> sub_rect(0.0, 0.0, 
                    static_cast<double>(pat_rect.width), static_cast<double>(pat_rect.height));

                core::Rect<double> transformed_rect = core::transform_rect(sub_rect, sub_tile.rotation);

                core::IntRect int_rect(
                    static_cast<std::int32_t>(sub_tile.position.x - transformed_rect.width * 0.5),
                    static_cast<std::int32_t>(sub_tile.position.y - transformed_rect.height * 0.5),
                    static_cast<std::int32_t>(transformed_rect.width),
                    static_cast<std::int32_t>(transformed_rect.height)
                );

                if (!result) result = int_rect;
                else result = combine(*result, int_rect);
            }
        }

        if (result) return *result;

        return {};
    }
}