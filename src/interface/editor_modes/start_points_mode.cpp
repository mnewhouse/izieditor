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

#include "start_points_mode.hpp"

#include "../editor_canvas.hpp"

#include "components/track.hpp"
#include "components/component_algorithms.hpp"

#include "scene/scene.hpp"

#include "graphics/font_bitmap.hpp"

NAMESPACE_INTERFACE_MODES

StartPointsMode::StartPointsMode(EditorCanvas* canvas)
: ModeBase(canvas)
{
}

void StartPointsMode::on_activate()
{
    canvas()->set_active_tool(EditorTool::Placement);
}

void StartPointsMode::render(sf::RenderTarget& render_target, sf::RenderStates render_states,
    const graphics::FontBitmap& font_bitmap)
{
    const auto& track = scene()->track();

    const auto& control_points = track.control_points();
    start_points_ = scene()->track().start_points();

    if (start_points_.empty() && !control_points.empty())
    {
        components::generate_default_start_points(control_points.front(), track.start_direction(), 20, std::back_inserter(start_points_));
    }

    float inverse_zoom = 1.0 / canvas()->zoom_level();

    sf::RectangleShape line_shape;
    line_shape.setSize(sf::Vector2f(inverse_zoom * 24.0f, inverse_zoom));
    line_shape.setFillColor(sf::Color(0, 255, 255));

    const float point_radius = 2.0f * inverse_zoom;
    sf::CircleShape point_shape(point_radius);
    point_shape.setFillColor(sf::Color(0, 255, 255));

    const sf::FloatRect point_bounds = point_shape.getLocalBounds();
    point_shape.setOrigin(point_bounds.width * 0.5f, point_bounds.height * 0.5f);

    std::vector<sf::Vertex> vertices;

    for (const auto& point : start_points_)
    {
        line_shape.setRotation(point.rotation.degrees());
        line_shape.setPosition(point.position.x, point.position.y);
        point_shape.setPosition(point.position.x, point.position.y);

        render_target.draw(line_shape, render_states);
        render_target.draw(point_shape, render_states);
    }

    sf::View track_view = render_target.getView();
    sf::FloatRect view_port = track_view.getViewport();
    sf::Transform view_transform = render_target.getView().getTransform();

    sf::Vector2u screen_size = render_target.getSize();
    float screen_width = static_cast<float>(screen_size.x);
    float screen_height = static_cast<float>(screen_size.y);

    vertex_cache_.clear();
    for (std::size_t index = 0; index != start_points_.size(); ++index)
    {
        const auto& point = start_points_[index];

        sf::Vector2f screen_position = view_transform.transformPoint(point.position.x, point.position.y);

        sf::Vector2f position =
        {
            (view_port.left + (screen_position.x + 1.0f) * 0.5f * view_port.width) * screen_width,
            (view_port.top - (screen_position.y - 1.0f) * 0.5f * view_port.height) * screen_height
        };

        position.x = std::floor(position.x) + 2.0f;
        position.y = std::floor(position.y) + 2.0f;

        graphics::generate_text_vertices(std::to_string(index + 1), font_bitmap,
            std::back_inserter(vertex_cache_), position, sf::Color(255, 255, 100));
    }

    sf::View replacement_view(sf::FloatRect(0.0f, 0.0f, screen_width, screen_height));
    render_target.setView(replacement_view);

    render_states = sf::RenderStates(&font_bitmap.texture());
    render_target.draw(vertex_cache_.data(), vertex_cache_.size(), sf::Quads, render_states);

    render_target.setView(track_view);
}

std::uint32_t StartPointsMode::enabled_tools() const
{
    return EditorTool::Placement | 0U;
}

void StartPointsMode::tool_changed(EditorTool tool)
{
    start_point_position_ = boost::none;
}


void StartPointsMode::place_start_point(core::Vector2i position, core::Vector2i face_towards, bool fix_rotation)
{
    const auto& start_points = scene()->track().start_points();
    if (start_points.size() < 20)
    {
        double x_offset = face_towards.x - static_cast<double>(position.x);
        double y_offset = face_towards.y - static_cast<double>(position.y);

        double radians = std::atan2(y_offset, x_offset);

        using components::StartPoint;
        StartPoint point{};
        point.position = position;
        point.rotation = core::Rotation<double>::radians(radians);

        if (fix_rotation)
        {
            double degrees = std::round(point.rotation.degrees() / 22.5) * 22.5;
            point.rotation = core::Rotation<double>::degrees(degrees);
        }

        auto command = [=]()
        {
            scene()->append_start_point(point);
        };

        auto undo_command = [=]()
        {
            scene()->delete_last_start_point();
        };

        command();
        canvas()->perform_action("Add start point", command, undo_command);
    }
}

void StartPointsMode::delete_last_start_point()
{
    const auto& start_points = scene()->track().start_points();
    if (!start_points.empty())
    {
        auto point = start_points.back();

        auto command = [=]()
        {
            scene()->delete_last_start_point();
        };

        auto undo_command = [=]()
        {
            scene()->append_start_point(point);
        };

        command();
        canvas()->perform_action("Remove start point", command, undo_command);
    }
}

void StartPointsMode::mouse_press_event(QMouseEvent* event)
{
    auto tool = canvas()->active_tool();
    if (tool == EditorTool::Placement && event->button() == Qt::LeftButton)
    {
        auto position = canvas()->mouse_position();

        if (start_point_position_)
        {
            bool fix_rotation = (event->modifiers() & Qt::AltModifier) == 0;
            place_start_point(*start_point_position_, position, fix_rotation);

            start_point_position_ = boost::none;
        }

        else
        {
            start_point_position_ = position;
        }
    }
}

NAMESPACE_INTERFACE_MODES_END