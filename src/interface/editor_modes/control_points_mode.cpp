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

#include "control_points_mode.hpp"

#include "components/track.hpp"
#include "components/control_point.hpp"

#include "scene/scene.hpp"

#include "graphics/font_bitmap.hpp"

#include "../editor_canvas.hpp"

NAMESPACE_INTERFACE_MODES

ControlPointsMode::ControlPointsMode(EditorCanvas* canvas)
  : ModeBase(canvas)
{
}

void ControlPointsMode::render(sf::RenderTarget& render_target, sf::RenderStates render_states,
    const graphics::FontBitmap& font_bitmap)
{
    sf::RectangleShape shape;
    shape.setFillColor(sf::Color(0, 255, 255));

    float inverse_zoom = 1.0f / canvas()->zoom_level();

    const auto& control_points = scene()->track().control_points();
    for (const auto& point : control_points)
    {
        shape.setPosition(point.start.x, point.start.y);

        sf::Vector2f size(point.length, inverse_zoom);
        if (point.direction == components::ControlPoint::Vertical) std::swap(size.x, size.y);
        shape.setSize(size);

        render_target.draw(shape);
    }

    sf::View track_view = render_target.getView();
    sf::FloatRect view_port = track_view.getViewport();
    sf::Transform view_transform = render_target.getView().getTransform();

    sf::Vector2u screen_size = render_target.getSize();
    float screen_width = static_cast<float>(screen_size.x);
    float screen_height = static_cast<float>(screen_size.y);

    vertex_cache_.clear();
    for (std::size_t index = 0; index != control_points.size(); ++index)
    {
        const auto& point = control_points[index];

        sf::Vector2f screen_position = view_transform.transformPoint(point.start.x, point.start.y);

        sf::Vector2f position =
        {
            (view_port.left + (screen_position.x + 1.0f) * 0.5f * view_port.width) * screen_width,
            (view_port.top - (screen_position.y - 1.0f) * 0.5f * view_port.height) * screen_height
        };

        position.x = std::floor(position.x) + 2.0f;
        position.y = std::floor(position.y) + 2.0f;

        std::string text = index == 0 ? "Finish" : std::to_string(index);
        graphics::generate_text_vertices(text, font_bitmap,
            std::back_inserter(vertex_cache_), position, sf::Color(255, 255, 100));
    }

    sf::View replacement_view(sf::FloatRect(0.0f, 0.0f, screen_width, screen_height));
    render_target.setView(replacement_view);

    render_states = sf::RenderStates(&font_bitmap.texture());
    render_target.draw(vertex_cache_.data(), vertex_cache_.size(), sf::Quads, render_states);

    render_target.setView(track_view);
}

void ControlPointsMode::on_activate()
{
    canvas()->set_active_tool(EditorTool::Placement);
}

void ControlPointsMode::tool_changed(EditorTool tool)
{
    control_point_start_ = boost::none;
}

std::uint32_t ControlPointsMode::enabled_tools() const
{
    return EditorTool::Placement | 0U;
}

void ControlPointsMode::mouse_press_event(QMouseEvent* event)
{
    if (canvas()->active_tool() == EditorTool::Placement && event->button() == Qt::LeftButton)
    {
        if (control_point_start_)
        {
            place_control_point(*control_point_start_, canvas()->mouse_position());
            control_point_start_ = boost::none;
        }

        else
        {
            control_point_start_ = canvas()->mouse_position();
        }
    }
}

void ControlPointsMode::place_control_point(core::Vector2i start, core::Vector2i end)
{
    using components::ControlPoint;

    if (start != end)
    {

        std::int32_t x_diff = end.x - start.x;
        std::int32_t y_diff = end.y - start.y;

        std::int32_t x_length = std::abs(x_diff);
        std::int32_t y_length = std::abs(y_diff);

        ControlPoint point{};
        point.start = start;

        if (x_length > y_length)
        {
            point.direction = ControlPoint::Horizontal;
            point.length = x_length;
            if (x_diff < 0) point.start.x = end.x;
        }

        else
        {
            point.direction = ControlPoint::Vertical;
            point.length = y_length;
            if (y_diff < 0) point.start.y = end.y;
        }

        auto command = [=]()
        {
            scene()->append_control_point(point);
        };

        auto undo_command = [=]()
        {
            scene()->delete_last_control_point();
        };

        command();
        canvas()->perform_action("Add control point", command, undo_command);
    }
}

void ControlPointsMode::delete_last_control_point()
{
    const auto& control_points = scene()->track().control_points();
    if (!control_points.empty())
    {
        auto last_point = control_points.back();

        auto command = [=]()
        {
            scene()->delete_last_control_point();
        };

        auto undo_command = [=]()
        {
            scene()->append_control_point(last_point);
        };

        control_point_start_ = boost::none;

        command();
        canvas()->perform_action("Remove control point", command, undo_command);
    }
}

NAMESPACE_INTERFACE_MODES_END