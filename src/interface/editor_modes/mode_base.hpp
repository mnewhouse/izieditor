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

#ifndef MODE_BASE_HPP
#define MODE_BASE_HPP

#include "../editor_modes.hpp"

#include "core/vector2.hpp"

#include <qevent.h>

#include <cstddef>

namespace scene
{
    class Scene;
}

#define NAMESPACE_INTERFACE_MODES namespace interface { namespace modes {
#define NAMESPACE_INTERFACE_MODES_END } }

namespace interface
{
    class EditorCanvas;

    struct ModeBase
    {
    public:
        ModeBase(EditorCanvas* canvas);

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

        virtual void customize_cursor();

        EditorTool active_tool() const;

        virtual void key_press_event(QKeyEvent* key_event) {}
        virtual void key_release_event(QKeyEvent* event) {}
        virtual void mouse_press_event(QMouseEvent* mouse_event) {}
        virtual void mouse_release_event(QMouseEvent* mouse_event) {}
        virtual void mouse_move_event(QMouseEvent* mouse_event, core::Vector2i track_point, core::Vector2i track_delta) {}
        virtual void wheel_event(QWheelEvent* wheel_event) {}

    private:
        virtual void on_initialize(scene::Scene* scene) {}
        virtual void on_activate() {}
        virtual void on_deactivate() {}
        virtual std::uint32_t enabled_tools() const;

        EditorCanvas* canvas_;
        scene::Scene* scene_ = nullptr;
        bool is_active_ = false;
    };
}

#endif