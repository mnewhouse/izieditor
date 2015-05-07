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

#include "editor_canvas.hpp"
#include "action.hpp"

#include "editor_modes/mode_base.hpp"
#include "editor_modes/tiles_mode.hpp"
#include "editor_modes/control_points_mode.hpp"
#include "editor_modes/start_points_mode.hpp"
#include "editor_modes/pit_mode.hpp"
#include "editor_modes/pattern_mode.hpp"

#include "scene/scene.hpp"

#include "graphics/font_bitmap.hpp"
#include "graphics/font_bitmap_data.hpp"

#include "components/track.hpp"
#include "components/tile_definition.hpp"

#include <qevent.h>
#include <qimage.h>
#include <qbitmap.h>

#include <boost/optional.hpp>

namespace interface
{
    struct AreaSelectionTool
    {
        core::Vector2i selection_origin_;
        core::IntRect selection_rect_;
        core::IntRect temp_selection_;
    };

    struct CursorStore
    {
        CursorStore(EditorCanvas* canvas);                

        using CursorId = EditorCanvas::CursorId;
        CursorId create_cursor_from_image(EditorCanvas* canvas, const QImage& image);
        CursorId create_cursor_from_resource(EditorCanvas* canvas, const QString& file_path);
        CursorId create_empty_cursor(EditorCanvas* canvas);

        CursorId cursor_id(EditorCursor cursor) const;

        CursorId empty_cursor_;
        CursorId magic_wand_;
        CursorId magic_wand_plus_;
        CursorId magic_wand_minus_;
        CursorId movement_tool_;
        CursorId rotation_tool_;
        CursorId dragging_hand_;
    };


    CursorStore::CursorStore(EditorCanvas* canvas)
        : empty_cursor_(create_empty_cursor(canvas)),
          magic_wand_(create_cursor_from_resource(canvas, ":/Icons/resources/wand.png")),
          magic_wand_plus_(create_cursor_from_resource(canvas, ":/Icons/resources/wand--plus.png")),
          magic_wand_minus_(create_cursor_from_resource(canvas, ":/Icons/resources/wand--minus.png")),
          dragging_hand_(create_cursor_from_resource(canvas, ":/Icons/resources/hand.png")),
          movement_tool_(create_cursor_from_resource(canvas, ":/Icons/resources/arrow-move.png")),
          rotation_tool_(create_cursor_from_resource(canvas, ":/Icons/resources/arrow-circle-225.png"))
    {
    }


    CursorStore::CursorId CursorStore::cursor_id(EditorCursor cursor) const
    {
        switch (cursor)
        {
        case EditorCursor::Default:
            return EditorCanvas::InvalidCursorId;

        case EditorCursor::None:
            return empty_cursor_;

        case EditorCursor::Hand:
            return dragging_hand_;

        case EditorCursor::MagicWand:
            return magic_wand_;

        case EditorCursor::MagicWandPlus:
            return magic_wand_plus_;

        case EditorCursor::MagicWandMinus:
            return magic_wand_minus_;

        case EditorCursor::Movement:
            return movement_tool_;

        case EditorCursor::Rotation:
            return rotation_tool_;
        }

        return EditorCanvas::InvalidCursorId;
    }

    CursorStore::CursorId CursorStore::create_empty_cursor(EditorCanvas* canvas)
    {
        QImage image(1, 1, QImage::Format_ARGB32);
        image.setPixel(0, 0, qRgba(0, 0, 0, 0));

        return create_cursor_from_image(canvas, image);
    }

    CursorStore::CursorId CursorStore::create_cursor_from_image(EditorCanvas* canvas, const QImage& image)
    {
        if (!image.isNull())
        {
            auto image_size = image.size();

            sf::Image sf_image;
            sf_image.create(image_size.width(), image_size.height());

            for (int y = 0; y < image_size.height(); ++y)
            {
                for (int x = 0; x < image_size.width(); ++x)
                {
                    auto color = image.pixel(x, y);
                    sf_image.setPixel(x, y, sf::Color(qRed(color), qGreen(color), qBlue(color), qAlpha(color)));
                }
            }

            return canvas->create_cursor(sf_image);
        }

        return EditorCanvas::InvalidCursorId;
    }

    CursorStore::CursorId CursorStore::create_cursor_from_resource(EditorCanvas* canvas, const QString& file_path)
    {
        return create_cursor_from_image(canvas, QImage(file_path));
    }

