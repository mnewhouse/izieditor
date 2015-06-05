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

#include <cmath>

NAMESPACE_INTERFACE_MODES

StartPointsMode::StartPointsMode(EditorCanvas* canvas)
: ModeBase(canvas)
{
}

void StartPointsMode::on_activate()
{
    reload_start_points();
}

void StartPointsMode::reload_start_points()
{
    const auto& track = scene()->track();
    const auto& control_points = scene()->track().control_points();
    start_points_ = scene()->track().start_points();

    if (start_points_.empty() && !control_points.empty())
    {
        components::generate_default_start_points(control_points.front(), track.start_direction(), 20, std::back_inserter(start_points_));
    }
}

void StartPointsMode::render(sf::RenderTarget& render_target, sf::RenderStates render_states,
    const graphics::FontBitmap& font_bitmap)
{
    const auto& track = scene()->track();
    float inverse_zoom = 1.0 / canvas()->zoom_level();

    sf::RectangleShape line_shape;
    line_shape.setSize(sf::Vector2f(inverse_zoom * 24.0f, inverse_zoom));

    const float point_radius = 2.0f * inverse_zoom;
    sf::CircleShape point_shape(point_radius);    

    const sf::FloatRect point_bounds = point_shape.getLocalBounds();
    point_shape.setOrigin(point_bounds.width * 0.5f, point_bounds.height * 0.5f);

    std::vector<sf::Vertex> vertices;

    for (std::size_t index = 0; index != start_points_.size(); ++index)
    {
        const auto& point = start_points_[index];

        line_shape.setRotation(static_cast<float>(point.rotation));
        line_shape.setPosition(point.position.x, point.position.y);
        point_shape.setPosition(point.position.x, point.position.y);

        sf::Color color(0, 255, 255);

        if (hovered_start_point_index_ && index == *hovered_start_point_index_) color = sf::Color(255, 255, 0);
        if (selected_start_point_index_ && index == *selected_start_point_index_) color = sf::Color(255, 255, 255);

        point_shape.setFillColor(color);
        line_shape.setFillColor(color);

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
    return EditorTool::Placement | EditorTool::TileSelection;
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
        auto rotation = core::Rotation<double>::radians(radians);

        using components::StartPoint;
        StartPoint point{};
        point.position = position;
        point.rotation = static_cast<std::int32_t>(std::round(rotation.degrees()));

        if (fix_rotation)
        {
            double degrees = std::round(rotation.degrees() / 22.5) * 22.5;
            point.rotation = static_cast<std::int32_t>(std::round(degrees));
        }

        auto index = selected_start_point_index_;
        auto command = [=]()
        {
            if (!index) scene()->append_start_point(point);
            else scene()->insert_start_point(*index, point);
        };

        auto undo_command = [=]()
        {
            if (!index) scene()->delete_last_start_point();
            else scene()->delete_start_point(*index);
        };

        command();
        canvas()->perform_action("Add start point", command, undo_command);
    }
}

void StartPointsMode::delete_selected_start_point()
{
    const auto& start_points = scene()->track().start_points();
    if (selected_start_point_index_ && *selected_start_point_index_ < start_points.size())
    {
        std::size_t index = *selected_start_point_index_;
        auto point = start_points[index];

        auto command = [=]()
        {
            scene()->delete_start_point(index);
            reload_start_points();
        };

        auto undo_command = [=]()
        {
            scene()->insert_start_point(index, point);
            reload_start_points();
        };

        command();
        canvas()->perform_action("Remove start point", command, undo_command);
    }

    deselect_start_point();
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
            reload_start_points();
        };

        auto undo_command = [=]()
        {
            scene()->append_start_point(point);
            reload_start_points();
        };

        command();
        canvas()->perform_action("Remove start point", command, undo_command);
    }

    deselect_start_point();
}

void StartPointsMode::rotate_start_point_towards(std::size_t index, core::Vector2i target_point, bool fix_rotation)
{
    if (index < start_points_.size())
    {
        auto& point = start_points_[index];

        double radians = std::atan2(target_point.y - point.position.y, target_point.x - point.position.x);
        auto rotation = core::Rotation<double>::radians(radians);

        std::int32_t degrees = static_cast<std::int32_t>(std::round(rotation.degrees(core::rotation::absolute)));
        if (fix_rotation)
        {
            degrees = (degrees + 22) / 45 * 45 % 360;
        }

        point.rotation = degrees;

        update_start_point_info(index);
    }
}

