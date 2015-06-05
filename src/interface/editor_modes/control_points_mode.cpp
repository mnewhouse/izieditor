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
    float inverse_zoom = 1.0f / canvas()->zoom_level();

    const auto& control_points = scene()->track().control_points();
    for (std::size_t index = 0; index != control_points.size(); ++index)
    {
        const auto& point = control_points[index];
        shape.setPosition(point.start.x, point.start.y);

        sf::Vector2f size(point.length, inverse_zoom);
        if (point.direction == components::ControlPoint::Vertical) std::swap(size.x, size.y);
        shape.setSize(size);

        sf::Color color(0, 255, 255);
        if (hovered_control_point_index_ && *hovered_control_point_index_ == index) color = sf::Color(255, 255, 0);
        if (selected_control_point_index_ && *selected_control_point_index_ == index) color = sf::Color(255, 255, 255);

        shape.setFillColor(color);

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
}

void ControlPointsMode::tool_changed(EditorTool tool)
{
    control_point_start_ = boost::none;
}

std::uint32_t ControlPointsMode::enabled_tools() const
{
    return EditorTool::Placement | EditorTool::TileSelection;
}

void ControlPointsMode::mouse_press_event(QMouseEvent* event)
{
    auto tool = canvas()->active_tool();
    if (tool == EditorTool::Placement && event->button() == Qt::LeftButton)
    {
        if (control_point_start_)
        {
            canvas()->releaseMouse();

            place_control_point(*control_point_start_, canvas()->mouse_position());
            control_point_start_ = boost::none;
        }

        else
        {
            canvas()->grabMouse();

            control_point_start_ = canvas()->mouse_position();
        }
    }

    else if (tool == EditorTool::TileSelection && event->button() == Qt::LeftButton)
    {
        if (hovered_control_point_index_)
        {
            select_control_point(*hovered_control_point_index_);
        }

        else if (selected_control_point_index_)
        {
            deselect_control_point();
        }
    }

    else if (tool == EditorTool::Rotation && event->button() == Qt::LeftButton && selected_control_point_index_)
    {
        std::size_t index = *selected_control_point_index_;
        auto command = [=]()
        {
            scene()->rotate_control_point(index);
            update_control_point_info(index);
        };

        command();

        // Heu, we can use the same command for undo as well
        canvas()->perform_action("Rotate control point", command, command);
    }

    else if ((tool == EditorTool::Movement || tool == EditorTool::Resize) &&
        event->button() == Qt::LeftButton && selected_control_point_index_)
    {
        const auto& control_points = scene()->track().control_points();
        old_point_state_ = control_points[*selected_control_point_index_];
    }
}

void ControlPointsMode::mouse_release_event(QMouseEvent* event)
{
    auto tool = canvas()->active_tool();
    if (selected_control_point_index_)
    {
        if (tool == EditorTool::Movement && (control_point_offset_.x != 0 || control_point_offset_.y != 0))
        {
            commit_control_point_movement(*selected_control_point_index_, control_point_offset_);
            control_point_offset_ = {};
        }

        else if (tool == EditorTool::Resize && (control_point_offset_.x != 0 || control_point_offset_.y != 0))
        {
            commit_control_point_resize(*selected_control_point_index_, control_point_offset_);
            control_point_offset_ = {};
        }
    }
}

void ControlPointsMode::mouse_move_event(QMouseEvent* event, core::Vector2i track_point, core::Vector2i track_delta)
{
    hovered_control_point_index_ = boost::none;

    auto tool = canvas()->active_tool();
    if (tool == EditorTool::TileSelection)
    {
        const auto& control_points = scene()->track().control_points();
        for (std::size_t index = 0; index != control_points.size(); ++index)
        {
            const auto& point = control_points[index];

            core::Vector2i start = point.start;
            core::Vector2i size(point.length, 8);

            if (point.direction == components::ControlPoint::Horizontal)
            {
                start.y -= 4;
            }

            else
            {
                start.x -= 4;

                std::swap(size.x, size.y);
            }

            core::IntRect rect(start, size);
            if (core::contains(rect, track_point))
            {
                hovered_control_point_index_ = index;
                if (!selected_control_point_index_)
                {
                    update_control_point_info(index);
                }

                break;
            }
        }
    }
    
    if ((event->buttons() & Qt::LeftButton) && selected_control_point_index_)
    {
        if (tool == EditorTool::Movement)
        {
            control_point_offset_ += track_delta;

            std::size_t index = *selected_control_point_index_;
            scene()->move_control_point(index, track_delta);
            update_control_point_info(index);
        }

        else if (tool == EditorTool::Resize)
        {
            control_point_offset_ += track_delta;

            resize_control_point(*selected_control_point_index_, control_point_offset_);
        }
    }
}

