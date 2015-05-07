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

#ifndef START_POINTS_MODE_HPP
#define START_POINTS_MODE_HPP

#include "mode_base.hpp"

#include "components/start_point.hpp"

#include "core/vector2.hpp"

#include <SFML/Graphics.hpp>

#include <qevent.h>

#include <boost/optional.hpp>

#include <vector>


namespace graphics
{
    class FontBitmap;
}

NAMESPACE_INTERFACE_MODES

struct StartPointsMode
    : ModeBase
{
    StartPointsMode(EditorCanvas* canvas);

    void render(sf::RenderTarget& render_target, sf::RenderStates render_states,
        const graphics::FontBitmap& font_bitmap);

    void mouse_press_event(QMouseEvent* event);
    void place_start_point(core::Vector2i position, core::Vector2i face_towards, bool fix_rotation);
    void delete_last_start_point();

    virtual void tool_changed(EditorTool tool) override;

private:
    virtual std::uint32_t enabled_tools() const override;
    virtual void on_activate() override;

    boost::optional<core::Vector2i> start_point_position_;
    std::vector<components::StartPoint> start_points_;
    std::vector<sf::Vertex> vertex_cache_;
};        

NAMESPACE_INTERFACE_MODES_END

#endif