    struct EditorCanvas::Impl
    {
        explicit Impl(EditorCanvas* self)
            : self_(self),
              font_bitmap_(graphics::default_font_data, graphics::default_font_data_size),
              tiles_mode_(self),
              control_points_mode_(self),
              start_points_mode_(self),
              pit_mode_(self),
              pattern_mode_(self),
              cursor_store_(self)
        {
        }

        EditorCanvas* self_;
        std::unique_ptr<scene::Scene> scene_;

        core::Vector2<double> camera_position_;
        double zoom_level_ = 1.5;

        core::Vector2i absolute_mouse_position_;
        core::Vector2i mouse_position_;
        core::Vector2<double> scroll_amount_;
        bool drag_active_ = false;

        EditorTool active_tool_ = EditorTool::None;
        EditorMode active_mode_ = EditorMode::None;

        modes::TilesMode tiles_mode_;
        modes::ControlPointsMode control_points_mode_;
        modes::StartPointsMode start_points_mode_;
        modes::PitMode pit_mode_;
        modes::PatternMode pattern_mode_;

        AreaSelectionTool area_selection_;
        components::ConstLayerHandle selected_layer_;

        graphics::FontBitmap font_bitmap_;
        CursorStore cursor_store_;

        void recalculate_view();
        double compute_fitting_zoom_level();

        void expand_area_selection(core::Vector2i position);
        void commit_area_selection();
        void select_area(core::IntRect rect);

        void select_layer(std::size_t layer_id);
        void select_layer(components::ConstLayerHandle layer);

        void hide_layer(std::size_t layer_id);
        void show_layer(std::size_t layer_id);

        void restore_layer(std::size_t layer_id, std::size_t index);
        void delete_layer(std::size_t layer_id);

        void move_layer(std::size_t layer_id, std::size_t new_index);
        void rename_layer(std::size_t layer_id, const std::string& new_name);

        void set_layer_level(std::size_t layer_id, std::size_t new_level);
        void dispatch_layer_mergable_signal();

        ModeBase* mode_object(EditorMode mode);
    };


    EditorCanvas::EditorCanvas(QWidget* parent)
        : QtSFMLCanvas(parent),
        impl_(std::make_unique<Impl>(this))
    {
    }

    EditorCanvas::~EditorCanvas()
    {
    }

    void EditorCanvas::onRender()
    {
        // Apply scrolling
        impl_->camera_position_.x += impl_->scroll_amount_.x;
        impl_->camera_position_.y += impl_->scroll_amount_.y;
        impl_->scroll_amount_ = {};

        clear(sf::Color(128, 128, 128));

        if (impl_->scene_)
        {
            impl_->recalculate_view();

            const auto track_size = impl_->scene_->track().size();            

            sf::RenderStates render_states;

            sf::RectangleShape shape;
            shape.setFillColor(sf::Color::Black);
            shape.setSize(sf::Vector2f(track_size.x, track_size.y));
            draw(shape, render_states);
            
            scene::draw(*impl_->scene_, *this, render_states);

            auto mode = active_mode();
            if (auto mode_object = impl_->mode_object(mode))
            {
                mode_object->customize_cursor();
            }

            if (mode == EditorMode::Tiles)
            {
                impl_->tiles_mode_.render(*this, render_states);
            }

            else if (mode == EditorMode::ControlPoints)
            {
                impl_->control_points_mode_.render(*this, render_states, impl_->font_bitmap_);
            }

            else if (mode == EditorMode::StartPoints)
            {
                impl_->start_points_mode_.render(*this, render_states, impl_->font_bitmap_);
            }

            else if (mode == EditorMode::Pit)
            {
                impl_->pit_mode_.render(*this, render_states);
            }

            else if (mode == EditorMode::Pattern)
            {
                impl_->pattern_mode_.render(*this, render_states);
            }

            auto rect = impl_->area_selection_.selection_rect_;
            if (rect.width == 0 || rect.height == 0)
            {
                rect = impl_->area_selection_.temp_selection_;
            }

            if (rect.width != 0 && rect.height != 0)
            {
                sf::RectangleShape rect_display(sf::Vector2f(rect.width, rect.height));
                rect_display.setOutlineColor(sf::Color(255, 255, 255, 100));
                rect_display.setOutlineThickness(1.0);
                rect_display.setFillColor(sf::Color(200, 220, 255, 40));
                rect_display.setPosition(static_cast<float>(rect.left), static_cast<float>(rect.top));
                draw(rect_display);
            }
        }
    }

