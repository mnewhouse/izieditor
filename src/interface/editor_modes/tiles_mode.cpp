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

#include "tiles_mode.hpp"

#include "../editor_canvas.hpp"
#include "../fill_properties.hpp"

#include "scene/track_display.hpp"
#include "scene/scene.hpp"

#include "components/tile_library.hpp"
#include "components/tile_definition.hpp"
#include "components/component_algorithms.hpp"

#include "core/vector2.hpp"

#include <qapplication.h>

#include <boost/optional.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <numeric>

NAMESPACE_INTERFACE_MODES

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

static const sf::Color selected_tile_color = sf::Color(0, 250, 180);
static const sf::Color selected_tile_hover_color = sf::Color(0, 160, 250);

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

    bool enable_strict_rotations_ = true;
};

struct TileClipboard
{
    bool clear_ = false;
    std::vector<components::Tile> tiles_;
};

struct TilesMode::Features
{
    TilePlacementTool tile_placement_;
    TileSelectionTool tile_selection_;
    TileMovementTool movement_;
    TileRotationTool rotation_;
    TileClipboard clipboard_;
};

TilesMode::TilesMode(EditorCanvas* canvas)
    : ModeBase(canvas),
        features_(std::make_unique<Features>())
{
}

TilesMode::~TilesMode()
{
}

void TilesMode::render(sf::RenderTarget& render_target, sf::RenderStates render_states)
{
    auto tool = active_tool();
    auto selection_states = render_states;

    if (tool == EditorTool::Movement)
    {
        auto offset = features_->movement_.fixed_offset_;
        selection_states.transform.translate(offset.x, offset.y);
    }

    else if (tool == EditorTool::Rotation)
    {
        auto origin = features_->rotation_.origin_;
        auto rotation = features_->rotation_.fixed_rotation_;

        sf::Transformable transformable;
        transformable.setOrigin(origin.x, origin.y);
        transformable.setRotation(rotation.degrees());
        transformable.setPosition(origin.x, origin.y);

        selection_states.transform *= transformable.getTransform();
    }

    auto& tile_placement = features_->tile_placement_;
    auto& tile_selection = features_->tile_selection_;

    selection_states.shader = &tile_selection.selection_shader_;
    tile_selection.selection_shader_.setParameter("color", selected_tile_color);

    scene::draw(tile_selection.display_layer_, render_target, selection_states);
    tile_selection.selection_shader_.setParameter("color", selected_tile_hover_color);

    if (tool == EditorTool::TileSelection)
    {
        scene::draw(tile_selection.hover_layer_, render_target, selection_states);
    }

    if (tool == EditorTool::Placement)
    {
        auto placement_states = render_states;
        auto translation = tile_placement.tile_position_;

        placement_states.transform.translate(translation.x, translation.y);
        scene::draw(tile_placement.display_layer_, render_target, placement_states);
    }
}


void TilesMode::key_press_event(QKeyEvent* event)
{
    auto tool = active_tool();
    auto key_modifiers = event->modifiers();

    if (tool == EditorTool::Placement)
    {
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
            features_->tile_placement_.fixed_position_ = canvas()->mouse_position();
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
        std::int32_t offset = 10;

        if (key_modifiers & Qt::ShiftModifier) offset = 5;
        if (key_modifiers & Qt::ControlModifier) offset = 1;

        if (event->key() == Qt::Key_Left)
        {
            move_selected_tiles(core::Vector2i(-offset, 0));
        }

        else if (event->key() == Qt::Key::Key_Right)
        {
            move_selected_tiles(core::Vector2i(offset, 0));
        }

        else if (event->key() == Qt::Key::Key_Up)
        {
            move_selected_tiles(core::Vector2i(0, -offset));
        }

        else if (event->key() == Qt::Key::Key_Down)
        {
            move_selected_tiles(core::Vector2i(0, offset));
        }
    }

    else if (tool == EditorTool::Rotation)
    {
        auto rotation_delta = core::Rotation<double>::degrees(45.0);
        if (key_modifiers & Qt::ShiftModifier) rotation_delta = core::Rotation<double>::degrees(5.0);
        if (key_modifiers & Qt::ControlModifier) rotation_delta = core::Rotation<double>::degrees(1.0);

        if (event->key() == Qt::Key_Left)
        {
            rotate_selected_tiles(-rotation_delta);
        }

        else if (event->key() == Qt::Key_Right)
        {
            rotate_selected_tiles(rotation_delta);
        }
    }
}

