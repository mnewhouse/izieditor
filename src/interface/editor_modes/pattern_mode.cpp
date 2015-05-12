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

#include "pattern_mode.hpp"

#include "components/pattern_builder.hpp"
#include "components/terrain_library.hpp"

#include "scene/scene.hpp"

NAMESPACE_INTERFACE_MODES


PatternMode::PatternMode(EditorCanvas* canvas)
: ModeBase(canvas)
{
}

void PatternMode::render(sf::RenderTarget& render_target, sf::RenderStates render_states)
{
    poll_loading_result();

    for (const auto& sub_texture : sub_textures_)
    {
        sf::IntRect sub_rect = sub_texture.sub_rect;
        sf::Vector2f position(sub_rect.left, sub_rect.top);
        sub_rect.left = 0;
        sub_rect.top = 0;

        sf::Sprite sprite(sub_texture.texture, sub_rect);
        sprite.setPosition(position);

        render_target.draw(sprite, render_states);
    }
}

void PatternMode::on_activate()
{
    sub_textures_.clear();

    initiate_pattern_building();
}

void PatternMode::poll_loading_result()
{
    if (loading_future_.valid() && loading_future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        sub_textures_ = loading_future_.get();
    }
}

void PatternMode::initiate_pattern_building()
{
    auto loading_func = [=]()
    {
        components::PatternBuilder pattern_builder(scene()->track(), scene()->pattern_store());
        auto pattern = pattern_builder();
        auto pattern_size = pattern.size();

        const auto& terrain_library = scene()->track().terrain_library();

        sf::Image image;
        image.create(pattern_size.x, pattern_size.y);

        for (std::uint32_t y = 0; y != pattern_size.y; ++y)
        {
            for (std::uint32_t x = 0; x != pattern_size.x; ++x)
            {
                const auto& terrain = terrain_library.terrain_by_id(pattern(x, y));

                sf::Color color(terrain.red, terrain.green, terrain.blue);
                image.setPixel(x, y, color);
            }
        }

        std::list<SubTexture> result;
        const std::uint32_t texture_size = std::min(sf::Texture::getMaximumSize(), 2048U);

        for (std::uint32_t y = 0; y < pattern_size.y; y += texture_size)
        {
            for (std::uint32_t x = 0; x < pattern_size.x; x += texture_size)
            {
                result.emplace_back();

                auto& sub_texture = result.back();
                sub_texture.sub_rect.left = x;
                sub_texture.sub_rect.top = y;
                sub_texture.sub_rect.width = std::min(x + texture_size, pattern_size.x) - x;
                sub_texture.sub_rect.height = std::min(y + texture_size, pattern_size.y) - y;

                sub_texture.texture.loadFromImage(image, sub_texture.sub_rect);
            }
        }

        return result;
    };

    loading_future_ = std::async(std::launch::async, loading_func);
}

std::uint32_t PatternMode::enabled_tools() const
{
    return 0;
}

NAMESPACE_INTERFACE_MODES_END