    void EditorCanvas::onResize()
    {
        auto view = getView();
        view.setSize(getSize().x, getSize().y);
        setView(view);

        impl_->recalculate_view();
    }

    void EditorCanvas::adopt_scene(std::unique_ptr<scene::Scene>& scene_ptr_)
    {
        impl_->scene_ = std::move(scene_ptr_);

        setMouseTracking(true);

        scene::Scene* scene_ptr = impl_->scene_.get();
        impl_->tiles_mode_.initialize(scene_ptr);
        impl_->control_points_mode_.initialize(scene_ptr);
        impl_->start_points_mode_.initialize(scene_ptr);
        impl_->pit_mode_.initialize(scene_ptr);
        impl_->pattern_mode_.initialize(scene_ptr);

        set_zoom_level(1.5);

        set_active_mode(EditorMode::None);
        set_active_tool(EditorTool::None);

        scene_loaded(impl_->scene_.get());

        impl_->selected_layer_ = {};
        layer_deselected();

        if (!track().layers().empty())
        {
            select_layer(track().layers().front().id());
        }

        impl_->select_area({});        

        show();        
    }

    void EditorCanvas::zoom_in()
    {
        set_zoom_level(zoom_level() * 1.1);
    }

    void EditorCanvas::zoom_out()
    {
        set_zoom_level(zoom_level() / 1.1);
    }

    void EditorCanvas::zoom_to_fit()
    {
        set_zoom_level(impl_->compute_fitting_zoom_level());
    }

    void EditorCanvas::set_zoom_level(double zoom_level)
    {
        impl_->zoom_level_ = std::max(std::min(zoom_level, 5.0), 0.2);

        zoom_level_changed(zoom_level);
    }

    double EditorCanvas::Impl::compute_fitting_zoom_level()
    {
        if (scene_)
        {
            auto track_size = scene_->track().size();
            auto track_width = static_cast<double>(track_size.x);
            auto track_height = static_cast<double>(track_size.y);

            auto my_size = self_->size();
            auto my_width = static_cast<double>(my_size.width());
            auto my_height = static_cast<double>(my_size.height());

            double zoom_x = my_width / track_width;
            double zoom_y = my_height / track_height;

            return std::min(zoom_x, zoom_y);
        }

        return 1.0;
    }

    void EditorCanvas::set_left_camera_anchor(double x_position)
    {
        sf::Vector2f top_left = getView().getInverseTransform().transformPoint(-1.0, 1.0);
        
        impl_->scroll_amount_.x += x_position - top_left.x;
    }

    void EditorCanvas::set_top_camera_anchor(double y_position)
    {
        sf::Vector2f top_left = getView().getInverseTransform().transformPoint(-1.0, 1.0);

        impl_->scroll_amount_.y += y_position - top_left.y;
    }

    void EditorCanvas::Impl::recalculate_view()
    {
        if (scene_)
        {
            auto fitting_zoom = compute_fitting_zoom_level();
            if (zoom_level_ < fitting_zoom) self_->set_zoom_level(fitting_zoom);

            core::Vector2u track_size = scene_->track().size();
            auto track_width = static_cast<float>(track_size.x);
            auto track_height = static_cast<float>(track_size.y);

            sf::Vector2u window_size = self_->getSize();
            auto window_width = static_cast<float>(window_size.x);
            auto window_height = static_cast<float>(window_size.y);

            const float inverse_zoom = 1.0 / zoom_level_;

            sf::View view;
            float visible_width = window_width * inverse_zoom;
            float visible_height = window_height * inverse_zoom;

            sf::FloatRect view_port;
            view_port.left = 0.0f;
            view_port.top = 0.0f;
            view_port.width = std::min(track_width / visible_width, 1.0f);
            view_port.height = std::min(track_height / visible_height, 1.0f);
            view.setViewport(view_port);

            view.setCenter(camera_position_.x, camera_position_.y);
            view.setSize(std::round(visible_width * view_port.width), std::round(visible_height * view_port.height));

            auto top_left = view.getInverseTransform().transformPoint(-1.0, 1.0);
            if (top_left.x < 0.0f)
            {
                camera_position_.x -= top_left.x;
            }

            if (top_left.y < 0.0f)
            {
                camera_position_.y -= top_left.y;
            }

            auto bottom_right = view.getInverseTransform().transformPoint(1.0, -1.0);
            if (bottom_right.x > track_width)
            {
                camera_position_.x -= bottom_right.x - track_width;
            }

            if (bottom_right.y > track_height)
            {
                camera_position_.y -= bottom_right.y - track_height;
            }

            view.setCenter(camera_position_.x, camera_position_.y);            
            
            self_->setView(view);

            top_left = view.getInverseTransform().transformPoint(-1.0, 1.0);
            core::DoubleRect visible_area =
            { 
                std::max(top_left.x, 0.0f) / track_width, 
                std::max(top_left.y, 0.0f) / track_height,
                visible_width / track_width,
                visible_height / track_height
            };

            self_->visible_area_updated(visible_area);
        }
    }

