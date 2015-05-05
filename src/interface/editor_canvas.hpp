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
}

namespace interface
{
    enum class EditorMode
    {
        None,
        Tiles,
        ControlPoints,
        StartPoints,
        Pattern
    };

    enum class EditorTool
        : std::uint32_t
    {
        None = 0,
        Placement = 1,
        AreaSelection = 2,
        TileSelection = 4,
        Movement = 8,
        Rotation = 16,
        All = 31
    };

    inline std::uint32_t operator|(EditorTool a, EditorTool b)
    {
        return static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b);
    }

    inline std::uint32_t operator|(std::uint32_t a, EditorTool b)
    {
        return a | static_cast<std::uint32_t>(b);
    }

    inline std::uint32_t operator|(EditorTool a, std::uint32_t b)
    {
        return static_cast<std::uint32_t>(a) | b;
    }

    inline std::uint32_t operator&(std::uint32_t a, EditorTool b)
    {
        return a & static_cast<std::uint32_t>(b);
    }


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

        virtual void leaveEvent(QEvent*) override;
        virtual void mouseMoveEvent(QMouseEvent*) override;
        virtual void mousePressEvent(QMouseEvent*) override;
        virtual void mouseReleaseEvent(QMouseEvent*) override;
        virtual void wheelEvent(QWheelEvent*) override;
        virtual void keyPressEvent(QKeyEvent*) override;

        EditorTool active_tool() const;
        EditorMode active_mode() const;

        const scene::Scene* scene() const;
        const components::Track& track() const;
        std::size_t num_levels() const;
        core::Vector2i track_size() const;

        const components::ConstLayerHandle& selected_layer() const;
        core::IntRect selected_area() const;

        core::Vector2i mouse_position() const;
        double zoom_level() const;

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

        void fill_area(const FillProperties& properties);
        void resize_track(std::int32_t, std::int32_t height,
            HorizontalAnchor horizontal_anchor, VerticalAnchor vertical_anchor);

    signals:
        void perform_action(const Action& action);

        void scene_loaded(const scene::Scene* scene_ptr);

        void tool_changed(EditorTool);
        void mode_changed(EditorMode);

        void tool_enabled(EditorTool tool);
        void tool_disabled(EditorTool tool);

        void mouse_move(const QPoint&);

        void tile_selection_changed(std::size_t selected_tile_count);
        void tile_selection_hover_changed(const components::Tile* tile);

        void selection_area_changed(core::IntRect area);

        void placement_tile_changed(const components::TileGroupDefinition* tile_group_def);
        void placement_tile_rotated(std::int32_t rotation);

        void tiles_rotated(core::Rotation<double> delta);
        void tiles_rotation_finished();

        void tiles_moved(core::Vector2i offset);
        void tiles_movement_finished();

        void clipboard_filled();
        void clipboard_emptied();

        void layer_created(std::size_t layer_id, std::size_t index);
        void layer_deleted(std::size_t layer_id);

        void layer_selected(std::size_t layer_id);
        void layer_deselected();

        void layer_moved(std::size_t layer_id, std::size_t new_index);
        void layer_renamed(std::size_t layer_id, const std::string& new_name);
        void layer_level_changed(std::size_t layer_id, std::size_t new_level);

    private:
        void recalculate_view();

        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
};

#endif