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
#include "fill_properties.hpp"
#include "action.hpp"

#include "scene/scene.hpp"

#include "graphics/font_bitmap.hpp"
#include "graphics/font_bitmap_data.hpp"

#include "components/track.hpp"
#include "components/tile_library.hpp"
#include "components/tile_definition.hpp"
#include "components/tile_group_expansion.hpp"
#include "components/component_algorithms.hpp"
#include "components/control_point.hpp"
#include "components/start_point.hpp"

#include <qevent.h>
#include <qapplication.h>
#include <qbitmap.h>

#include <boost/optional.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <chrono>
#include <numeric>
#include <set>

namespace interface
{
    static const std::string selection_vertex_shader_code =
        "#version 110\n"

        "uniform vec4 color;\n"

        "void main() {\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;\n"
        "    gl_FrontColor = color;\n"
        "}";

    static const std::string selection_fragment_shader_code =
        "#version 110\n"

        "uniform sampler2D texture;\n"

        "void main() {\n"
        "    if (abs(mod(gl_FragCoord.x, 2.0) - mod(gl_FragCoord.y, 2.0)) >= 1.0) {\n"
        "        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n"
        "    } else {\n"
        "        vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);\n"
        "        gl_FragColor = pixel * gl_Color;\n"
        "    }\n"
        "}";


    struct TilePlacementTool
    {
        scene::DisplayLayer display_layer_;

        const components::TileGroupDefinition* current_tile = nullptr;
        std::int32_t rotation = 0;
        boost::optional<core::Vector2i> fixed_position_;
        core::Vector2i tile_position_;        
    };

    struct TileSelectionTool
    {
        std::unordered_map<components::TileId, core::IntRect> tile_group_bounding_boxes_;
        std::map<std::size_t, components::Tile> selected_tiles_;

        boost::optional<std::size_t> active_index_;
        scene::DisplayLayer display_layer_;
        scene::DisplayLayer hover_layer_;

        sf::Shader selection_shader_;
    };

    struct AreaSelectionTool
    {
        core::Vector2i selection_origin_;
        core::IntRect selection_rect_;
        core::IntRect temp_selection_;
    };

    struct TileMovementTool
    {
        core::Vector2i real_offset_;
        core::Vector2i fixed_offset_;
    };

    struct TileRotationTool
    {
        core::Rotation<double> fixed_rotation_;
        core::Rotation<double> real_rotation_;
        core::Vector2<double> origin_;
    };

    struct TileClipboard
    {
        bool clear_ = false;
        std::vector<components::Tile> tiles_;
    };

    static const sf::Color selected_tile_color = sf::Color(0, 250, 180);
    static const sf::Color selected_tile_hover_color = sf::Color(0, 160, 250);

    struct ModeBase
    {
    public:
        ModeBase(EditorCanvas* canvas)
            : canvas_(canvas)
        {
        }

        void initialize(scene::Scene* scene);

        void activate();
        void deactivate();
        bool is_active() const;

        const scene::Scene* scene() const;
        scene::Scene* scene();

        const EditorCanvas* canvas() const;
        EditorCanvas* canvas();

        virtual void tool_changed(EditorTool tool) {}
        virtual void layer_selected(std::size_t layer_id) {}

    private:
        virtual void on_initialize(scene::Scene* scene) {}
        virtual void on_activate() {}
        virtual void on_deactivate() {}
        virtual std::uint32_t enabled_tools() const;

        EditorCanvas* canvas_;
        scene::Scene* scene_ = nullptr;
        bool is_active_ = false;
    };

    struct TilesMode
        : public ModeBase
    {
        TilesMode(EditorCanvas* canvas)
            : ModeBase(canvas)
        {}

        void render(sf::RenderTarget& render_target, sf::RenderStates render_states);

        virtual void tool_changed(EditorTool tool) override;
        virtual void layer_selected(std::size_t layer_id) override;

        void key_press_event(QKeyEvent* key_event);

        void mouse_press_event(QMouseEvent* mouse_event);
        void mouse_release_event(QMouseEvent* mouse_event);
        void mouse_move_event(QMouseEvent* mouse_event, core::Vector2i track_point, core::Vector2i track_delta);
        void wheel_event(QWheelEvent* wheel_event);

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
        void paste_clipboard(core::Vector2<double> position);

        void move_selected_tiles(core::Vector2i offset, bool fix_position = false);
        void commit_tile_movement();
        

        void rotate_selected_tiles(core::Rotation<double> delta_rotation, bool fix_rotation = false);
        void commit_tile_rotation();
        void compute_rotation_origin();

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

        TilePlacementTool tile_placement_;
        TileSelectionTool tile_selection_;
        TileMovementTool movement_;
        TileRotationTool rotation_;
        TileClipboard clipboard_;
    };

    struct ControlPointsMode
        : ModeBase
    {
        ControlPointsMode(EditorCanvas* canvas)
            : ModeBase(canvas)
        {
        }

        void render(sf::RenderTarget& render_target, sf::RenderStates render_states,
            const graphics::FontBitmap& font_bitmap);

        void delete_last_control_point();

        void mouse_press_event(QMouseEvent* event);

        virtual void tool_changed(EditorTool tool) override;

    private:
        virtual std::uint32_t enabled_tools() const override;
        virtual void on_activate() override;

        void place_control_point(core::Vector2i start, core::Vector2i end);

        boost::optional<core::Vector2i> control_point_start_;
        std::vector<sf::Vertex> vertex_cache_;
    };

    struct StartPointsMode
        : ModeBase
    {
        StartPointsMode(EditorCanvas* canvas)
            : ModeBase(canvas)
        {
        }

        void render(sf::RenderTarget& render_target, sf::RenderStates render_states,
                    const graphics::FontBitmap& font_bitmap);

        void mouse_press_event(QMouseEvent* event);
        void place_start_point(core::Vector2i position, core::Vector2i face_towards, bool fix_rotation);
        void delete_last_start_point();

        virtual void tool_changed(EditorTool tool) override;

    private:
        virtual std::uint32_t enabled_tools() const override;
        virtual void on_activate() override;

        boost::optional<core::Vector2i> start_point_position_;
        std::vector<components::StartPoint> start_points_;
        std::vector<sf::Vertex> vertex_cache_;
    };

    struct EditorCanvas::Impl
    {
        explicit Impl(EditorCanvas* self)
            : self_(self),
              font_bitmap_(graphics::default_font_data, graphics::default_font_data_size),
              tiles_mode_(self),
              control_points_mode_(self),
              start_points_mode_(self)
        {
        }

        EditorCanvas* self_;
        std::unique_ptr<scene::Scene> scene_;