    void EditorCanvas::keyPressEvent(QKeyEvent* event)
    {
        QtSFMLCanvas::keyPressEvent(event);

        if (auto mode = impl_->mode_object(active_mode()))
        {
            mode->key_press_event(event);
        }
    }

    void EditorCanvas::keyReleaseEvent(QKeyEvent* event)
    {
        QtSFMLCanvas::keyReleaseEvent(event);

        if (auto mode = impl_->mode_object(active_mode()))
        {
            mode->key_release_event(event);
        }
    }

    void EditorCanvas::wheelEvent(QWheelEvent* event)
    {
        QtSFMLCanvas::wheelEvent(event);

        if (auto mode = impl_->mode_object(active_mode()))
        {
            mode->wheel_event(event);
        }
    }

    void EditorCanvas::mousePressEvent(QMouseEvent* event)
    {
        QtSFMLCanvas::mousePressEvent(event);

        if (auto mode = impl_->mode_object(active_mode()))
        {
            mode->mouse_press_event(event);
        }

        if (active_tool() == EditorTool::AreaSelection)
        {
            auto& area_selection = impl_->area_selection_;
            area_selection.selection_origin_ = mouse_position();
            area_selection.temp_selection_ = core::IntRect(area_selection.selection_origin_, core::Vector2i(0, 0));

            selection_area_changed(area_selection.temp_selection_);
        }

        if (event->button() == Qt::RightButton)
        {
            impl_->drag_active_ = true;

            set_prioritized_cursor(EditorCursor::Hand);
        }
    }

    void EditorCanvas::mouseReleaseEvent(QMouseEvent* event)
    {
        QtSFMLCanvas::mouseReleaseEvent(event);

        if (auto mode = impl_->mode_object(active_mode()))
        {
            mode->mouse_release_event(event);
        }

        if (active_tool() == EditorTool::AreaSelection)
        {
            impl_->commit_area_selection();
        }

        if (event->button() == Qt::RightButton)
        {
            impl_->drag_active_ = false;

            set_prioritized_cursor(EditorCursor::Default);
        }
    }

    void EditorCanvas::mouseMoveEvent(QMouseEvent* event)
    {
        QtSFMLCanvas::mouseMoveEvent(event);

        auto my_size = size();
        auto new_pos = event->pos();
        auto modifiers = event->modifiers();

        std::int32_t x = new_pos.x(), y = new_pos.y();
        std::int32_t w = my_size.width(), h = my_size.height();

        if (impl_->scene_)
        {
            const auto& view = getView();
            const auto& view_port = view.getViewport();

            float relative_x = static_cast<float>(x) / my_size.width() / view_port.width;
            float relative_y = static_cast<float>(y) / my_size.height() / view_port.height;

            auto absolute_delta = core::Vector2i(x, y) - impl_->absolute_mouse_position_;

            relative_x = relative_x * 2.0f - 1.0f;
            relative_y = -relative_y * 2.0f + 1.0f;

            sf::Vector2f track_point_sf = view.getInverseTransform().transformPoint(relative_x, relative_y);

            core::Vector2i track_point(track_point_sf.x, track_point_sf.y);
            core::Vector2i delta = track_point - impl_->mouse_position_;

            mouse_move(QPoint(track_point.x, track_point.y));

            auto tool = active_tool();
            auto buttons = event->buttons();

            impl_->mouse_position_ = track_point;
            if (impl_->drag_active_)
            {
                impl_->scroll_amount_.x -= absolute_delta.x / zoom_level();
                impl_->scroll_amount_.y -= absolute_delta.y / zoom_level();
            }

            if (tool == EditorTool::AreaSelection && buttons & Qt::LeftButton)
            {
                impl_->expand_area_selection(track_point);
            }
            
            if (auto mode = impl_->mode_object(active_mode()))
            {
                mode->mouse_move_event(event, track_point, delta);
            }
        }

        impl_->absolute_mouse_position_.x = x;
        impl_->absolute_mouse_position_.y = y;
    }