void TilesMode::wheel_event(QWheelEvent* event)
{
    if (canvas()->active_tool() == EditorTool::Placement)
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
}

        
void TilesMode::mouse_release_event(QMouseEvent* event)
{
    auto tool = canvas()->active_tool();
    if (event->button() == Qt::LeftButton)
    {
        if (tool == EditorTool::Movement)
        {
            auto& movement = features_->movement_;
            if (movement.fixed_offset_.x != 0 && movement.fixed_offset_.y != 0)
            {
                commit_tile_movement();
            }
        }

        if (tool == EditorTool::Rotation)
        {
            auto& rotation = features_->rotation_;
            if (std::abs(rotation.fixed_rotation_.degrees()) >= 1.0)
            {
                commit_tile_rotation();
            }
        }
    }
}


void TilesMode::mouse_move_event(QMouseEvent* event, core::Vector2i track_point, core::Vector2i track_delta)
{
    auto tool = canvas()->active_tool();
    auto buttons = event->buttons();

    auto mouse_position = canvas()->mouse_position();
    auto modifiers = event->modifiers();

    if ((buttons & Qt::LeftButton) && tool == EditorTool::Rotation)
    {
        auto prior_position = track_point - track_delta;

        auto origin = features_->rotation_.origin_;
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
        auto& tile_placement = features_->tile_placement_;

        if ((modifiers & Qt::AltModifier) == 0)
        {
            tile_placement.fixed_position_ = boost::none;
        }

        auto position = track_point;
        if (tile_placement.fixed_position_)
        {
            auto fixed_position = *tile_placement.fixed_position_;
            auto difference = fixed_position - position;
            if (std::abs(difference.x) < std::abs(difference.y)) position.x = fixed_position.x;
            else position.y = fixed_position.y;
        }

        tile_placement.tile_position_ = position;
    }
}


void TilesMode::initialize_tile_selection()
{
    auto& tile_selection = features_->tile_selection_;
    bool x = tile_selection.selection_shader_.loadFromMemory(selection_vertex_shader_code, selection_fragment_shader_code);
    tile_selection.selection_shader_.setParameter("texture", sf::Shader::CurrentTexture);

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

        tile_selection.tile_group_bounding_boxes_[dummy_tile.id] = bounding_box;
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
    features_->tile_selection_.hover_layer_.clear();

    if (tool == EditorTool::Placement)
    {
        update_tile_placement();
        auto& tile_placement = features_->tile_placement_;

        tile_placement.fixed_position_ = boost::none;
        tile_placement.tile_position_ = canvas()->mouse_position();
    }
}

void TilesMode::goto_next_tile()
{
    const auto& tile_library = scene()->tile_library();
    auto& current_tile = features_->tile_placement_.current_tile;

    current_tile = tile_library.next_tile_group(current_tile->id());
    if (current_tile == nullptr)
    {
        current_tile = tile_library.first_tile_group();
    }

    update_tile_placement();
}

void TilesMode::goto_previous_tile()
{
    const auto& tile_library = scene()->tile_library();
    auto& current_tile = features_->tile_placement_.current_tile;

    current_tile = tile_library.previous_tile_group(current_tile->id());
    if (current_tile == nullptr)
    {
        current_tile = tile_library.last_tile_group();
    }

    update_tile_placement();
}

