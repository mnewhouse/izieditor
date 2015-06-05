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

#include "pit_mode.hpp"

#include "../editor_canvas.hpp"

#include "scene/scene.hpp"


NAMESPACE_INTERFACE_MODES

PitMode::PitMode(EditorCanvas* canvas)
: ModeBase(canvas)
{
}

std::uint32_t PitMode::enabled_tools() const
{
    return static_cast<std::uint32_t>(EditorTool::Placement);
}

void PitMode::on_activate()
{
    canvas()->set_active_tool(EditorTool::Placement);

    if (auto pit = scene()->track().pit())
    {
        pit_defined(*pit);
    }

    else
    {
        pit_undefined();
    }
}

void PitMode::define_pit(core::Vector2i start, core::Vector2i end)
{
    core::IntRect pit(start, end, core::rect::from_points);
    scene()->define_pit(pit);

    auto command = [=]()
    {
        scene()->define_pit(pit);
        pit_defined(pit);
    };

    auto undo_command = [=]()
    {
        scene()->undefine_pit();
        pit_undefined();
    };

    command();
    canvas()->perform_action("Define pit", command, undo_command);
}

void PitMode::undefine_pit()
{

    if (auto pit_ptr = scene()->track().pit())
    {
        core::IntRect pit = *pit_ptr;

        auto command = [=]()
        {
            scene()->undefine_pit();
            pit_undefined();
        };

        auto undo_command = [=]()
        {
            scene()->define_pit(pit);
            pit_defined(pit);
        };

        command();
        canvas()->perform_action("Pit undefined", command, undo_command);
    }
}

void PitMode::mouse_press_event(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (pit_start_)
        {
            define_pit(*pit_start_, canvas()->mouse_position());

            pit_start_ = boost::none;
        }

        else
        {
            pit_start_ = canvas()->mouse_position();
        }
    }
}

void PitMode::render(sf::RenderTarget& render_target, sf::RenderStates render_states)
{
    if (pit_start_)
    {
        sf::RectangleShape origin_shape;
        origin_shape.setFillColor(sf::Color(255, 250, 0));

        float size = 2.0f / canvas()->zoom_level();
        origin_shape.setSize(sf::Vector2f(size, size));
        origin_shape.setPosition(sf::Vector2f(pit_start_->x, pit_start_->y));

        render_target.draw(origin_shape, render_states);
    }

    else if (auto pit_rect = scene()->track().pit())
    {
        sf::RectangleShape pit_shape;
        pit_shape.setOutlineThickness(2.0f / canvas()->zoom_level());
        pit_shape.setOutlineColor(sf::Color(255, 250, 0));
        pit_shape.setFillColor(sf::Color::Transparent);

        pit_shape.setSize(sf::Vector2f(pit_rect->width, pit_rect->height));
        pit_shape.setPosition(sf::Vector2f(pit_rect->left, pit_rect->top));

        render_target.draw(pit_shape, render_states);
    }
}

void PitMode::pit_defined(core::IntRect pit)
{
    QString text = "Pit defined from (";
    text += QString::number(pit.left);
    text += ", ";
    text += QString::number(pit.top);
    text += ") to (";
    text += QString::number(pit.right());
    text += ", ";
    text += QString::number(pit.bottom());
    text += ").";

    canvas()->display_tool_info(text);
    canvas()->display_secondary_tool_info("");
}

void PitMode::pit_undefined()
{
    canvas()->display_tool_info("No pit defined.");
    canvas()->display_secondary_tool_info("");
}

NAMESPACE_INTERFACE_MODES_END