    void EditorCanvas::set_active_cursor(EditorCursor cursor)
    {
        CursorId cursor_id = impl_->cursor_store_.cursor_id(cursor);
        set_active_cursor(cursor_id);
    }
    
    void EditorCanvas::set_prioritized_cursor(EditorCursor cursor)
    {
        CursorId cursor_id = impl_->cursor_store_.cursor_id(cursor);
        set_prioritized_cursor(cursor_id);
    }

    EditorTool EditorCanvas::active_tool() const
    {
        return impl_->active_tool_;
    }

    EditorMode EditorCanvas::active_mode() const
    {
        return impl_->active_mode_;
    }

    ModeBase* EditorCanvas::Impl::mode_object(EditorMode mode)
    {
        switch (mode)
        {
        case EditorMode::Tiles:
            return &tiles_mode_;

        case EditorMode::ControlPoints:
            return &control_points_mode_;

        case EditorMode::StartPoints:
            return &start_points_mode_;

        case EditorMode::Pit:
            return &pit_mode_;

        case EditorMode::Pattern:
            return &pattern_mode_;
        }

        return nullptr;
    }

    core::IntRect EditorCanvas::selected_area() const
    {
        return impl_->area_selection_.selection_rect_;
    }

    core::Vector2i EditorCanvas::mouse_position() const
    {
        return impl_->mouse_position_;
    }

    double EditorCanvas::zoom_level() const
    {
        return impl_->zoom_level_;
    }

    void EditorCanvas::perform_action(const std::string& text, std::function<void()> command,
        std::function<void()> undo_command)
    {
        perform_action(Action(text, std::move(command), std::move(undo_command)));
    }


    const scene::Scene* EditorCanvas::scene() const
    {
        return impl_->scene_.get();
    }

    std::size_t EditorCanvas::num_levels() const
    {
        if (impl_->scene_)
        {
            return impl_->scene_->track().num_levels();
        }

        return 0;
    }

    core::Vector2i EditorCanvas::track_size() const
    {
        if (impl_->scene_)
        {
            return impl_->scene_->track().size();
        }

        return {};
    }


    const components::Track& EditorCanvas::track() const
    {
        return impl_->scene_->track();
    }

    const std::string& EditorCanvas::track_name() const
    {
        return track().name();
    }


    const components::PatternStore& EditorCanvas::pattern_store() const
    {
        return impl_->scene_->pattern_store();
    }

    const components::ConstLayerHandle& EditorCanvas::selected_layer() const
    {
        return impl_->selected_layer_;
    }

    void EditorCanvas::enable_tool(EditorTool tool, bool enable)
    {
        if (enable) tool_enabled(tool);
        else tool_disabled(tool);
    }

    void EditorCanvas::set_active_tool(EditorTool tool)
    {
        auto old_tool = active_tool();
        impl_->active_tool_ = tool;
        tool_changed(tool);

        auto mode = active_mode();

        if (auto mode_object = impl_->mode_object(mode))
        {
            mode_object->tool_changed(tool);
        }

        if (tool == EditorTool::TileSelection || tool == EditorTool::Movement || tool == EditorTool::Rotation)
        {
            impl_->select_area({});
        }
    }

    void EditorCanvas::set_active_mode(EditorMode mode)
    {
        auto old_mode = active_mode();
        impl_->active_mode_ = mode;
        mode_changed(mode);

        if (old_mode != mode)
        {

            if (auto old_mode_object = impl_->mode_object(old_mode))
            {
                old_mode_object->deactivate();
            }

            if (auto new_mode_object = impl_->mode_object(mode))
            {
                new_mode_object->activate();
            }
        }
    }

    void EditorCanvas::activate_placement_tool()
    {
        set_active_tool(EditorTool::Placement);
    }

    void EditorCanvas::activate_tile_selection_tool()
    {
        set_active_tool(EditorTool::TileSelection);
    }

    void EditorCanvas::activate_area_selection_tool()
    {
        set_active_tool(EditorTool::AreaSelection);
    }

    void EditorCanvas::activate_move_tool()
    {
        set_active_tool(EditorTool::Movement);
    }

    void EditorCanvas::activate_rotation_tool()
    {
        set_active_tool(EditorTool::Rotation);
    }

    void EditorCanvas::activate_tiles_mode()
    {
        set_active_mode(EditorMode::Tiles);
    }

    void EditorCanvas::activate_control_points_mode()
    {
        set_active_mode(EditorMode::ControlPoints);
    }

