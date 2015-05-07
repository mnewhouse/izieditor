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

#include "tile_partitioner.hpp"
#include "tile_mapping.hpp"

#include "components/tile_library.hpp"

#include "graphics/image_loader.hpp"

#include "core/rect.hpp"
#include "core/vector2.hpp"

#include <SFML/Graphics.hpp>

#include <unordered_map>
#include <cstdint>
#include <string>
#include <algorithm>
#include <numeric>
#include <cassert>

using core::IntRect;
using core::Vector2i;

using components::TileLibrary;
using components::TileDefinition;
using components::TileGroupDefinition;

namespace scene
{
    namespace impl
    {
        struct PartitionRow
        {
            IntRect area;
            std::int32_t free_space = 0;
        };

        struct TileFragment
        {
            IntRect source_rect;
            IntRect target_rect;
            IntRect full_source_rect;
        };

        struct Partition
        {
            static const std::int32_t texture_size;
            std::int32_t row_start = 0;

            std::vector<PartitionRow> partition_rows;
            std::unordered_map<std::string, std::vector<TileFragment>> tile_placement;

            bool allocate_tile_space(const TileDefinition& tile_def, IntRect allocation_rect);
            bool has_image_rect(const std::string& image, IntRect rect) const;

            std::pair<bool, IntRect> find_image_rect(const std::string& image, IntRect rect) const;
            std::pair<bool, IntRect> find_fragment(const std::string& image, IntRect rect) const;

            IntRect allocate_space(Vector2i size);

        private:
            
            //void deallocate_space(IntRect bounding_box);

            template <typename ForwardIt>
            ForwardIt best_matching_row(ForwardIt it, ForwardIt end, Vector2i size);
        };

        const std::int32_t Partition::texture_size = std::min(sf::Texture::getMaximumSize(), 2048U);

        struct PartitionManager
        {
            std::list<Partition> partitions;

            void allocate_fragmented_tile_space(const TileDefinition& tile_def, IntRect rect);
            Partition* create_partition();

            bool has_image_rect(const std::string& image, IntRect rect) const;
        };

        using ImageRectMap = std::unordered_map<std::string, std::vector<IntRect>>;
        ImageRectMap compute_image_rects_no_overlap(const TileLibrary& tile_lib);

        const IntRect* find_enclosing_rect(const ImageRectMap& image_rect_map,
            const std::string& image_file, IntRect rect);       
    }
}

template <typename ForwardIt>
ForwardIt scene::impl::Partition::best_matching_row(ForwardIt it, ForwardIt end, Vector2i size)
{
    auto condition = [size](const PartitionRow& row)
    {
        return row.free_space > size.x && size.y < row.area.height - 1;
    };

    auto best_match = it = std::find_if(it, end, condition);
    while (it != end)
    {
        if (it->area.height - size.y < best_match->area.height - size.y)
        {
            best_match = it;
        }

        it = std::find_if(std::next(it), end, condition);
    }

    return best_match;
}


core::IntRect scene::impl::Partition::allocate_space(Vector2i size)
{
    if (size.x > texture_size || size.y > texture_size)
    {
        return IntRect();
    }

    // Find the best matching row
    auto best_row = best_matching_row(partition_rows.begin(), partition_rows.end(), size);

    // If there is no matching row or the best matching row is a bad match, attempt to create a new row.
    // However, if there is no space to create a new row, use the best matching row anyway, even if it's bad.

    if (best_row == partition_rows.end() || size.y * 10 < best_row->area.height * 7)
    {
        if (row_start + size.y < texture_size)
        {
            partition_rows.emplace_back();

            best_row = std::prev(partition_rows.end());
            best_row->area.top = row_start;
            best_row->area.height = size.y + 2;
            best_row->area.left = 0;
            best_row->area.width = texture_size;
            best_row->free_space = best_row->area.width;

            row_start += best_row->area.height;
        }
    }

    if (best_row == partition_rows.end())
    {
        return IntRect();
    }
    Vector2i tile_position(best_row->area.right() - best_row->free_space, best_row->area.top);

    best_row->free_space -= size.x + 2;

    return IntRect(tile_position.x, tile_position.y, size.x, size.y);
}

/*
void scene::impl::Partition::deallocate_space(IntRect area)
{
    auto row_it = std::find_if(partition_rows.begin(), partition_rows.end(),
        [area](const PartitionRow& row)
    {
        return row.area.top == area.top && area.left >= row.area.left && area.right() <= row.area.right();
    });

    if (row_it != partition_rows.end())
    {
        PartitionRow remainder;
        remainder.area.left = area.right();
        remainder.area.width = row_it->area.right() - remainder.area.left;
        remainder.area.top = row_it->area.top;
        remainder.area.height = row_it->area.height;
        remainder.free_space = row_it->free_space;

        row_it->free_space = area.width;
        row_it->area.width -= remainder.area.width;

        partition_rows.push_back(remainder);
    }
}
*/