        core::Vector2<float> camera_position_;
        float zoom_level_ = 1.5f;
        std::int32_t horizontal_scroll_ = 0;
        std::int32_t vertical_scroll_ = 0;

        core::Vector2i mouse_position_;

        std::chrono::high_resolution_clock::time_point last_frame_;

        EditorTool active_tool_ = EditorTool::None;
        EditorMode active_mode_ = EditorMode::None;

        TilesMode tiles_mode_;
        ControlPointsMode control_points_mode_;
        StartPointsMode start_points_mode_;

        AreaSelectionTool area_selection_;
        components::ConstLayerHandle selected_layer_;

        std::vector<components::Tile> tile_cache_;
        std::vector<components::PlacedTile> sub_tile_cache_;

        graphics::FontBitmap font_bitmap_;

        double compute_fitting_zoom_level();

        void expand_area_selection(core::Vector2i position);
        void commit_area_selection();
        void select_area(core::IntRect rect);

        void select_layer(std::size_t layer_id);
        void select_layer(components::ConstLayerHandle layer);

        void restore_layer(std::size_t layer_id, std::size_t index);
        void delete_layer(std::size_t layer_id);

        void move_layer(std::size_t layer_id, std::size_t new_index);
        void rename_layer(std::size_t layer_id, const std::string& new_name);

        void set_layer_level(std::size_t layer_id, std::size_t new_level);
    };

    void ModeBase::initialize(scene::Scene* scene)
    {
        scene_ = scene;

        on_initialize(scene);
    }

    void ModeBase::activate()
    {
        is_active_ = true;

        std::uint32_t tool_flags = enabled_tools();

        auto conditional_enable = [=](EditorTool tool)
        {
            if ((tool_flags & tool) != 0) canvas()->tool_enabled(tool);
            else canvas()->tool_disabled(tool);
        };

        conditional_enable(EditorTool::Placement);
        conditional_enable(EditorTool::Movement);
        conditional_enable(EditorTool::Rotation);
        conditional_enable(EditorTool::TileSelection);
        conditional_enable(EditorTool::AreaSelection);

        on_activate();
    }

    std::uint32_t ModeBase::enabled_tools() const
    {
        return EditorTool::Placement | EditorTool::TileSelection | EditorTool::AreaSelection |
            EditorTool::Movement | EditorTool::Rotation;
    }

    void ModeBase::deactivate()
    {
        is_active_ = false;

        on_deactivate();
    }

    bool ModeBase::is_active() const
    {
        return is_active_;
    }

    EditorCanvas* ModeBase::canvas()
    {
        return canvas_;
    }

    const EditorCanvas* ModeBase::canvas() const
    {
        return canvas_;
    }

    scene::Scene* ModeBase::scene()
    {
        return scene_;
    }

    const scene::Scene* ModeBase::scene() const
    {
        return scene_;
    }


    EditorCanvas::EditorCanvas(QWidget* parent)
        : QtSFMLCanvas(parent),
        impl_(std::make_unique<Impl>(this))
    {
        impl_->last_frame_ = std::chrono::high_resolution_clock::now();
    }

    EditorCanvas::~EditorCanvas()
    {
    }

    void TilesMode::render(sf::RenderTarget& render_target, sf::RenderStates render_states)
    {
        auto tool = canvas()->active_tool();
        auto selection_states = render_states;

        if (tool == EditorTool::Movement)
        {
            auto offset = movement_.fixed_offset_;
            selection_states.transform.translate(offset.x, offset.y);
        }

        else if (tool == EditorTool::Rotation)
        {
            auto origin = rotation_.origin_;
            auto rotation = rotation_.fixed_rotation_;

            sf::Transformable transformable;
            transformable.setOrigin(origin.x, origin.y);
            transformable.setRotation(rotation.degrees());
            transformable.setPosition(origin.x, origin.y);

            selection_states.transform *= transformable.getTransform();
        }

        selection_states.shader = &tile_selection_.selection_shader_;
        tile_selection_.selection_shader_.setParameter("color", selected_tile_color);

        scene::draw(tile_selection_.display_layer_, render_target, selection_states);
        tile_selection_.selection_shader_.setParameter("color", selected_tile_hover_color);

        if (tool == EditorTool::TileSelection)
        {
            scene::draw(tile_selection_.hover_layer_, render_target, selection_states);
        }

        if (tool == EditorTool::Placement)
        {
            auto placement_states = render_states;
            auto translation = tile_placement_.tile_position_;

            placement_states.transform.translate(translation.x, translation.y);
            scene::draw(tile_placement_.display_layer_, render_target, placement_states);
        }
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
        return EditorTool::Placement | EditorTool::AreaSelection;
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
        return EditorTool::Placement | EditorTool::AreaSelection;
    }

    void StartPointsMode::tool_changed(EditorTool tool)
    {
        start_point_position_ = boost::none;
    }