    void EditorCanvas::activate_start_points_mode()
    {
        set_active_mode(EditorMode::StartPoints);
    }

    void EditorCanvas::activate_pit_mode()
    {
        set_active_mode(EditorMode::Pit);
    }

    void EditorCanvas::activate_pattern_mode()
    {
        set_active_mode(EditorMode::Pattern);
    }

    void EditorCanvas::deselect()
    {
        if (active_mode() == EditorMode::Tiles)
        {
            impl_->tiles_mode_.deselect();
        }

        if (impl_->area_selection_.selection_rect_.width != 0 &&
            impl_->area_selection_.selection_rect_.height != 0)
        {
            auto old_selection = impl_->area_selection_.selection_rect_;

            auto command = [=]()
            {
                impl_->select_area({});
            };

            auto undo_command = [=]()
            {
                impl_->select_area(old_selection);
            };

            command();
            perform_action("Deselect area", command, undo_command);
        }
    }

    void EditorCanvas::delete_selection()
    {
        if (active_mode() == EditorMode::Tiles)
        {
            impl_->tiles_mode_.delete_selection();
        }
    }

    void EditorCanvas::copy_selection()
    {
        if (active_mode() == EditorMode::Tiles)
        {
            impl_->tiles_mode_.copy_selection();
        }
    }

    void EditorCanvas::cut_selection()
    {
        if (active_mode() == EditorMode::Tiles)
        {
            impl_->tiles_mode_.cut_selection();
        }
    }

    void EditorCanvas::paste_clipboard()
    {
        if (active_mode() == EditorMode::Tiles)
        {
            impl_->tiles_mode_.paste_clipboard(impl_->camera_position_);
        }
    }

    void EditorCanvas::fill_area(const FillProperties& fill_properties)
    {
        if (active_mode() == EditorMode::Tiles)
        {
            impl_->tiles_mode_.fill_area(fill_properties);
        }
    }

    void EditorCanvas::Impl::expand_area_selection(core::Vector2i position)
    {
        auto origin = area_selection_.selection_origin_;
        area_selection_.temp_selection_ = core::IntRect(origin, position, core::rect::from_points);

        self_->selection_area_changed(area_selection_.temp_selection_);
    }

    void EditorCanvas::Impl::select_area(core::IntRect area)
    {
        area_selection_.selection_rect_ = area;
        area_selection_.temp_selection_ = {};
        area_selection_.selection_origin_ = {};

        self_->area_selected(area);
    }

    void EditorCanvas::Impl::commit_area_selection()
    {
        auto old_selection = area_selection_.selection_rect_;
        auto new_selection = area_selection_.temp_selection_;

        if (new_selection != old_selection)
        {
            auto command = [=]()
            {
                self_->set_active_tool(EditorTool::AreaSelection);
                select_area(new_selection);
            };

            auto undo_command = [=]()
            {
                select_area(new_selection);
            };

            command();
            const char* text = "Select area";
            if (new_selection.width == 0 || new_selection.height == 0) text = "Deselect area";

            self_->perform_action(text, command, undo_command);
        }
    }

    void EditorCanvas::Impl::dispatch_layer_mergable_signal()
    {
        const auto& layers = scene_->layers();
        if (auto layer = selected_layer_)
        {
            const auto& layers = scene_->layers();

            std::size_t layer_id = layer.id();
            std::size_t index = scene_->find_layer_index(layer_id);
            if (index != 0 && layers[index - 1]->level == layer->level)
            {
                self_->layer_merging_enabled(true);
            }

            else
            {
                self_->layer_merging_enabled(false);
            }
        }

        else
        {
            self_->layer_merging_enabled(false);
        }
    }


    void EditorCanvas::Impl::select_layer(std::size_t layer_id)
    {
        if (scene_)
        {
            if (auto layer = scene_->track().layer_by_id(layer_id))
            {
                select_layer(layer);
            }           
        }
    }

    void EditorCanvas::Impl::select_layer(components::ConstLayerHandle layer)
    {
        if (selected_layer_ != layer)
        {
            selected_layer_ = layer;

            if (active_mode_ == EditorMode::Tiles)
            {
                tiles_mode_.layer_selected(layer.id());
            }

            if (layer)
            {
                self_->layer_selected(layer.id());
            }

            else
            {
                self_->layer_deselected();
            }

            dispatch_layer_mergable_signal();
        }
    }

    void EditorCanvas::Impl::hide_layer(std::size_t layer_id)
    {
        if (scene_)
        {
            scene_->hide_layer(layer_id);

            self_->layer_visibility_changed(layer_id, false);
        }
    }