void TilesMode::goto_tile(components::TileId tile_id)
{
    const auto& tile_library = scene()->tile_library();
    auto& current_tile = features_->tile_placement_.current_tile;

    current_tile = tile_library.next_tile_group(tile_id - 1U);
    if (current_tile == nullptr)
    {
        current_tile = tile_library.first_tile_group();
    }

    update_tile_placement();
}

void TilesMode::initialize_tile_placement()
{
    features_->tile_placement_.current_tile = scene()->tile_library().first_tile_group();
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

    return canvas()->create_layer("Level " + std::to_string(level), level);

}

void TilesMode::place_tile()
{
    auto& tile_placement = features_->tile_placement_;
    auto selected_layer = canvas()->selected_layer();
    if (selected_layer && tile_placement.current_tile)
    {
        components::Tile tile;
        tile.position = tile_placement.tile_position_;
        tile.id = tile_placement.current_tile->id();
        tile.rotation = components::convert_rotation(tile_placement.rotation);

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
    auto& tile_placement = features_->tile_placement_;
    auto& tile_selection = features_->tile_selection_;

    auto selected_layer = canvas()->selected_layer();
    if (selected_layer && tile_placement.current_tile)
    {
        components::Tile tile;
        tile.position = tile_placement.tile_position_;
        tile.id = tile_placement.current_tile->id();
        tile.rotation = components::convert_rotation(tile_placement.rotation);

        std::size_t layer_id = selected_layer.id();
        if (tile.id >= 5000 && selected_layer->level == 0)
        {
            layer_id = acquire_level_layer(2);
            selected_layer = scene()->track().layer_by_id(layer_id);
        }

        std::size_t tile_index = 0;
        if (!tile_selection.selected_tiles_.empty())
        {
            tile_index = tile_selection.selected_tiles_.begin()->first;
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
    auto& tile_placement = features_->tile_placement_;
    tile_placement.rotation += rotation_delta % 360;
    while (tile_placement.rotation < 0)
    {
        tile_placement.rotation += 360;
    }

    while (tile_placement.rotation >= 360)
    {
        tile_placement.rotation -= 360;
    }

    if (round)
    {
        tile_placement.rotation -= tile_placement.rotation % 45;
    }

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

    auto real_position = core::vector2_cast<double>(track_point);

    auto& tile_selection = features_->tile_selection_;
    const auto& bounding_boxes = tile_selection.tile_group_bounding_boxes_;

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
        tile_selection.active_index_ = std::distance(tile_list.begin(), best_match);
        const auto& tile = *best_match;
        const auto& tile_mapping = scene()->tile_mapping();

        tile_selection.hover_layer_ = scene::create_display_layer(&tile, &tile + 1, tile_library, tile_mapping);
        tile_selection_hover_changed(&tile);
    }

    else
    {
        tile_selection.active_index_ = boost::none;
        tile_selection_hover_changed(nullptr);
    }
}

void TilesMode::rebuild_tile_selection_display()
{
    auto& tile_selection = features_->tile_selection_;
    auto selected_layer = canvas()->selected_layer();
    if (!selected_layer)
    {
        tile_selection.display_layer_.clear();
        return;
    }

    const auto& tile_library = scene()->tile_library();
    const auto& tile_mapping = scene()->tile_mapping();

    auto transform_func = [&](const std::pair<const std::size_t, components::Tile>& pair) -> const components::Tile&
    {
        return pair.second;
    };

    auto begin = boost::make_transform_iterator(tile_selection.selected_tiles_.begin(), transform_func);
    auto end = boost::make_transform_iterator(tile_selection.selected_tiles_.end(), transform_func);

    tile_selection.display_layer_ = scene::create_display_layer(begin, end,
        tile_library, tile_mapping);
}

void TilesMode::select_active_tile()
{
    auto& tile_selection = features_->tile_selection_;
    auto selected_layer = canvas()->selected_layer();
    if (selected_layer && tile_selection.active_index_ &&
        *tile_selection.active_index_ < selected_layer->tiles.size())
    {
        std::size_t index = *tile_selection.active_index_;
        std::size_t layer_id = selected_layer.id();

        const auto& tile = selected_layer->tiles[index];

        const auto& old_selection = tile_selection.selected_tiles_;
        auto undo_command = [=]()
        {
            select_tiles(old_selection);
        };

        auto key_modifiers = QApplication::queryKeyboardModifiers();
        if (key_modifiers & Qt::ControlModifier)
        {
            tile_selection.selected_tiles_.emplace(index, tile);
        }

        else if (key_modifiers & Qt::AltModifier)
        {
            tile_selection.selected_tiles_.erase(index);
        }

        else
        {
            tile_selection.selected_tiles_.clear();
            tile_selection.selected_tiles_.emplace(index, tile);
        }

        const auto& new_selection = tile_selection.selected_tiles_;
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
    auto& tile_selection = features_->tile_selection_;
    const auto& selection = tile_selection.selected_tiles_;

    const bool enable_tools = !selection.empty();
    canvas()->enable_tool(EditorTool::Movement, enable_tools);
    canvas()->enable_tool(EditorTool::Rotation, enable_tools);

    tile_selection_changed(tile_selection.selected_tiles_.size());
}

void TilesMode::select_tiles(const std::map<std::size_t, components::Tile>& selection)
{
    features_->tile_selection_.selected_tiles_ = selection;

    rebuild_tile_selection_display();
    compute_rotation_origin();

    tile_selection_changed();
}

void TilesMode::select_tile_range(std::size_t tile_index, std::size_t tile_count)
{
    auto& tile_selection = features_->tile_selection_;
    if (auto selected_layer = canvas()->selected_layer())
    {
        tile_selection.selected_tiles_.clear();

        const auto& tiles = selected_layer->tiles;
        std::size_t end = std::min(tile_index + tile_count, tiles.size());
        for (; tile_index != end; ++tile_index)
        {
            tile_selection.selected_tiles_.emplace(tile_index, tiles[tile_index]);
        }

        rebuild_tile_selection_display();
        compute_rotation_origin();

        tile_selection_changed();
    }
}

void TilesMode::select_tiles_in_area(core::IntRect area)
{
    auto& tile_selection = features_->tile_selection_;
    if (auto selected_layer = canvas()->selected_layer())
    {
        auto& selected_tiles = tile_selection.selected_tiles_;
        selected_tiles.clear();

        for (std::size_t tile_index = 0; tile_index != selected_layer->tiles.size(); ++tile_index)
        {
            const auto& tile = selected_layer->tiles[tile_index];
            if (core::contains(area, tile.position))
            {
                selected_tiles.emplace(tile_index, tile);
            }
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

    auto& tile_placement = features_->tile_placement_;
    auto& tile_rotation = features_->rotation_;

    if (tile_placement.current_tile != nullptr)
    {
        if (tile_rotation.enable_strict_rotations_ && !tile_placement.current_tile->rotatable())
        {
            tile_placement.rotation = 0;
        }

        components::Tile dummy_tile;
        dummy_tile.id = tile_placement.current_tile->id();
        dummy_tile.position.x = 0.0;
        dummy_tile.position.y = 0.0;
        dummy_tile.rotation = components::convert_rotation(tile_placement.rotation);

        tile_placement.display_layer_ = scene::create_display_layer(&dummy_tile, &dummy_tile + 1,
            tile_library, tile_mapping);
    }

    else
    {
        tile_placement.display_layer_.clear();
    }

    update_placement_tile_info(tile_placement.current_tile);
    update_placement_tile_rotation_info(tile_placement.current_tile, tile_placement.rotation);
}

void TilesMode::mouse_press_event(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        auto tool = canvas()->active_tool();
        if (tool == EditorTool::Placement)
        {
            place_tile();
        }

        else if (tool == EditorTool::TileSelection)
        {
            select_active_tile();
        }
    }
}

void TilesMode::move_selected_tiles(core::Vector2i offset, bool fix_position)
{
    if (auto selected_layer = canvas()->selected_layer())
    {
        auto& movement = features_->movement_;
        auto new_offset = movement.fixed_offset_ + offset;
        auto new_real_offset = movement.real_offset_ + offset;

        if (fix_position)
        {
            if (std::abs(new_real_offset.x) < std::abs(new_real_offset.y)) new_offset.x = 0;
            else new_offset.y = 0;

            offset = new_offset - movement.fixed_offset_;
        }

        double offset_x = static_cast<double>(offset.x);
        double offset_y = static_cast<double>(offset.y);

        auto& tile_selection = features_->tile_selection_;

        for (auto& tile : tile_selection.selected_tiles_)
        {
            tile.second.position.x += offset_x;
            tile.second.position.y += offset_y;

            scene()->update_tile_preview(selected_layer.id(), tile.first, tile.second);
        }

        movement.real_offset_ = new_real_offset;
        movement.fixed_offset_ = new_offset;

        tiles_moved(new_offset);
    }
}

void TilesMode::commit_tile_movement()
{
    if (auto selected_layer = canvas()->selected_layer())
    {
        std::size_t layer_id = selected_layer.id();

        const auto& new_state = features_->tile_selection_.selected_tiles_;
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
        tiles_movement_finished();

        features_->movement_ = {};
    }
}

void TilesMode::rotate_selected_tiles(core::Rotation<double> rotation_delta, bool fix_rotation)
{
    if (auto selected_layer = canvas()->selected_layer())
    {
        auto& rotation = features_->rotation_;
        auto origin = rotation.origin_;

        auto new_rotation = rotation.fixed_rotation_ + rotation_delta;
        auto new_real_rotation = rotation.real_rotation_ + rotation_delta;

        if (fix_rotation)
        {
            auto fixed_degrees = std::round(new_real_rotation.degrees() / 45.0) * 45.0;
            new_rotation = components::convert_rotation(fixed_degrees);

            rotation_delta = new_rotation - rotation.fixed_rotation_;
        }

        const auto& tiles = selected_layer->tiles;

        auto& tile_selection = features_->tile_selection_;
        for (auto& tile : tile_selection.selected_tiles_)
        {
            if (tile.first >= tiles.size()) continue;

            const auto& layer_tile = tiles[tile.first];            
            tile.second.rotation = layer_tile.rotation + new_rotation;

            auto position = core::vector2_cast<double>(layer_tile.position);
            auto offset = core::transform_point(position - origin, new_rotation);
            tile.second.position = core::vector2_round<std::int32_t>(origin + offset);

            scene()->update_tile_preview(selected_layer.id(), tile.first, tile.second);
        }

        rotation.real_rotation_ = new_real_rotation;
        rotation.fixed_rotation_ = new_rotation;

        tiles_rotated(new_rotation);
    }
}

void TilesMode::commit_tile_rotation()
{
    if (auto selected_layer = canvas()->selected_layer())
    {
        std::size_t layer_id = selected_layer.id();

        auto& tile_selection = features_->tile_selection_;
        const auto& new_state = tile_selection.selected_tiles_;
        auto prior_state = tile_selection.selected_tiles_;

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
        tiles_rotation_finished();

        features_->rotation_.real_rotation_ = {};
        features_->rotation_.fixed_rotation_ = {};

        rebuild_tile_selection_display();
    }
}

void TilesMode::compute_rotation_origin()
{
    auto& rotation = features_->rotation_;
    auto& tile_selection = features_->tile_selection_;

    const auto& vertices = tile_selection.display_layer_.vertices();
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

        rotation.origin_.x = (min_x + max_x) * 0.5f;
        rotation.origin_.y = (min_y + max_y) * 0.5f;
    }

    else
    {
        rotation.origin_ = {};
    }
}

void TilesMode::delete_last_tile()
{
    auto selected_layer = canvas()->selected_layer();
    if (selected_layer && !selected_layer->tiles.empty())
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


void TilesMode::delete_selection()
{
    if (auto selected_layer = canvas()->selected_layer())
    {
        auto& tile_selection = features_->tile_selection_;
        const auto& selection = tile_selection.selected_tiles_;
        std::size_t layer_id = selected_layer.id();

        auto command = [=]()
        {
            canvas()->select_layer(layer_id);

            for (auto it = selection.rbegin(); it != selection.rend(); ++it)
            {
                scene()->delete_tile(layer_id, it->first);
            }

            select_tiles({});
        };

        auto undo_command = [=]()
        {
            canvas()->select_layer(layer_id);

            for (auto it = selection.rbegin(); it != selection.rend(); ++it)
            {
                scene()->insert_tile(layer_id, it->first, it->second);
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

        auto& tile_selection = features_->tile_selection_;
        const auto& selection = tile_selection.selected_tiles_;
        auto command = [=]()
        {
            canvas()->select_layer(layer_id);

            features_->clipboard_.clear_ = true;
            features_->clipboard_.tiles_.clear();


            for (auto it = selection.rbegin(); it != selection.rend(); ++it)
            {
                scene()->delete_tile(layer_id, it->first);
            }

            for (const auto& tile : selection)
            {
                features_->clipboard_.tiles_.push_back(tile.second);
            }

            clipboard_filled();

            select_tiles({});
        };

        const auto& clipboard = features_->clipboard_;
        auto undo_command = [=]()
        {
            canvas()->select_layer(layer_id);

            features_->clipboard_ = clipboard;
            if (clipboard.tiles_.size() == 0) clipboard_emptied();
            else clipboard_filled();

            for (auto it = selection.rbegin(); it != selection.rend(); ++it)
            {
                scene()->insert_tile(layer_id, it->first, it->second);
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
        auto& clipboard = features_->clipboard_;
        clipboard.clear_ = false;
        clipboard.tiles_.clear();

        for (const auto& tile : features_->tile_selection_.selected_tiles_)
        {
            clipboard.tiles_.push_back(tile.second);
        }

        if (!clipboard.tiles_.empty())
        {
            clipboard_filled();
        }
    }
}

void TilesMode::paste_clipboard(core::Vector2i position)
{
    if (auto selected_layer = canvas()->selected_layer())
    {
        std::size_t layer_id = selected_layer.id();

        const auto& clipboard = features_->clipboard_;
        const auto& selection = features_->tile_selection_.selected_tiles_;

        core::Vector2i average_position = std::accumulate(clipboard.tiles_.begin(), clipboard.tiles_.end(), core::Vector2i(),
            [](core::Vector2i pos, const components::Tile& tile)
        {
            return pos + tile.position;
        });
        
        average_position /= clipboard.tiles_.size();

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
                features_->clipboard_.tiles_.clear();
                clipboard_emptied();
            }

            canvas()->select_layer(layer_id);
            select_tile_range(tile_index, clipboard.tiles_.size());
        };

        auto undo_command = [=]()
        {
            features_->clipboard_ = clipboard;
            clipboard_filled();

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
    auto& tile_selection = features_->tile_selection_;
    auto selected_layer = canvas()->selected_layer();
    if (!tile_selection.selected_tiles_.empty())
    {
        std::size_t layer_id = selected_layer.id();
        const auto& old_selection = tile_selection.selected_tiles_;

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


void TilesMode::fill_area(const FillProperties& properties)
{
    auto& tile_placement = features_->tile_placement_;
    auto selected_layer = canvas()->selected_layer();
    if (selected_layer && tile_placement.current_tile)
    {
        components::FillProperties prop;
        prop.density = properties.density * 0.01;
        prop.position_jitter = properties.position_jitter * 0.01;
        prop.randomize_rotation = properties.randomize_rotation;
        prop.rotation = properties.rotation;

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
        const components::TileGroupDefinition& tile_group = *tile_placement.current_tile;

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

void TilesMode::customize_cursor()
{
    ModeBase::customize_cursor();

    auto tool = active_tool();
    if (tool == EditorTool::Placement)
    {
        canvas()->set_active_cursor(EditorCursor::None);
    }

    else if (tool == EditorTool::TileSelection)
    {
        auto cursor = EditorCursor::MagicWand;
        auto modifiers = QApplication::queryKeyboardModifiers();

        if (modifiers & Qt::ControlModifier) cursor = EditorCursor::MagicWandPlus;
        else if (modifiers & Qt::AltModifier) cursor = EditorCursor::MagicWandMinus;

        canvas()->set_active_cursor(cursor);
    }
}


void TilesMode::tile_selection_changed(std::size_t selected_tile_count)
{
    bool enable_tools = selected_tile_count != 0;

    canvas()->enable_tool(EditorTool::Movement, enable_tools);
    canvas()->enable_tool(EditorTool::Rotation, enable_tools);

    canvas()->enable_deleting(enable_tools);
    canvas()->enable_cutting(enable_tools);
    canvas()->enable_copying(enable_tools);
}


void TilesMode::tile_selection_hover_changed(const components::Tile* tile)
{
    if (tile)
    {
        QString text = "Tile " + QString::number(tile->id);
        canvas()->display_tool_info(text);

        std::int32_t degrees = static_cast<std::int32_t>(std::round(tile->rotation.degrees()));

        QString secondary_text = "x=";
        secondary_text += QString::number(tile->position.x);
        secondary_text += " y=";
        secondary_text += QString::number(tile->position.y);
        secondary_text += " r=";
        secondary_text += QString::number(degrees);
        secondary_text += L'°';
        canvas()->display_secondary_tool_info(secondary_text);
    }

    else
    {
        canvas()->display_tool_info("");
        canvas()->display_secondary_tool_info("");
    }
}

void TilesMode::update_placement_tile_info(const components::TileGroupDefinition* tile_group)
{
    if (tile_group)
    {
        auto text = "Tile " + QString::number(tile_group->id());
        canvas()->display_tool_info(text);
    }

    else
    {
        canvas()->display_tool_info("");
    }
}

void TilesMode::update_placement_tile_rotation_info(const components::TileGroupDefinition* tile_group, std::int32_t rotation)
{
    if (!tile_group || (!tile_group->rotatable() && features_->rotation_.enable_strict_rotations_))
    {
        canvas()->display_secondary_tool_info("Cannot rotate");
    }

    else
    {
        canvas()->display_secondary_tool_info(QString::number(rotation) + L'°');
    }   
}


void TilesMode::tiles_moved(core::Vector2i offset)
{
    QString text = "Offset: ";
    text += QString::number(offset.x);
    text += ", ";
    text += QString::number(offset.y);

    canvas()->display_tool_info(text);
}

void TilesMode::tiles_rotated(core::Rotation<double> delta)
{
    QString text = "Rotation: ";
    text += QString::number(static_cast<int>(delta.degrees()));
    text += L'°';

    canvas()->display_tool_info(text);
}

void TilesMode::tiles_movement_finished()
{
    canvas()->display_tool_info("");
    canvas()->display_secondary_tool_info("");
}

void TilesMode::tiles_rotation_finished()
{
    canvas()->display_tool_info("");
    canvas()->display_secondary_tool_info("");
}

void TilesMode::clipboard_filled()
{
    canvas()->enable_pasting(true);
}

void TilesMode::clipboard_emptied()
{
    canvas()->enable_pasting(false);
}

void TilesMode::enable_strict_rotations(bool enable)
{
    features_->rotation_.enable_strict_rotations_ = enable;

    update_tile_placement();
}

NAMESPACE_INTERFACE_MODES_END