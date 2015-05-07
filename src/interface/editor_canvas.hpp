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

#ifndef EDITOR_CANVAS_HPP
#define EDITOR_CANVAS_HPP

#include "qt_sfml_canvas.hpp"
#include "resize_anchor.hpp"
#include "editor_modes.hpp"

#include "core/vector2.hpp"

#include "components/track_layer.hpp"

#include <memory>
#include <set>
#include <functional>

namespace scene
{
    class Scene;
}

namespace components
{
    class Track;
    class PatternStore;
}

namespace interface
{
    struct FillProperties;

    class Action;

    class EditorCanvas
        : public QtSFMLCanvas
    {
        Q_OBJECT

    public:
        EditorCanvas(QWidget* widget);
        virtual ~EditorCanvas();

        virtual void onRender() override;
        virtual void onResize() override;

        virtual void mouseMoveEvent(QMouseEvent*) override;
        virtual void mousePressEvent(QMouseEvent*) override;
        virtual void mouseReleaseEvent(QMouseEvent*) override;
        virtual void wheelEvent(QWheelEvent*) override;
        virtual void keyPressEvent(QKeyEvent*) override;
        virtual void keyReleaseEvent(QKeyEvent*) override;

        EditorTool active_tool() const;
        EditorMode active_mode() const;
        void enable_tool(EditorTool tool, bool enable);

        const scene::Scene* scene() const;
        const components::Track& track() const;
        const components::PatternStore& pattern_store() const;

        std::size_t num_levels() const;
        core::Vector2i track_size() const;
        const std::string& track_name() const;

        const components::ConstLayerHandle& selected_layer() const;
        core::IntRect selected_area() const;

        core::Vector2i mouse_position() const;

        double zoom_level() const;
        void set_zoom_level(double zoom_level);

        void set_left_camera_anchor(double x);
        void set_top_camera_anchor(double y);

        void set_active_cursor(EditorCursor cursor);

        void perform_action(const std::string& text, std::function<void()>, std::function<void()>);

    public slots:
        void adopt_scene(std::unique_ptr<scene::Scene>& scene_ptr);

        void zoom_in();
        void zoom_out();
        void zoom_to_fit();

        void activate_placement_tool();
        void activate_tile_selection_tool();
        void activate_area_selection_tool();
        void activate_move_tool();
        void activate_rotation_tool();

        void activate_tiles_mode();
        void activate_control_points_mode();
        void activate_start_points_mode();
        void activate_pattern_mode();
        void activate_pit_mode();

        void set_active_tool(EditorTool tool);
        void set_active_mode(EditorMode mode);

        void delete_last();
        void delete_selection();

        void cut_selection();
        void copy_selection();
        void paste_clipboard();

        void deselect();

        void show_layer(std::size_t layer_id);
        void hide_layer(std::size_t layer_id);

        void move_layer(std::size_t layer_id, std::size_t new_index);
        void rename_layer(std::size_t layer_id, const std::string& new_name);
        void set_layer_level(std::size_t layer_id, std::size_t new_level);

        std::size_t create_layer();
        std::size_t create_layer(const std::string& layer_name, std::size_t level);
        void delete_layer(std::size_t layer);

        void select_layer(std::size_t layer_id);
        void deselect_layer();

        void merge_selected_layer_with_previous();

        void fill_area(const FillProperties& properties);
        void resize_track(std::int32_t, std::int32_t height,
            HorizontalAnchor horizontal_anchor, VerticalAnchor vertical_anchor);

    signals:
        void perform_action(const Action& action);

        void scene_loaded(const scene::Scene* scene_ptr);
        void track_resized(core::Vector2i new_size);

        void zoom_level_changed(double zoom_level);

        void visible_area_updated(core::DoubleRect area);

        void tool_changed(EditorTool);
        void mode_changed(EditorMode);

        void tool_enabled(EditorTool tool);
        void tool_disabled(EditorTool tool);

        void clear_tool_info();

        void mouse_move(const QPoint&);

        void tile_selection_changed(std::size_t selected_tile_count);
        void tile_selection_hover_changed(const components::Tile* tile);

        void selection_area_changed(core::IntRect area);
        void area_selected(core::IntRect area);

        void placement_tile_changed(const components::TileGroupDefinition* tile_group_def);
        void placement_tile_rotated(std::int32_t rotation);

        void tiles_rotated(core::Rotation<double> delta);
        void tiles_rotation_finished();

        void tiles_moved(core::Vector2i offset);
        void tiles_movement_finished();

        void clipboard_filled();
        void clipboard_emptied();

        void pit_defined(core::IntRect pit);
        void pit_undefined();

        void layer_created(std::size_t layer_id, std::size_t index);
        void layer_deleted(std::size_t layer_id);

        void layer_selected(std::size_t layer_id);
        void layer_deselected();

        void layer_merging_enabled(bool enabled);

        void layer_moved(std::size_t layer_id, std::size_t new_index);
        void layer_renamed(std::size_t layer_id, const std::string& new_name);
        void layer_level_changed(std::size_t layer_id, std::size_t new_level);
        void layer_visibility_changed(std::size_t layer_id, bool visible);

    private:
        void set_prioritized_cursor(EditorCursor cursor);
        using QtSFMLCanvas::set_prioritized_cursor;
        using QtSFMLCanvas::set_active_cursor;

        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
};

#endif