    void EditorCanvas::Impl::show_layer(std::size_t layer_id)
    {
        if (scene_)
        {
            scene_->show_layer(layer_id);

            self_->layer_visibility_changed(layer_id, true);
        }
    }

    void EditorCanvas::Impl::restore_layer(std::size_t layer_id, std::size_t index)
    {
        if (scene_)
        {
            scene_->restore_layer(layer_id, index);
            self_->layer_created(layer_id, index);
        }
    }

    void EditorCanvas::Impl::delete_layer(std::size_t layer_id)
    {
        if (scene_)
        {
            if (selected_layer_.id() == layer_id)
            {
                select_layer(components::ConstLayerHandle());
            }

            scene_->delete_layer(layer_id);
            self_->layer_deleted(layer_id);

            dispatch_layer_mergable_signal();
        }
    }

    void EditorCanvas::Impl::move_layer(std::size_t layer_id, std::size_t new_index)
    {
        if (scene_)
        {
            scene_->move_layer(layer_id, new_index);
            self_->layer_moved(layer_id, new_index);

            dispatch_layer_mergable_signal();
        }
    }

    void EditorCanvas::Impl::rename_layer(std::size_t layer_id, const std::string& new_name)
    {
        if (scene_)
        {
            scene_->rename_layer(layer_id, new_name);
            self_->layer_renamed(layer_id, new_name);
        }
    }

    void EditorCanvas::Impl::set_layer_level(std::size_t layer_id, std::size_t new_level)
    {
        if (scene_)
        {
            scene_->set_layer_level(layer_id, new_level);
            self_->layer_level_changed(layer_id, new_level);
        }
    }

    void EditorCanvas::delete_last()
    {
        auto mode = active_mode();
        if (mode == EditorMode::Tiles)
        {
            impl_->tiles_mode_.delete_last_tile();
        }

        else if (mode == EditorMode::ControlPoints)
        {
            impl_->control_points_mode_.delete_last_control_point();
        }

        else if (mode == EditorMode::StartPoints)
        {
            impl_->start_points_mode_.delete_last_start_point();
        }

        else if (mode == EditorMode::Pit)
        {
            impl_->pit_mode_.undefine_pit();
        }
    }

    void EditorCanvas::hide_layer(std::size_t layer_id)
    {
        if (impl_->scene_)
        {
            std::size_t selected_layer = impl_->selected_layer_.id();

            auto command = [=]()
            {
                impl_->hide_layer(layer_id);

                if (impl_->selected_layer_.id() == layer_id)
                {
                    impl_->select_layer(components::ConstLayerHandle());
                }
            };

            auto undo_command = [=]()
            {
                impl_->show_layer(layer_id);
                impl_->select_layer(selected_layer);
            };

            command();
            perform_action("Hide layer", command, undo_command);
        }
    }

    void EditorCanvas::show_layer(std::size_t layer_id)
    {
        if (impl_->scene_)
        {
            std::size_t selected_layer = impl_->selected_layer_.id();

            auto command = [=]()
            {
                impl_->scene_->show_layer(layer_id);
                impl_->select_layer(layer_id);
            };

            auto undo_command = [=]()
            {
                impl_->scene_->hide_layer(layer_id);
                impl_->select_layer(selected_layer);
            };

            command();
            perform_action("Show layer", command, undo_command);            
        }
    }

    void EditorCanvas::move_layer(std::size_t layer_id, std::size_t new_index)
    {
        if (impl_->scene_)
        {
            std::size_t old_index = impl_->scene_->find_layer_index(layer_id);

            auto command = [=]()
            {
                impl_->move_layer(layer_id, new_index);
            };

            auto undo_command = [=]()
            {
                impl_->move_layer(layer_id, old_index);
            };


            command();
            perform_action("Move layer", command, undo_command);
        }
    }

    void EditorCanvas::rename_layer(std::size_t layer_id, const std::string& new_name)
    {
        if (impl_->scene_)
        {
            if (auto layer = impl_->scene_->track().layer_by_id(layer_id))
            {
                const auto& old_name = layer->name;

                auto command = [=]()
                {
                    impl_->rename_layer(layer_id, new_name);
                };

                auto undo_command = [=]()
                {
                    impl_->rename_layer(layer_id, old_name);
                };

                command();
                perform_action("Rename layer", command, undo_command);
            }            
        }
    }

