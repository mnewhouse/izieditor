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

#ifndef TILES_MODE_HPP
#define TILES_MODE_HPP

#include "mode_base.hpp"

#include "../editor_modes.hpp"

#include "components/tile_definition.hpp"

#include "core/vector2.hpp"

#include <SFML/Graphics.hpp>

#include <qevent.h>

#include <map>
#include <cstdint>
#include <cstddef>
#include <memory>

namespace interface
{
    struct FillProperties;
}

NAMESPACE_INTERFACE_MODES

struct TilesMode
    : public ModeBase
{
    TilesMode(EditorCanvas* canvas);
    ~TilesMode();

    void render(sf::RenderTarget& render_target, sf::RenderStates render_states);

    virtual void tool_changed(EditorTool tool) override;
    virtual void layer_selected(std::size_t layer_id) override;

    virtual void key_press_event(QKeyEvent* key_event) override;
    virtual void mouse_press_event(QMouseEvent* mouse_event) override;
    virtual void mouse_release_event(QMouseEvent* mouse_event) override;
    virtual void mouse_move_event(QMouseEvent* mouse_event, core::Vector2i track_point, core::Vector2i track_delta) override;
    virtual void wheel_event(QWheelEvent* wheel_event) override;

    void place_tile();
    void place_tile_before();

    void rotate_placement_tile(std::int32_t rotation_delta, bool round = false);

    void goto_next_tile();
    void goto_previous_tile();
    void goto_tile(components::TileId tile_id);

    void fill_area(const FillProperties& fill_properties);

    void select_active_tile();
    void select_tiles(const std::map<std::size_t, components::Tile>& selection);
    void select_tile_range(std::size_t tile_index, std::size_t count);
    void tile_selection_changed();
    void deselect();

    void delete_selection();
    void delete_last_tile();
    void cut_selection();
    void copy_selection();
    void paste_clipboard(core::Vector2i position);

    void move_selected_tiles(core::Vector2i offset, bool fix_position = false);
    void commit_tile_movement();

    void rotate_selected_tiles(core::Rotation<double> delta_rotation, bool fix_rotation = false);
    void commit_tile_rotation();
    void compute_rotation_origin();

    virtual void customize_cursor() override;

private:
    virtual void on_initialize(scene::Scene* scene) override;
    virtual void on_activate() override;
    virtual std::uint32_t enabled_tools() const override;

    void initialize_tile_placement();
    void initialize_tile_selection();

    void update_tile_selection(core::Vector2i track_point);
    void update_tile_placement();

    void rebuild_tile_selection_display();
    std::size_t acquire_level_layer(std::size_t level);

    struct Features;
    std::unique_ptr<Features> features_;
};

NAMESPACE_INTERFACE_MODES_END

#endif