// Compute all the image rects *without* overlapping
scene::impl::ImageRectMap scene::impl::compute_image_rects_no_overlap(const TileLibrary& tile_library)
{
    ImageRectMap result;
    
    for (const TileDefinition* tile_def = tile_library.first_tile(); tile_def != nullptr;
        tile_def = tile_library.next_tile(tile_def->id))
    {
        const std::string& image_file = tile_def->image_file();
        IntRect image_rect = tile_def->image_rect;

        auto& rect_list = result[image_file];

        auto no_intersection = [&image_rect](const IntRect& rect)
        {
            return !intersects(rect, image_rect);
        };

        // Put the intersecting rects at the back
        auto partition_it = std::partition(rect_list.begin(), rect_list.end(), no_intersection);
        while (partition_it != rect_list.end())
        {
            image_rect = std::accumulate(partition_it, rect_list.end(), image_rect,
                [](const IntRect& a, const IntRect& b)
            {
                return combine(a, b);
            });

            rect_list.erase(partition_it, rect_list.end());
            partition_it = std::partition(rect_list.begin(), rect_list.end(), no_intersection);
        }

        // Then, combine them all, erase the previous ones and insert the combination.
        rect_list.push_back(image_rect);
    }

    return result;
}

const core::IntRect* scene::impl::find_enclosing_rect(const ImageRectMap& image_rect_map, 
    const std::string& file_name, IntRect needle)
{
    auto map_it = image_rect_map.find(file_name);
    if (map_it == image_rect_map.end()) return nullptr;

    const auto& rect_list = map_it->second;
    auto list_it = std::find_if(rect_list.begin(), rect_list.end(), 
        [needle](const IntRect& rect)
    {
        return intersection(rect, needle) == needle;
    });

    if (list_it == rect_list.end())
    {
        return nullptr;
    }

    return &*list_it;
}

bool scene::impl::Partition::has_image_rect(const std::string& image_file, IntRect image_rect) const
{
    auto map_it = tile_placement.find(image_file);
    if (map_it == tile_placement.end())
    {
        return false;
    }

    const auto& placement_list = map_it->second;
    auto search_result = std::find_if(placement_list.begin(), placement_list.end(), 
        [image_rect](const TileFragment& fragment)
    {
        return intersection(image_rect, fragment.source_rect) == image_rect;
    });

    return search_result != placement_list.end();
}

std::pair<bool, core::IntRect> scene::impl::Partition::find_image_rect(const std::string& image_file, IntRect image_rect) const
{
    auto map_it = tile_placement.find(image_file);
    if (map_it != tile_placement.end())
    {
        const auto& placement_list = map_it->second;
        auto search_result = std::find_if(placement_list.begin(), placement_list.end(),
            [image_rect](const TileFragment& fragment)
        {
            return intersection(image_rect, fragment.source_rect) == image_rect;
        });

        if (search_result != placement_list.end())
        {
            IntRect dest_rect = search_result->target_rect;
            dest_rect.left += image_rect.left - search_result->source_rect.left;
            dest_rect.top += image_rect.top - search_result->source_rect.top;
            dest_rect.width = image_rect.width;
            dest_rect.height = image_rect.height;

            return std::make_pair(true, dest_rect);
        }
    }

    return std::make_pair(false, IntRect());
}

bool scene::impl::Partition::allocate_tile_space(const TileDefinition& tile_def, IntRect rect)
{
    auto result = allocate_space({ rect.width, rect.height });
    if (result.width == rect.width && result.height == rect.height)
    {
        auto& placement_list = tile_placement[tile_def.image_file()];
        placement_list.emplace_back();
        placement_list.back().source_rect = rect;
        placement_list.back().full_source_rect = rect;
        placement_list.back().target_rect = result;        

        return true;
    }

    return false;    
}

scene::impl::Partition* scene::impl::PartitionManager::create_partition()
{
    partitions.emplace_back();
    return &partitions.back();
}

bool scene::impl::PartitionManager::has_image_rect(const std::string& file_name, IntRect rect) const
{
    auto search_result = std::find_if(partitions.begin(), partitions.end(),
        [&](const Partition& partition)
    {
        return partition.has_image_rect(file_name, rect);
    });

    return search_result != partitions.end();
}


void scene::impl::PartitionManager::allocate_fragmented_tile_space(const TileDefinition& tile_def, IntRect rect)
{
    const auto texture_size = Partition::texture_size;

    TileFragment fragment;
    fragment.full_source_rect = rect;

    for (std::int32_t y = rect.top, bottom = rect.bottom(); y < bottom; y += texture_size)
    {
        for (std::int32_t x = rect.left, right = rect.right(); x < right; x += texture_size)
        {
            IntRect sub_rect(x, y, texture_size, texture_size);
            fragment.source_rect = intersection(rect, sub_rect);

            auto partition = create_partition();            
            fragment.target_rect = partition->allocate_space({ fragment.source_rect.width, fragment.source_rect.height });

            auto& fragment_list = partition->tile_placement[tile_def.image_file()];
            fragment_list.push_back(fragment);
        }
    }
}

scene::TileMapping scene::create_tile_mapping(const TileLibrary& tile_library)
{
    return create_tile_mapping(tile_library, {}, nullptr);
}