    void EditorCanvas::onRender()
    {
        auto new_frame = std::chrono::high_resolution_clock::now();

        auto frame_duration = std::chrono::duration_cast<std::chrono::milliseconds>(new_frame - impl_->last_frame_);
        float scroll_speed = frame_duration.count() / impl_->zoom_level_ * 0.8f;

        // Apply scrolling
        impl_->camera_position_.x += scroll_speed * impl_->horizontal_scroll_;
        impl_->camera_position_.y += scroll_speed * impl_->vertical_scroll_;

        impl_->last_frame_ = new_frame;

        if (impl_->scene_)
        {
            clear(sf::Color::Black);

            recalculate_view();

            sf::RenderStates render_states;
            scene::draw(*impl_->scene_, *this, render_states);

            auto mode = active_mode();
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

        else
        {
            clear(sf::Color(128, 128, 128));
        }
    }

    void EditorCanvas::onResize()
    {
        auto view = getView();
        view.setSize(getSize().x, getSize().y);
        setView(view);

        recalculate_view();
    }

    void EditorCanvas::adopt_scene(std::unique_ptr<scene::Scene>& scene_ptr_)
    {
        impl_->scene_ = std::move(scene_ptr_);

        setMouseTracking(true);

        scene::Scene* scene_ptr = impl_->scene_.get();
        impl_->tiles_mode_.initialize(scene_ptr);
        impl_->control_points_mode_.initialize(scene_ptr);
        impl_->start_points_mode_.initialize(scene_ptr);

        set_active_mode(EditorMode::None);
        set_active_tool(EditorTool::None);        

        impl_->selected_layer_ = {};
        layer_deselected();

        if (!impl_->scene_->track().layers().empty())
        {
            impl_->selected_layer_ = impl_->scene_->track().layers().front();
            layer_selected(impl_->selected_layer_.id());
        }

        impl_->select_area({});

        scene_loaded(impl_->scene_.get());

        show();        
    }

    void EditorCanvas::zoom_in()
    {
        impl_->zoom_level_ *= 1.1f;
        impl_->zoom_level_ = std::min(impl_->zoom_level_, 5.0f);
    }

    void EditorCanvas::zoom_out()
    {
        impl_->zoom_level_ /= 1.1f;
        impl_->zoom_level_ = std::max(impl_->zoom_level_, 0.2f);
    }

    void EditorCanvas::zoom_to_fit()
    {
        impl_->zoom_level_ = impl_->compute_fitting_zoom_level();
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

    void EditorCanvas::recalculate_view()
    {
        if (impl_->scene_)
        {
            auto fitting_zoom = impl_->compute_fitting_zoom_level();
            if (impl_->zoom_level_ < fitting_zoom) impl_->zoom_level_ = fitting_zoom;

            core::Vector2u track_size = impl_->scene_->track().size();
            auto track_width = static_cast<float>(track_size.x);
            auto track_height = static_cast<float>(track_size.y);

            sf::Vector2u window_size = getSize();

            auto window_width = static_cast<float>(window_size.x);
            auto window_height = static_cast<float>(window_size.y);

            auto inverse_zoom = 1.0f / impl_->zoom_level_;

            sf::View view;
            float visible_width = window_width * inverse_zoom;
            float visible_height = window_height * inverse_zoom;

            sf::FloatRect view_port;
            view_port.left = 0.0f;
            view_port.top = 0.0f;
            view_port.width = std::min(track_width / visible_width, 1.0f);
            view_port.height = std::min(track_height / visible_height, 1.0f);
            view.setViewport(view_port);

            view.setCenter(impl_->camera_position_.x, impl_->camera_position_.y);
            view.setSize(std::round(visible_width * view_port.width), std::round(visible_height * view_port.height));

            auto top_left = view.getInverseTransform().transformPoint(-1.0, 1.0);
            if (top_left.x < 0.0f)
            {
                impl_->camera_position_.x -= top_left.x;
            }

            if (top_left.y < 0.0f)
            {
                impl_->camera_position_.y -= top_left.y;
            }

            auto bottom_right = view.getInverseTransform().transformPoint(1.0, -1.0);
            if (bottom_right.x > track_width)
            {
                impl_->camera_position_.x -= bottom_right.x - track_width;
            }

            if (bottom_right.y > track_height)
            {
                impl_->camera_position_.y -= bottom_right.y - track_height;
            }

            view.setCenter(impl_->camera_position_.x, impl_->camera_position_.y);

            setView(view);
        }
    }

    void TilesMode::key_press_event(QKeyEvent* event)
    {
        auto tool = canvas()->active_tool();
        if (tool == EditorTool::Placement)
        {
            auto key_modifiers = QApplication::queryKeyboardModifiers();
            std::int32_t rotation_amount = 45;
            bool round = false;
            if (key_modifiers & Qt::ControlModifier) rotation_amount = 1;
            else if (key_modifiers & Qt::ShiftModifier) rotation_amount = 5;
            else round = true;

            if (event->key() == Qt::Key_Left)
            {
                rotate_placement_tile(-rotation_amount, round);
            }

            else if (event->key() == Qt::Key_Right)
            {
                rotate_placement_tile(rotation_amount, round);
            }

            else if (event->key() == Qt::Key_Up)
            {
                goto_previous_tile();
            }

            else if (event->key() == Qt::Key_Down)
            {
                goto_next_tile();
            }

            else if (event->key() == Qt::Key_Alt || event->key() == Qt::Key_AltGr)
            {
                tile_placement_.fixed_position_ = canvas()->mouse_position();
            }

            else if (event->key() == Qt::Key_Space)
            {
                place_tile_before();
            }

            switch (event->key())
            {
            case Qt::Key_0:
                goto_tile(0); break;
            case Qt::Key_1:
                goto_tile(100); break;
            case Qt::Key_2:
                goto_tile(200); break;
            case Qt::Key_3:
                goto_tile(300); break;
            case Qt::Key_4:
                goto_tile(400); break;
            case Qt::Key_5:
                goto_tile(500); break;
            case Qt::Key_6:
                goto_tile(600); break;
            case Qt::Key_7:
                goto_tile(700); break;
            case Qt::Key_8:
                goto_tile(800); break;
            case Qt::Key_9:
                goto_tile(900); break;

            default:
                break;
            }
        }

        else if (tool == EditorTool::Movement || tool == EditorTool::TileSelection)
        {
            if (event->key() == Qt::Key_Left)
            {
                move_selected_tiles(core::Vector2i(-10, 0));
            }

            else if (event->key() == Qt::Key::Key_Right)
            {
                move_selected_tiles(core::Vector2i(10, 0));
            }

            else if (event->key() == Qt::Key::Key_Up)
            {
                move_selected_tiles(core::Vector2i(0, -10));
            }

            else if (event->key() == Qt::Key::Key_Down)
            {
                move_selected_tiles(core::Vector2i(0, 10));
            }
        }
    }

    void EditorCanvas::keyPressEvent(QKeyEvent* event)
    {
        if (active_mode() == EditorMode::Tiles)
        {
            impl_->tiles_mode_.key_press_event(event);
        }
    }

    void TilesMode::wheel_event(QWheelEvent* event)
    {
        if (event->delta() > 0)
        {
            goto_next_tile();
        }

        else if (event->delta() < 0)
        {
            goto_previous_tile();
        }
    }

    void EditorCanvas::wheelEvent(QWheelEvent* event)
    {
        if (active_mode() == EditorMode::Tiles)
        {
            impl_->tiles_mode_.wheel_event(event);
        }
    }

    void EditorCanvas::mousePressEvent(QMouseEvent* event)
    {
        auto mode = active_mode();
        if (mode == EditorMode::Tiles)
        {
            impl_->tiles_mode_.mouse_press_event(event);
        }

        else if (mode == EditorMode::ControlPoints)
        {
            impl_->control_points_mode_.mouse_press_event(event);
        }

        else if (mode == EditorMode::StartPoints)
        {
            impl_->start_points_mode_.mouse_press_event(event);
        }

        if (active_tool() == EditorTool::AreaSelection)
        {
            auto& area_selection = impl_->area_selection_;
            area_selection.selection_origin_ = mouse_position();
            area_selection.temp_selection_ = core::IntRect(area_selection.selection_origin_, core::Vector2i(0, 0));

            selection_area_changed(area_selection.temp_selection_);
        }
    }

    void TilesMode::mouse_release_event(QMouseEvent* event)
    {
        auto tool = canvas()->active_tool();
        if (event->button() == Qt::LeftButton)
        {
            if (tool == EditorTool::Movement)
            {
                if (movement_.fixed_offset_.x != 0 && movement_.fixed_offset_.y != 0)
                {
                    commit_tile_movement();
                }
            }

            if (tool == EditorTool::Rotation)
            {
                if (std::abs(rotation_.fixed_rotation_.degrees()) >= 1.0)
                {
                    commit_tile_rotation();
                }
            }
        }
    }


    void EditorCanvas::mouseReleaseEvent(QMouseEvent* event)
    {
        if (active_mode() == EditorMode::Tiles)
        {
            impl_->tiles_mode_.mouse_release_event(event);
        }

        if (active_tool() == EditorTool::AreaSelection)
        {
            impl_->commit_area_selection();
        }
    }

    void TilesMode::mouse_move_event(QMouseEvent* event, core::Vector2i track_point, core::Vector2i track_delta)
    {
        auto tool = canvas()->active_tool();
        auto buttons = event->buttons();

        auto mouse_position = canvas()->mouse_position();

        auto modifiers = QApplication::queryKeyboardModifiers();
        if ((buttons & Qt::LeftButton) && tool == EditorTool::Rotation)
        {
            auto prior_position = track_point - track_delta;

            auto origin = rotation_.origin_;
            auto before = std::atan2(prior_position.y - origin.y, prior_position.x - origin.x);
            auto after = std::atan2(track_point.y - origin.y, track_point.x - origin.x);
            auto delta = core::Rotation<double>::radians(after - before);

            bool fix_rotation = (modifiers & Qt::AltModifier) != 0;
            rotate_selected_tiles(delta, fix_rotation);
        }

        else if ((buttons & Qt::LeftButton) && tool == EditorTool::Movement)
        {
            bool fix_position = (modifiers & Qt::AltModifier) != 0;
            move_selected_tiles(track_delta, fix_position);
        }

        else if (tool == EditorTool::TileSelection)
        {
            update_tile_selection(track_point);
        }

        else if (tool == EditorTool::Placement)
        {
            if ((modifiers & Qt::AltModifier) == 0)
            {
                tile_placement_.fixed_position_ = boost::none;
            }

            auto position = track_point;
            if (tile_placement_.fixed_position_)
            {
                auto fixed_position = *tile_placement_.fixed_position_;
                auto difference = fixed_position - position;
                if (std::abs(difference.x) < std::abs(difference.y)) position.x = fixed_position.x;
                else position.y = fixed_position.y;
            }

            tile_placement_.tile_position_ = position;
        }
    }

    void EditorCanvas::mouseMoveEvent(QMouseEvent* event)
    {
        auto my_size = size();
        auto new_pos = event->pos();

        std::int32_t multiplier = 1;
        auto modifiers = QApplication::queryKeyboardModifiers();
        if (modifiers & Qt::ControlModifier)
        {
            multiplier = 2;
        }

        std::int32_t x = new_pos.x(), y = new_pos.y();
        std::int32_t w = my_size.width(), h = my_size.height();

        impl_->horizontal_scroll_ = 0;
        if (x >= 0 && x < 25)
        {
            impl_->horizontal_scroll_ -= multiplier;
        }

        if (x < w && x >= w - 25)
        {
            impl_->horizontal_scroll_ += multiplier;
        }

        impl_->vertical_scroll_ = 0;
        if (y >= 0 && y < 25)
        {
            impl_->vertical_scroll_ -= multiplier;
        }

        if (y < h && y >= h - 25)
        {
            impl_->vertical_scroll_ += multiplier;
        }

        if (impl_->scene_)
        {
            const auto& view = getView();
            const auto& view_port = view.getViewport();

            float relative_x = static_cast<float>(new_pos.x()) / my_size.width() / view_port.width;
            float relative_y = static_cast<float>(new_pos.y()) / my_size.height() / view_port.height;

            relative_x = relative_x * 2.0f - 1.0f;
            relative_y = -relative_y * 2.0f + 1.0f;

            sf::Vector2f track_point_sf = view.getInverseTransform().transformPoint(relative_x, relative_y);

            core::Vector2i track_point(static_cast<std::int32_t>(std::round(track_point_sf.x)),
                static_cast<std::int32_t>(std::round(track_point_sf.y)));

            core::Vector2i delta = track_point - core::Vector2i(impl_->mouse_position_);

            mouse_move(QPoint(track_point.x, track_point.y));

            auto tool = active_tool();
            auto buttons = event->buttons();

            if (tool == EditorTool::AreaSelection && buttons & Qt::LeftButton)
            {
                impl_->expand_area_selection(track_point);
            }

            impl_->mouse_position_ = track_point;
            if (active_mode() == EditorMode::Tiles)
            {
                impl_->tiles_mode_.mouse_move_event(event, track_point, delta);
            }           
        }
    }

    void EditorCanvas::leaveEvent(QEvent* event)
    {
        impl_->horizontal_scroll_ = 0;
        impl_->vertical_scroll_ = 0;
    }

    EditorTool EditorCanvas::active_tool() const
    {
        return impl_->active_tool_;
    }

    EditorMode EditorCanvas::active_mode() const
    {
        return impl_->active_mode_;
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

        if (tool != old_tool)
        {
            if (active_mode() == EditorMode::Tiles)
            {
                impl_->tiles_mode_.tool_changed(tool);
            }

            if (tool == EditorTool::TileSelection || tool == EditorTool::Movement || tool == EditorTool::Rotation)
            {
                impl_->select_area({});
            }
        }

        tool_changed(tool);
    }

    void EditorCanvas::set_active_mode(EditorMode mode)
    {
        auto old_mode = active_mode();
        if (old_mode != mode)
        {
            impl_->active_mode_ = mode;
            auto identify_mode = [=](EditorMode mode) -> ModeBase*
            {
                switch (mode)
                {
                case EditorMode::Tiles:
                    return &impl_->tiles_mode_;

                case EditorMode::ControlPoints:
                    return &impl_->control_points_mode_;

                case EditorMode::StartPoints:
                    return &impl_->start_points_mode_;
                }

                return nullptr;
            };

            if (auto old_mode_object = identify_mode(old_mode))
            {
                old_mode_object->deactivate();
            }

            if (auto new_mode_object = identify_mode(mode))
            {
                new_mode_object->activate();
            }
        }

        mode_changed(mode);
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

    void EditorCanvas::activate_pattern_mode()
    {
        set_active_mode(EditorMode::Pattern);
    }

    void TilesMode::initialize_tile_selection()
    {
        bool x = tile_selection_.selection_shader_.loadFromMemory(selection_vertex_shader_code, selection_fragment_shader_code);
        tile_selection_.selection_shader_.setParameter("texture", sf::Shader::CurrentTexture);

        const auto& tile_library = scene()->tile_library();

        std::vector<components::PlacedTile> tile_buffer_;

        // We need to cache the tile groups' bounding boxes if we're going to use
        // the tile selection tool efficiently.       
        for (auto tile_group = tile_library.first_tile_group(); tile_group != nullptr;
            tile_group = tile_library.next_tile_group(tile_group->id()))
        {
            components::Tile dummy_tile;
            dummy_tile.id = tile_group->id();

            tile_buffer_.clear();
            components::expand_tile_groups(&dummy_tile, &dummy_tile + 1, tile_library,
                std::back_inserter(tile_buffer_));

            core::IntRect bounding_box = std::accumulate(tile_buffer_.begin(), tile_buffer_.end(), core::IntRect(),
                [](core::IntRect rect, const components::PlacedTile& placed_tile)
            {
                // Need to find the bounding box of the group component as relative to the 
                // tile group's origin.
                core::IntRect pattern_rect = placed_tile.tile_def->pattern_rect;

                pattern_rect.left = placed_tile.tile.position.x - pattern_rect.width / 2;
                pattern_rect.top = placed_tile.tile.position.y - pattern_rect.height / 2;

                return combine(pattern_rect, rect);
            });

            tile_selection_.tile_group_bounding_boxes_[dummy_tile.id] = bounding_box;
        }
    }

    void TilesMode::on_initialize(scene::Scene* scene)
    {
        initialize_tile_placement();
        initialize_tile_selection();
    }

    void TilesMode::on_activate()
    {
        canvas()->set_active_tool(EditorTool::Placement);
    }

    std::uint32_t TilesMode::enabled_tools() const
    {
        return EditorTool::Placement | EditorTool::AreaSelection | EditorTool::TileSelection;
    }

    void TilesMode::layer_selected(std::size_t layer)
    {
        select_tiles({});
    }

    void TilesMode::tool_changed(EditorTool tool)
    {
        if (tool == EditorTool::Placement)
        {
            update_tile_placement();

            canvas()->placement_tile_changed(tile_placement_.current_tile);
            canvas()->placement_tile_rotated(tile_placement_.rotation);

            tile_placement_.fixed_position_ = boost::none;
            tile_placement_.rotation = {};
            tile_placement_.tile_position_ = {};
        }
    }

    void TilesMode::goto_next_tile()
    {
        const auto& tile_library = scene()->tile_library();
        auto& current_tile = tile_placement_.current_tile;

        current_tile = tile_library.next_tile_group(current_tile->id());
        if (current_tile == nullptr)
        {
            current_tile = tile_library.first_tile_group();
        }

        canvas()->placement_tile_changed(current_tile);
        update_tile_placement();
    }

    void TilesMode::goto_previous_tile()
    {
        const auto& tile_library = scene()->tile_library();
        auto& current_tile = tile_placement_.current_tile;

        current_tile = tile_library.previous_tile_group(current_tile->id());
        if (current_tile == nullptr)
        {
            current_tile = tile_library.last_tile_group();
        }

        canvas()->placement_tile_changed(current_tile);
        update_tile_placement();
    }

    void TilesMode::goto_tile(components::TileId tile_id)
    {
        const auto& tile_library = scene()->tile_library();
        auto& current_tile = tile_placement_.current_tile;

        current_tile = tile_library.next_tile_group(tile_id);
        if (current_tile == nullptr)
        {
            current_tile = tile_library.last_tile_group();
        }

        canvas()->placement_tile_changed(current_tile);
        update_tile_placement();
    }

    void TilesMode::initialize_tile_placement()
    { 
        tile_placement_.current_tile = scene()->tile_library().first_tile_group();
    }

    std::size_t TilesMode::acquire_level_layer(std::size_t level)
    {
        const auto& layers = scene()->track().layers();
        auto layer_it = std::find_if(layers.begin(), layers.end(),
                                     [level](const components::ConstLayerHandle& layer)
        {
            return layer->level == level;
        });

        if (layer_it != layers.end())
        {
            return layer_it->id();
        }

        return canvas()->create_layer("Layer " + std::to_string(level), level);

    }

    void TilesMode::place_tile()
    {
        auto selected_layer = canvas()->selected_layer();
        if (selected_layer && tile_placement_.current_tile)
        {
            components::Tile tile;
            tile.position = tile_placement_.tile_position_;
            tile.id = tile_placement_.current_tile->id();
            tile.rotation = core::Rotation<double>::degrees(tile_placement_.rotation);

            std::size_t layer_id = selected_layer.id();
            if (tile.id >= 5000 && selected_layer->level == 0)
            {
                layer_id = acquire_level_layer(2);
            }

            auto command = [=]()
            {
                canvas()->select_layer(layer_id);
                scene()->append_tile(layer_id, tile);
            };

            auto undo_command = [=]()
            {
                canvas()->select_layer(layer_id);
                scene()->delete_last_tile(layer_id);
            };

            command();
            canvas()->perform_action("Place tile", command, undo_command);
        }
    }

    void TilesMode::place_tile_before()
    {
        auto selected_layer = canvas()->selected_layer();
        if (selected_layer && tile_placement_.current_tile)
        {
            components::Tile tile;
            tile.position = tile_placement_.tile_position_;
            tile.id = tile_placement_.current_tile->id();
            tile.rotation = core::Rotation<double>::degrees(tile_placement_.rotation);

            std::size_t layer_id = selected_layer.id();
            if (tile.id >= 5000 && selected_layer->level == 0)
            {
                layer_id = acquire_level_layer(2);
                selected_layer = scene()->track().layer_by_id(layer_id);
            }

            std::size_t tile_index = 0;
            if (!tile_selection_.selected_tiles_.empty())
            {
                tile_index = tile_selection_.selected_tiles_.begin()->first;
            }

            else if (!selected_layer->tiles.empty())
            {
                tile_index = selected_layer->tiles.size() - 1;
            }

            auto command = [=]()
            {
                canvas()->select_layer(layer_id);
                scene()->insert_tile(layer_id, tile_index, tile);
            };

            auto undo_command = [=]()
            {
                canvas()->select_layer(layer_id);
                scene()->delete_tile(layer_id, tile_index);
            };

            command();
            canvas()->perform_action("Place tile", command, undo_command);
        }
    }

    void TilesMode::rotate_placement_tile(std::int32_t rotation_delta, bool round)
    {
        tile_placement_.rotation += rotation_delta;
        tile_placement_.rotation %= 360;

        if (round)
        {
            tile_placement_.rotation -= tile_placement_.rotation % 45;
        }

        canvas()->placement_tile_rotated(tile_placement_.rotation);
        update_tile_placement();
    }

    void TilesMode::update_tile_selection(core::Vector2i track_point)
    {
        auto selected_layer = canvas()->selected_layer();
        if (!selected_layer) return;

        const auto& tile_library = scene()->tile_library();
        const auto& tile_list = selected_layer->tiles;

        auto best_match = tile_list.end();
        std::int32_t best_distance = 0;
        std::int32_t num_matches = 0;

        core::Vector2<double> real_position = track_point;

        const auto& bounding_boxes = tile_selection_.tile_group_bounding_boxes_;

        for (auto tile_it = tile_list.begin(); tile_it != tile_list.end(); ++tile_it)
        {
            auto tile_group = tile_library.tile_group(tile_it->id);
            if (!tile_group) continue;

            auto position = real_position;
            position.x -= tile_it->position.x;
            position.y -= tile_it->position.y;

            position = core::transform_point(position, -tile_it->rotation);

            auto map_it = bounding_boxes.find(tile_group->id());
            if (map_it != bounding_boxes.end() && contains(map_it->second, position))
            {
                std::int32_t x_diff = std::abs(position.x);
                std::int32_t y_diff = std::abs(position.y);
                std::int32_t distance = x_diff * x_diff + y_diff * y_diff;
                if (best_match == tile_list.end() || distance < best_distance)
                {
                    best_match = tile_it;
                    best_distance = distance;
                    ++num_matches;
                }
            }
        }

        if (best_match != tile_list.end())
        {
            tile_selection_.active_index_ = std::distance(tile_list.begin(), best_match);
            const auto& tile = *best_match;
            const auto& tile_mapping = scene()->tile_mapping();

            tile_selection_.hover_layer_ = scene::create_display_layer(&tile, &tile + 1, tile_library, tile_mapping);
            canvas()->tile_selection_hover_changed(&tile);
        }

        else
        {
            tile_selection_.active_index_ = boost::none;
            canvas()->tile_selection_hover_changed(nullptr);
        }
    }

    void TilesMode::rebuild_tile_selection_display()
    {
        auto selected_layer = canvas()->selected_layer();
        if (!selected_layer)
        {
            tile_selection_.display_layer_.clear();
            return;
        }

        const auto& tile_list = selected_layer->tiles;
        const auto& tile_library = scene()->tile_library();
        const auto& tile_mapping = scene()->tile_mapping();

        auto transform_func = [&](const std::pair<const std::size_t, components::Tile>& pair) -> const components::Tile&
        {
            return pair.second;
        };
        
        auto begin = boost::make_transform_iterator(tile_selection_.selected_tiles_.begin(), transform_func);
        auto end = boost::make_transform_iterator(tile_selection_.selected_tiles_.end(), transform_func);

        tile_selection_.display_layer_ = scene::create_display_layer(begin, end,
            tile_library, tile_mapping);
    }

    void TilesMode::select_active_tile()
    {
        auto selected_layer = canvas()->selected_layer();
        if (selected_layer && tile_selection_.active_index_ && 
            *tile_selection_.active_index_ < selected_layer->tiles.size())
        {
            std::size_t index = *tile_selection_.active_index_;
            std::size_t layer_id = selected_layer.id();

            const auto& tile = selected_layer->tiles[index];

            const auto& old_selection = tile_selection_.selected_tiles_;
            auto undo_command = [=]()
            {
                select_tiles(old_selection);
            };

            if (QApplication::queryKeyboardModifiers() & Qt::ControlModifier)
            {
                tile_selection_.selected_tiles_.emplace(index, tile);
            }

            else if (QApplication::queryKeyboardModifiers() & Qt::AltModifier)
            {
                tile_selection_.selected_tiles_.erase(index);
            }

            else
            {
                tile_selection_.selected_tiles_.clear();
                tile_selection_.selected_tiles_.emplace(index, tile);
            }

            const auto& new_selection = tile_selection_.selected_tiles_;
            auto command = [=]()
            {
                canvas()->select_layer(layer_id);
                select_tiles(new_selection);
            };            

            rebuild_tile_selection_display();
            compute_rotation_origin();

            tile_selection_changed();
            canvas()->perform_action("Select tiles", command, undo_command);
        }
    }

    void TilesMode::tile_selection_changed()
    {
        const auto& selection = tile_selection_.selected_tiles_;

        const bool enable_tools = !selection.empty();
        canvas()->enable_tool(EditorTool::Movement, enable_tools);
        canvas()->enable_tool(EditorTool::Rotation, enable_tools);

        canvas()->tile_selection_changed(tile_selection_.selected_tiles_.size());
    }

    void TilesMode::select_tiles(const std::map<std::size_t, components::Tile>& selection)
    {
        tile_selection_.selected_tiles_ = selection;

        rebuild_tile_selection_display();
        compute_rotation_origin();

        tile_selection_changed();
    }

    void TilesMode::select_tile_range(std::size_t tile_index, std::size_t tile_count)
    {       
        if (auto selected_layer = canvas()->selected_layer())
        {
            tile_selection_.selected_tiles_.clear();

            const auto& tiles = selected_layer->tiles;
            std::size_t end = std::min(tile_index + tile_count, tiles.size());
            for (; tile_index != end; ++tile_index)
            {
                tile_selection_.selected_tiles_.emplace(tile_index, tiles[tile_index]);
            }

            rebuild_tile_selection_display();
            compute_rotation_origin();

            tile_selection_changed();
        }       
    }

    void TilesMode::update_tile_placement()
    {
        const auto& tile_library = scene()->tile_library();
        const auto& tile_mapping = scene()->tile_mapping();

        if (tile_placement_.current_tile != nullptr)
        {
            components::Tile dummy_tile;
            dummy_tile.id = tile_placement_.current_tile->id();
            dummy_tile.position.x = 0.0;
            dummy_tile.position.y = 0.0;
            dummy_tile.rotation = core::Rotation<double>::degrees(tile_placement_.rotation);

            tile_placement_.display_layer_ = scene::create_display_layer(&dummy_tile, &dummy_tile + 1,
                tile_library, tile_mapping);
        }
    }

    void TilesMode::mouse_press_event(QMouseEvent* event)
    {
        auto tool = canvas()->active_tool();
        if (tool  == EditorTool::Placement)
        {
            place_tile();
        }

        else if (tool == EditorTool::TileSelection)
        {
            select_active_tile();
        }
    }

    void TilesMode::move_selected_tiles(core::Vector2i offset, bool fix_position)
    {        
        if (auto selected_layer = canvas()->selected_layer())
        {
            auto new_offset = movement_.fixed_offset_ + offset;
            auto new_real_offset = movement_.real_offset_ + offset;

            if (fix_position)
            {
                if (std::abs(new_real_offset.x) < std::abs(new_real_offset.y)) new_offset.x = 0;
                else new_offset.y = 0;

                offset = new_offset - movement_.fixed_offset_;
            }

            double offset_x = static_cast<double>(offset.x);
            double offset_y = static_cast<double>(offset.y);

            for (auto& tile : tile_selection_.selected_tiles_)
            {
                tile.second.position.x += offset_x;
                tile.second.position.y += offset_y;

                scene()->update_tile_preview(selected_layer.id(), tile.first, tile.second);
            }

            movement_.real_offset_ = new_real_offset;
            movement_.fixed_offset_ = new_offset;
            
            canvas()->tiles_moved(new_offset);
        }
    }

    void TilesMode::commit_tile_movement()
    {
        if (auto selected_layer = canvas()->selected_layer())
        {
            std::size_t layer_id = selected_layer.id();

            const auto& new_state = tile_selection_.selected_tiles_;
            auto prior_state = new_state;

            for (auto& pair : prior_state)
            {
                pair.second = selected_layer->tiles[pair.first];
            }

            auto command = [=]()
            {
                for (auto& pair : new_state)
                {
                    scene()->update_tile(layer_id, pair.first, pair.second);
                }

                canvas()->select_layer(layer_id);
                select_tiles(new_state);
            };

            auto undo_command = [=]()
            {
                for (const auto& pair : prior_state)
                {
                    scene()->update_tile(layer_id, pair.first, pair.second);
                }

                select_tiles(prior_state);
            };

            command();
            canvas()->perform_action("Move tiles", command, undo_command);
            canvas()->tiles_movement_finished();

            movement_ = {};
        }
    }

    void TilesMode::rotate_selected_tiles(core::Rotation<double> rotation_delta, bool fix_rotation)
    {
        if (auto selected_layer = canvas()->selected_layer())
        {
            auto origin = rotation_.origin_;

            auto new_rotation = rotation_.fixed_rotation_ + rotation_delta;
            auto new_real_rotation = rotation_.real_rotation_ + rotation_delta;

            if (fix_rotation)
            {
                auto fixed_degrees = std::round(new_real_rotation.degrees() / 45.0) * 45.0;
                new_rotation = core::Rotation<double>::degrees(fixed_degrees);

                rotation_delta = new_rotation - rotation_.fixed_rotation_;
            }            

            for (auto& tile : tile_selection_.selected_tiles_)
            {
                tile.second.rotation += rotation_delta;

                auto offset = core::transform_point(tile.second.position - origin, rotation_delta);
                tile.second.position = origin + offset;

                scene()->update_tile_preview(selected_layer.id(), tile.first, tile.second);
            }

            rotation_.real_rotation_ = new_real_rotation;
            rotation_.fixed_rotation_ = new_rotation;

            canvas()->tiles_rotated(new_rotation);
        }
    }

    void TilesMode::commit_tile_rotation()
    {
        if (auto selected_layer = canvas()->selected_layer())
        {
            std::size_t layer_id = selected_layer.id();

            const auto& new_state = tile_selection_.selected_tiles_;
            auto prior_state = tile_selection_.selected_tiles_;

            for (auto& pair : prior_state)
            {
                pair.second = selected_layer->tiles[pair.first];
            }

            auto command = [=]()
            {
                for (auto& pair : new_state)
                {
                    scene()->update_tile(layer_id, pair.first, pair.second);
                }

                canvas()->select_layer(layer_id);
                select_tiles(new_state);
            };

            auto undo_command = [=]()
            {
                for (const auto& pair : prior_state)
                {
                    scene()->update_tile(layer_id, pair.first, pair.second);
                }

                select_tiles(prior_state);
            };

            command();
            canvas()->perform_action("Rotate tiles", command, undo_command);
            canvas()->tiles_rotation_finished();

            rotation_.real_rotation_ = {};
            rotation_.fixed_rotation_ = {};

            rebuild_tile_selection_display();
        }
    }

    void TilesMode::compute_rotation_origin()
    {
        const auto& vertices = tile_selection_.display_layer_.vertices();
        if (!vertices.empty())
        {
            float min_x = vertices.front().position.x, max_x = min_x;
            float min_y = vertices.front().position.y, max_y = min_y;

            for (const auto& vertex : vertices)
            {
                auto position = vertex.position;

                if (position.x < min_x) min_x = position.x;
                if (position.y < min_y) min_y = position.y;

                if (position.x > max_x) max_x = position.x;
                if (position.y > max_y) max_y = position.y;
            }

            rotation_.origin_.x = (min_x + max_x) * 0.5f;
            rotation_.origin_.y = (min_y + max_y) * 0.5f;
        }

        else
        {
            rotation_.origin_ = {};
        }
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
        if (selected_layer_ != layer && (!layer || layer->visible))
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
        }
    }

    void EditorCanvas::Impl::move_layer(std::size_t layer_id, std::size_t new_index)
    {
        if (scene_)
        {
            scene_->move_layer(layer_id, new_index);
            self_->layer_moved(layer_id, new_index);
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

    void TilesMode::delete_last_tile()
    {
        if (auto selected_layer = canvas()->selected_layer())
        {
            std::size_t layer_id = selected_layer.id();
            auto tile = selected_layer->tiles.back();

            auto command = [=]()
            {
                canvas()->select_layer(layer_id);
                scene()->delete_last_tile(layer_id);
            };

            auto undo_command = [=]()
            {
                canvas()->select_layer(layer_id);
                scene()->append_tile(layer_id, tile);
            };

            command();
            canvas()->perform_action("Remove tile", command, undo_command);
        }
    }

    void ControlPointsMode::mouse_press_event(QMouseEvent*)
    {
        if (canvas()->active_tool() == EditorTool::Placement)
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
        if (tool == EditorTool::Placement)
        {
            auto position = canvas()->mouse_position();

            if (start_point_position_)
            {
                bool fix_rotation = (QApplication::queryKeyboardModifiers() & Qt::AltModifier) == 0;
                place_start_point(*start_point_position_, position, fix_rotation);

                start_point_position_ = boost::none;
            }

            else
            {
                start_point_position_ = position;
            }
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
    }

    void TilesMode::delete_selection()
    {
        if (auto selected_layer = canvas()->selected_layer())
        {
            const auto& selection = tile_selection_.selected_tiles_;
            std::size_t layer_id = selected_layer.id();

            auto command = [=]()
            {
                canvas()->select_layer(layer_id);

                for (const auto& tile : selection)
                {
                    scene()->delete_tile(layer_id, tile.first);
                }

                select_tiles({});
            };

            auto undo_command = [=]()
            {
                canvas()->select_layer(layer_id);

                for (const auto& tile : selection)
                {
                    scene()->insert_tile(layer_id, tile.first, tile.second);
                }

                select_tiles(selection);
            };

            command();
            canvas()->perform_action("Delete tiles", command, undo_command);
        }
    }

    void TilesMode::cut_selection()
    {
        if (auto selected_layer = canvas()->selected_layer())
        {
            std::size_t layer_id = selected_layer.id();

            const auto& selection = tile_selection_.selected_tiles_;
            auto command = [=]()
            {
                canvas()->select_layer(layer_id);
               
                clipboard_.clear_ = true;
                clipboard_.tiles_.clear();

                for (const auto& tile : selection)
                {
                    scene()->delete_tile(layer_id, tile.first);
                    clipboard_.tiles_.push_back(tile.second);
                }

                canvas()->clipboard_filled();

                select_tiles({});
            };

            const auto& clipboard = clipboard_;
            auto undo_command = [=]()
            {
                canvas()->select_layer(layer_id);

                clipboard_ = clipboard;
                if (clipboard.tiles_.size() == 0) canvas()->clipboard_emptied();
                else canvas()->clipboard_filled();

                for (const auto& tile : selection)
                {
                    scene()->insert_tile(layer_id, tile.first, tile.second);
                }

                select_tiles(selection);
            };

            command();
            canvas()->perform_action("Cut tiles", command, undo_command);
        }
    }

    void TilesMode::copy_selection()
    {
        if (auto selected_layer = canvas()->selected_layer())
        {
            clipboard_.clear_ = false;
            clipboard_.tiles_.clear();

            for (const auto& tile : tile_selection_.selected_tiles_)
            {
                clipboard_.tiles_.push_back(tile.second);
            }

            if (!clipboard_.tiles_.empty())
            {
                canvas()->clipboard_filled();
            }
        }
    }

    void TilesMode::paste_clipboard(core::Vector2<double> position)
    {
        if (auto selected_layer = canvas()->selected_layer())
        {
            std::size_t layer_id = selected_layer.id();

            const auto& clipboard = clipboard_;
            const auto& selection = tile_selection_.selected_tiles_;

            core::Vector2<double> average_position = std::accumulate(clipboard.tiles_.begin(), clipboard.tiles_.end(), core::Vector2<double>(),
                [](core::Vector2<double> pos, const components::Tile& tile)
            {
                return pos + tile.position;
            }) / static_cast<double>(clipboard.tiles_.size());;

            auto command = [=]()
            {
                

                std::size_t tile_index = selected_layer->tiles.size();
                
                for (auto tile : clipboard.tiles_)
                {
                    tile.position = position + tile.position - average_position;
                    scene()->append_tile(layer_id, tile); 
                }

                if (clipboard.clear_)
                {
                    clipboard_.tiles_.clear();
                    canvas()->clipboard_emptied();
                }

                canvas()->select_layer(layer_id);
                select_tile_range(tile_index, clipboard.tiles_.size());
            };

            auto undo_command = [=]()
            {               
                clipboard_ = clipboard;
                canvas()->clipboard_filled();

                scene()->delete_last_tiles(selected_layer.id(), clipboard.tiles_.size());

                canvas()->select_layer(layer_id);
                select_tiles(selection);
            };

            command();
            canvas()->perform_action("Paste tiles", command, undo_command);
        }
    }

    void TilesMode::deselect()
    {
        auto selected_layer = canvas()->selected_layer();
        if (!tile_selection_.selected_tiles_.empty())
        {
            std::size_t layer_id = selected_layer.id();
            const auto& old_selection = tile_selection_.selected_tiles_;

            auto command = [=]()
            {
                canvas()->select_layer(layer_id);
                select_tiles({});
            };

            auto undo_command = [=]()
            {
                canvas()->select_layer(layer_id);
                select_tiles(old_selection);
            };

            command();
            canvas()->perform_action("Deselect tiles", command, undo_command);
        }
    }

    void EditorCanvas::hide_layer(std::size_t layer_id)
    {
        if (impl_->scene_)
        {
            std::size_t selected_layer = impl_->selected_layer_.id();

            auto command = [=]()
            {
                impl_->scene_->hide_layer(layer_id);

                if (impl_->selected_layer_.id() == layer_id)
                {
                    impl_->select_layer(components::ConstLayerHandle());
                }
            };

            auto undo_command = [=]()
            {
                impl_->scene_->show_layer(layer_id);
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


    void TilesMode::fill_area(const FillProperties& properties)
    {
        auto selected_layer = canvas()->selected_layer();
        if (selected_layer && tile_placement_.current_tile)
        {
            components::FillProperties prop;
            prop.density = properties.density * 0.01;
            prop.position_jitter = properties.position_jitter * 0.01;
            prop.randomize_rotation = properties.randomize_rotation;
            prop.rotation = core::Rotation<double>::degrees(static_cast<double>(properties.rotation));

            if (properties.area == FillProperties::Selection)
            {
                prop.area = canvas()->selected_area();
            }

            else
            {
                const auto& track_size = scene()->track().size();
                prop.area.left = 0;
                prop.area.top = 0;
                prop.area.width = track_size.x;
                prop.area.height = track_size.y;
            }
            const components::TileGroupDefinition& tile_group = *tile_placement_.current_tile;

            std::size_t layer_id = selected_layer.id();
            if (tile_group.id() >= 5000 && selected_layer->level == 0)
            {
                layer_id = acquire_level_layer(2);
            }

            std::vector<components::Tile> added_tiles = scene()->fill_area(layer_id, tile_group, prop);
            std::size_t added_tile_count = added_tiles.size();

            auto command = [=]()
            {
                canvas()->select_layer(layer_id);
                scene()->append_tiles(layer_id, added_tiles.begin(), added_tiles.end());
            };

            auto undo_command = [=]()
            {
                canvas()->select_layer(layer_id);
                scene()->delete_last_tiles(layer_id, added_tile_count);
            };

            canvas()->perform_action("Fill area", command, undo_command);
        }        
    }
}