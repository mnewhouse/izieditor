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

    virtual void mouse_press_event(QMouseEvent* event) override;
    virtual void mouse_release_event(QMouseEvent* event) override;
    virtual void mouse_move_event(QMouseEvent* event, core::Vector2i track_position, core::Vector2i track_delta) override;

    void place_start_point(core::Vector2i position, core::Vector2i face_towards, bool fix_rotation);

    void delete_last_start_point();
    void delete_selected_start_point();

    virtual void tool_changed(EditorTool tool) override;

private:
    virtual std::uint32_t enabled_tools() const override;
    virtual void on_activate() override;

    void select_start_point(std::size_t index);
    void deselect_start_point();

    void rotate_start_point_towards(std::size_t index, core::Vector2i target_point, bool fix_rotation);

    void commit_start_point_changes(const std::string& action_description);
    void reload_start_points();

    void update_start_point_info(std::size_t index);
    void update_start_point_info();

    boost::optional<core::Vector2i> start_point_position_;
    boost::optional<std::size_t> hovered_start_point_index_;
    boost::optional<std::size_t> selected_start_point_index_;

    std::vector<components::StartPoint> start_points_;
    std::vector<sf::Vertex> vertex_cache_;
};        

NAMESPACE_INTERFACE_MODES_END

#endif