scene::TileMapping scene::create_tile_mapping(const TileLibrary& tile_library,
    graphics::ImageLoader image_loader, std::function<void(double)> update_progress)
{
    impl::ImageRectMap image_rect_map = impl::compute_image_rects_no_overlap(tile_library);

    impl::PartitionManager partition_manager;
    auto current_partition = partition_manager.create_partition();

    // Need to make sure that the sequence of tiles can be rendered with a low amount of components    

    // For all tile groups in the library,
    // Attempt to place all of their subtiles in the current partition.

    for (auto tile_group = tile_library.first_tile_group(); tile_group != nullptr; 
        tile_group = tile_library.next_tile_group(tile_group->id()))
    {
        std::size_t group_size = tile_group->sub_tiles().size();

        for (const auto& sub_tile : tile_group->sub_tiles())
        {
            const TileDefinition* tile_def = tile_library.tile(sub_tile.id);
            
            if (!tile_def) continue;

            const std::string& image_file = tile_def->image_file();
            IntRect image_rect = tile_def->image_rect;

            const IntRect* enclosing_rect = impl::find_enclosing_rect(image_rect_map, image_file, image_rect);

            // All enclosing rects should be accounted for.
            assert(enclosing_rect != nullptr);

            // If it's a singular tile, or if it's a big tile, test all partitions for its presence.
            if (group_size == 1 || enclosing_rect->width > 256 || enclosing_rect->height > 256)
            {
                if (partition_manager.has_image_rect(image_file, image_rect)) continue;
            }

            // It's a small tile in a group - only test the current partition.
            else if (current_partition->has_image_rect(image_file, image_rect))
            {
                continue;
            }
            

            bool success = current_partition->allocate_tile_space(*tile_def, *enclosing_rect);
            if (!success)
            {
                // Failed to allocate space
                current_partition = partition_manager.create_partition();

                success = current_partition->allocate_tile_space(*tile_def, *enclosing_rect);
                if (!success)
                {
                    // Even failed to allocate space in clean partition.
                    partition_manager.allocate_fragmented_tile_space(*tile_def, *enclosing_rect);
                }
            }
        }
    }

    std::unordered_map<std::string, std::vector<const TileDefinition*>> tiles_by_image;
    for (auto tile = tile_library.first_tile(); tile != nullptr; tile = tile_library.next_tile(tile->id))
    {
        const auto& file = tile->image_file();
        tiles_by_image[file].push_back(tile);
    }

    TileMapping tile_mapping;

    std::size_t i = 0;
    for (const auto& partition : partition_manager.partitions)
    {
        sf::Image dest_image;
        dest_image.create(partition.texture_size, partition.texture_size, sf::Color::Transparent);

        for (const auto& image_info : partition.tile_placement)
        {
            const auto& source_image = image_loader.load_from_file(image_info.first);
            for (const auto& placement : image_info.second)
            {
                auto source_rect = placement.source_rect;
                auto dest_rect = placement.target_rect;
                dest_image.copy(source_image, dest_rect.left, dest_rect.top,
                    sf::IntRect(source_rect.left, source_rect.top, source_rect.width, source_rect.height));
            }
        }

        auto texture = tile_mapping.create_texture_from_image(dest_image);
        // Need to get the tile ids that are contained in the tile_placement.

        for (const auto& image_info : partition.tile_placement)
        {
            auto map_it = tiles_by_image.find(image_info.first);
            if (map_it == tiles_by_image.end()) continue;

            auto& tile_list = map_it->second;

            for (const auto& fragment : image_info.second)
            {
                // Need to find the tiles that match this fragment.
                auto source_rect = fragment.full_source_rect;

                auto fully_contained = [=](const TileDefinition* tile_def)
                {
                    return intersection(tile_def->image_rect, source_rect) == tile_def->image_rect;
                };

                auto end = std::partition(tile_list.begin(), tile_list.end(), fully_contained);
                for (auto it = tile_list.begin(); it != end; ++it)
                {
                    const auto* tile_def = *it;
                    auto tile_rect = intersection(tile_def->image_rect, fragment.source_rect);

                    auto texture_rect = fragment.target_rect;
                    texture_rect.width = tile_rect.width;
                    texture_rect.height = tile_rect.height;
                    texture_rect.left += tile_def->image_rect.left - source_rect.left;
                    texture_rect.top += tile_def->image_rect.top - source_rect.top;

                    if (fragment.source_rect != fragment.full_source_rect)
                    {
                        tile_rect.left = fragment.source_rect.left - fragment.full_source_rect.left;
                        tile_rect.top = fragment.source_rect.top - fragment.full_source_rect.top;

                        tile_mapping.define_tile_fragment(tile_def->id, texture, tile_rect, texture_rect);
                    }

                    else
                    {
                        tile_rect.left = 0;
                        tile_rect.top = 0;
                        tile_mapping.define_tile_placement(tile_def->id, texture, tile_rect, texture_rect);
                    }
                }
            }
        }
    }

    return tile_mapping;
}