void ControlPointsMode::select_control_point(std::size_t index)
{
    selected_control_point_index_ = index;

    canvas()->enable_tool(EditorTool::Movement, true);
    canvas()->enable_tool(EditorTool::Resize, true);
    canvas()->enable_tool(EditorTool::Rotation, true);

    canvas()->enable_deleting(true);

    const auto& control_points = scene()->track().control_points();
    if (index < control_points.size())
    {
        old_point_state_ = control_points[index];
    }

    update_control_point_info(index);
}

void ControlPointsMode::deselect_control_point()
{
    selected_control_point_index_ = boost::none;

    canvas()->enable_tool(EditorTool::Movement, false);
    canvas()->enable_tool(EditorTool::Resize, false);
    canvas()->enable_tool(EditorTool::Rotation, false);

    canvas()->enable_deleting(false);

    update_control_point_info();
}

void ControlPointsMode::place_control_point(core::Vector2i start, core::Vector2i end)
{
    using components::ControlPoint;

    if (start != end)
    {
        std::int32_t x_diff = end.x - start.x;
        std::int32_t y_diff = end.y - start.y;

        std::int32_t x_length = std::abs(x_diff) + 1;
        std::int32_t y_length = std::abs(y_diff) + 1;

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

        auto index = selected_control_point_index_;
        auto command = [=]()
        {
            if (!index) scene()->append_control_point(point);
            else
            {
                scene()->insert_control_point(*index, point);
                select_control_point(*index + 1);
            }
        };

        auto undo_command = [=]()
        {
            if (!index) scene()->delete_last_control_point();
            else scene()->delete_control_point(*index);
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

    deselect_control_point();
}

void ControlPointsMode::commit_control_point_movement(std::size_t index, core::Vector2i offset)
{
    const auto& control_points = scene()->track().control_points();
    if (index < control_points.size())
    {
        auto point = control_points[index], old_point = old_point_state_;

        auto command = [=]()
        {
            scene()->update_control_point(index, point);
        };

        auto undo_command = [=]()
        {
            scene()->update_control_point(index, old_point);
        };

        command();
        canvas()->perform_action("Move control point", command, undo_command);
    }
}

void ControlPointsMode::resize_control_point(std::size_t index, core::Vector2i offset)
{
    auto point = old_point_state_;
    using components::ControlPoint;

    if (point.direction == ControlPoint::Horizontal)
    {
        point.length += offset.x;
        if (point.length < 0)
        {
            point.length = std::abs(point.length);
            point.start.x -= point.length;
        }
    }

    else
    {
        point.length += offset.y;
        if (point.length < 0)
        {
            point.length = std::abs(point.length);
            point.start.y -= point.length;
        }
    }

    scene()->update_control_point(index, point);
    update_control_point_info(index);
}

void ControlPointsMode::commit_control_point_resize(std::size_t index, core::Vector2i offset)
{
    const auto& control_points = scene()->track().control_points();
    if (index < control_points.size())
    {
        auto point = control_points[index], old_point = old_point_state_;
        
        auto command = [=]()
        {
            scene()->update_control_point(index, point);
        };

        auto undo_command = [=]()
        {
            scene()->update_control_point(index, old_point);
        };

        command();
        canvas()->perform_action("Resize control point", command, undo_command);        
    }
}

void ControlPointsMode::delete_selected_control_point()
{
    const auto& control_points = scene()->track().control_points();
    if (selected_control_point_index_ && *selected_control_point_index_ < control_points.size())
    {
        std::size_t index = *selected_control_point_index_;
        auto point = control_points[index];

        auto command = [=]()
        {
            scene()->delete_control_point(index);
        };

        auto undo_command = [=]()
        {
            scene()->insert_control_point(index, point);
        };

        command();
        canvas()->perform_action("Remove control point", command, undo_command);
    }

    deselect_control_point();
}

void ControlPointsMode::update_control_point_info()
{
    canvas()->display_tool_info("");
    canvas()->display_secondary_tool_info("");
}

void ControlPointsMode::update_control_point_info(std::size_t index)
{
    const auto& control_points = scene()->track().control_points();
    if (index < control_points.size())
    {
        using components::ControlPoint;
        const auto& point = control_points[index];

        canvas()->display_tool_info("Point " + QString::number(index));

        core::Vector2i start = point.start;
        core::Vector2i end = start;

        if (point.direction == ControlPoint::Horizontal) end.x += point.length;
        else end.y += point.length;

        QString text = QString::number(start.x) + ", " + QString::number(start.y);
        text += " -> ";
        text += QString::number(end.x) + ", " +QString::number(end.y);

        canvas()->display_secondary_tool_info(text);
    }

    else
    {
        update_control_point_info();
    }
}

NAMESPACE_INTERFACE_MODES_END