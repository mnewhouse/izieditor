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

#include "tile_mapping.hpp"

#include <functional>

namespace scene
{
    TextureCreationError::TextureCreationError()
        : std::runtime_error("could not create texture")
    {
    }

    TileMapping::TileMapping(TileMapping&& other)
        : tile_placement_(std::move(other.tile_placement_)),
          tile_fragments_(std::move(other.tile_fragments_)),
          textures_(std::move(other.textures_))        
    {
    }

    TileMapping& TileMapping::operator=(TileMapping&& rhs)
    {
        tile_placement_ = std::move(rhs.tile_placement_);
        tile_fragments_ = std::move(rhs.tile_fragments_);
        textures_ = std::move(rhs.textures_);

        return *this;
    }

    TileMapping::PlacementRange TileMapping::find_tile(components::TileId tile_id,
        const sf::Texture* texture_hint) const
    {
        TilePlacement dummy;
        dummy.tile_id = tile_id;

        auto comparator = [](const TilePlacement& first, const TilePlacement& second)
        {
            return first.tile_id < second.tile_id;
        };

        auto range = std::equal_range(tile_placement_.begin(), tile_placement_.end(), dummy, comparator);
        if (range.first != range.second)
        {
            if (texture_hint)
            {
                auto search_result = std::find_if(range.first, range.second, [texture_hint](const TilePlacement& placement)
                {
                    return placement.texture == texture_hint;
                });

                if (search_result != range.second)
                {
                    return boost::make_iterator_range(&*search_result, &*search_result + 1);
                }
            }

            return boost::make_iterator_range(&*range.first, &*range.first + 1);
        }

        range = std::equal_range(tile_fragments_.begin(), tile_fragments_.end(), dummy, comparator);
        return boost::make_iterator_range(&*range.first, &*range.second);
    }

    const sf::Texture* TileMapping::create_texture_from_image(const sf::Image& image, sf::IntRect rect)
    {
        auto texture = std::make_unique<sf::Texture>();
        if (!texture->loadFromImage(image, rect))
        {
            throw TextureCreationError();
        }

        textures_.push_back(std::move(texture));
        return textures_.back().get();
    }

    void TileMapping::define_tile_placement(components::TileId tile_id, const sf::Texture* texture, 
        core::IntRect tile_rect, core::IntRect texture_rect)
    {
        auto insert_position = std::lower_bound(tile_placement_.begin(), tile_placement_.end(), tile_id,
            [](const TilePlacement& placement, components::TileId tile_id)
        {
            return placement.tile_id < tile_id;
        });

        // This will be kinda inefficient, maybe there's a better solution.

        TilePlacement placement;
        placement.tile_id = tile_id;
        placement.tile_rect = tile_rect;
        placement.texture_rect = texture_rect;
        placement.texture = texture;
        tile_placement_.insert(insert_position, placement);
    }

    void TileMapping::define_tile_fragment(components::TileId tile_id, const sf::Texture* texture, 
        core::IntRect tile_rect, core::IntRect texture_rect)
    {
        auto insert_position = std::lower_bound(tile_fragments_.begin(), tile_fragments_.end(), tile_id,
            [](const TilePlacement& placement, components::TileId tile_id)
        {
            return placement.tile_id < tile_id;
        });

        TilePlacement fragment;
        fragment.tile_id = tile_id;
        fragment.tile_rect = tile_rect;
        fragment.texture_rect = texture_rect;
        fragment.texture = texture;
        tile_fragments_.insert(insert_position, fragment);
    }
}