    void EditorCanvas::set_layer_level(std::size_t layer_id, std::size_t new_level)
    {
        if (impl_->scene_)
        {
            if (auto layer = impl_->scene_->track().layer_by_id(layer_id))
            {
                std::size_t old_index = impl_->scene_->find_layer_index(layer_id);
                std::size_t old_level = layer->level;

                auto command = [=]()
                {
                    impl_->set_layer_level(layer_id, new_level);
                };

                auto undo_command = [=]()
                {
                    impl_->set_layer_level(layer_id, old_level);
                    impl_->move_layer(layer_id, old_index);
                };

                command();
                perform_action("Change level", command, undo_command);
            }
        }
    }

    std::size_t EditorCanvas::create_layer()
    {
        std::size_t layer_count = impl_->scene_->track().layer_count();
        create_layer("Layer " + std::to_string(layer_count + 1), 0);

        return components::InvalidLayerId;
    }

    std::size_t EditorCanvas::create_layer(const std::string& layer_name, std::size_t level)
    {
        auto layer = impl_->scene_->create_layer(layer_name, level);
        std::size_t layer_id = layer.id();
        std::size_t index = impl_->scene_->find_layer_index(layer_id);

        layer_created(layer_id, index);
        impl_->select_layer(layer);           

        auto command = [=]()
        {
            impl_->restore_layer(layer_id, index);
            impl_->select_layer(layer);                
        };

        auto undo_command = [=]()
        {
            impl_->delete_layer(layer_id);
        };

        perform_action("Create layer", command, undo_command);
        return layer_id;
    }

    void EditorCanvas::delete_layer(std::size_t layer_id)
    {
        if (impl_->scene_)
        {
            std::size_t selected_layer = impl_->selected_layer_.id();
            std::size_t index = impl_->scene_->find_layer_index(layer_id);

            auto command = [=]()
            {
                impl_->delete_layer(layer_id);
            };

            auto undo_command = [=]()
            {
                impl_->restore_layer(layer_id, index);
                impl_->select_layer(selected_layer);
            };

            command();
            perform_action("Delete layer", command, undo_command);
        }
    }

    void EditorCanvas::select_layer(std::size_t layer_id)
    {
        impl_->select_layer(layer_id);
    }

    void EditorCanvas::deselect_layer()
    {
        impl_->select_layer(components::ConstLayerHandle());
    }

    void EditorCanvas::merge_selected_layer_with_previous()
    {
        if (auto layer = selected_layer())
        {
            const auto& layers = scene()->layers();
            std::size_t layer_id = layer.id();
            std::size_t index = scene()->find_layer_index(layer_id);

            if (index != 0 && layers[index - 1]->level == layer->level)
            {
                const auto& previous = layers[index - 1];
                std::size_t previous_id = previous.id();
                std::size_t tile_count = layer->tiles.size();

                auto command = [=]()
                {
                    for (const auto& tile : layer->tiles)
                    {
                        impl_->scene_->append_tile(previous_id, tile);
                    }

                    impl_->delete_layer(layer_id);
                    select_layer(previous_id);
                };

                auto undo_command = [=]()
                {
                    impl_->scene_->delete_last_tiles(previous_id, tile_count);
                    impl_->restore_layer(layer_id, index);
                    select_layer(layer_id);
                };

                command();
                perform_action("Merge layers", command, undo_command);
            }
        }
    }

    void EditorCanvas::resize_track(std::int32_t new_width, std::int32_t new_height,
        HorizontalAnchor horizontal_anchor, VerticalAnchor vertical_anchor)
    {
        if (impl_->scene_)
        {
            core::Vector2i tile_offset;

            core::Vector2i new_size(new_width, new_height);
            core::Vector2i old_size = impl_->scene_->track().size();
            
            core::Vector2i growth = new_size - old_size;

            if (horizontal_anchor == HorizontalAnchor::Right) tile_offset.x = growth.x;
            else if (horizontal_anchor == HorizontalAnchor::Center) tile_offset.x = growth.x / 2;

            if (vertical_anchor == VerticalAnchor::Bottom) tile_offset.y = growth.y;
            else if (vertical_anchor == VerticalAnchor::Center) tile_offset.y = growth.y / 2;

            auto command = [=]()
            {
                impl_->scene_->resize_track(new_size);
                impl_->scene_->move_all_tiles(tile_offset);
            };

            auto undo_command = [=]()
            {
                impl_->scene_->resize_track(old_size);
                impl_->scene_->move_all_tiles(-tile_offset);
            };

            command();
            perform_action("Resize track", command, undo_command);            
        }
    }
}