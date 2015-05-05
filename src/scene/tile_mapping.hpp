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

#ifndef TILE_MAPPING_HPP
#define TILE_MAPPING_HPP


#include "components/tile_definition.hpp"

#include "core/rect.hpp"

#include <SFML/Graphics.hpp>

#include <boost/range.hpp>

#include <memory>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <exception>

namespace scene
{
    struct TilePlacement
    {
        components::TileId tile_id;
        const sf::Texture* texture;
        core::IntRect texture_rect;
        core::IntRect tile_rect;
    };

    struct TextureCreationError
        : std::runtime_error
    {
        TextureCreationError();
    };

    // class TileMapping. Takes (tile_definition, texture_hint) and gives back
    // all texture rects that are needed to display it.
    class TileMapping
    {
    public:
        TileMapping() = default;

        TileMapping(TileMapping&&);
        TileMapping& operator=(TileMapping&&);

        using PlacementRange = boost::iterator_range<const TilePlacement*>;

        PlacementRange find_tile(components::TileId tile_id, 
                                 const sf::Texture* texture_hint = nullptr) const;

        const sf::Texture* create_texture_from_image(const sf::Image& image, sf::IntRect rect = sf::IntRect());

        void define_tile_placement(components::TileId, const sf::Texture* texture, 
            core::IntRect source_rect, core::IntRect texture_rect);

        void define_tile_fragment(components::TileId, const sf::Texture* texture, 
            core::IntRect source_rect, core::IntRect texture_rect);

    private:
        std::vector<TilePlacement> tile_placement_;
        std::vector<TilePlacement> tile_fragments_;
        std::vector<std::unique_ptr<sf::Texture>> textures_;
    };
}

#endif