void StartPointsMode::mouse_move_event(QMouseEvent* event, core::Vector2i track_position, core::Vector2i track_delta)
{
    auto tool = canvas()->active_tool();

    if (tool == EditorTool::TileSelection)
    {
        core::IntRect hover_rect(track_position.x - 4, track_position.y - 4, 8, 8);

        hovered_start_point_index_ = boost::none;
        for (std::size_t index = 0; index != start_points_.size(); ++index)
        {
            const auto& point = start_points_[index];
            if (core::contains(hover_rect, point.position))
            {
                hovered_start_point_index_ = index;

                if (!selected_start_point_index_)
                {
                    update_start_point_info(index);
                }
                break;
            }
        }
    }

    if ((event->buttons() & Qt::LeftButton) && selected_start_point_index_)
    {
        std::size_t index = *selected_start_point_index_;
        

        if (tool == EditorTool::Movement)
        {
            auto& point = start_points_[index];
            point.position += track_delta;

            update_start_point_info(index);
        }

        else if (tool == EditorTool::Rotation)
        {
            rotate_start_point_towards(index, track_position, (event->modifiers() & Qt::AltModifier) != 0);
        }
    }
}

void StartPointsMode::mouse_press_event(QMouseEvent* event)
{
    auto position = canvas()->mouse_position();
    auto tool = canvas()->active_tool();
    if (tool == EditorTool::Placement && event->button() == Qt::LeftButton)
    {
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

    else if (tool == EditorTool::TileSelection)
    {
        if (hovered_start_point_index_)
        {
            select_start_point(*hovered_start_point_index_);
        }

        else if (selected_start_point_index_)
        {
            deselect_start_point();
        }
    }

    else if (tool == EditorTool::Rotation)
    {
        if (selected_start_point_index_)
        {
            rotate_start_point_towards(*selected_start_point_index_, position, (event->modifiers() & Qt::AltModifier) != 0);
        }
    }
}

void StartPointsMode::mouse_release_event(QMouseEvent* event)
{
    auto tool = canvas()->active_tool();

    if (selected_start_point_index_)
    {
        std::size_t index = *selected_start_point_index_;
        const auto& start_points = scene()->track().start_points();

        if (tool == EditorTool::Movement)
        {
            if (index >= start_points.size() || start_points[index].position != start_points_[index].position)
            {
                commit_start_point_changes("Move start point");
            }
        }

        else if (tool == EditorTool::Rotation)
        {
            if (index >= start_points.size() || start_points[index].rotation != start_points_[index].rotation)
            {
                commit_start_point_changes("Rotate start point");
            }
        }
    }
}

void StartPointsMode::commit_start_point_changes(const std::string& description)
{
    const auto& new_state = start_points_;
    auto command = [=]()
    {
        scene()->update_start_points(new_state);
        reload_start_points();
    };

    const auto& old_state = scene()->track().start_points();
    auto undo_command = [=]()
    {
        scene()->update_start_points(old_state);
        reload_start_points();
    };

    command();
    canvas()->perform_action(description, command, undo_command);
}

void StartPointsMode::select_start_point(std::size_t index)
{
    selected_start_point_index_ = index;

    canvas()->enable_deleting(true);
    canvas()->enable_tool(EditorTool::Rotation, true);
    canvas()->enable_tool(EditorTool::Movement, true);

    update_start_point_info(index);
}

void StartPointsMode::deselect_start_point()
{
    canvas()->enable_deleting(false);
    canvas()->enable_tool(EditorTool::Rotation, false);
    canvas()->enable_tool(EditorTool::Movement, false);

    update_start_point_info();
}

void StartPointsMode::update_start_point_info()
{
    canvas()->display_tool_info("");
    canvas()->display_secondary_tool_info("");
}

void StartPointsMode::update_start_point_info(std::size_t index)
{
    if (index < start_points_.size())
    {
        auto point = start_points_[index];

        canvas()->display_tool_info("Point " + QString::number(index));

        QString text = QString::number(point.position.x) + ", " + QString::number(point.position.y);
        text += "; " + QString::number(point.rotation) + L'°';

        canvas()->display_secondary_tool_info(text);
    }

    else
    {
        update_start_point_info();
    }
}

NAMESPACE_INTERFACE_MODES_END