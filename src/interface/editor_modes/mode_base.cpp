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

#include "mode_base.hpp"

#include "../editor_canvas.hpp"

namespace interface
{
    ModeBase::ModeBase(EditorCanvas* canvas)
        : canvas_(canvas)
    {
    }

    EditorTool ModeBase::active_tool() const
    {
        return canvas()->active_tool();
    }

    void ModeBase::initialize(scene::Scene* scene)
    {
        scene_ = scene;

        on_initialize(scene);
    }

    void ModeBase::activate()
    {
        canvas()->clear_tool_info();
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
        conditional_enable(EditorTool::Resize);

        on_activate();
    }

    std::uint32_t ModeBase::enabled_tools() const
    {
        return static_cast<std::uint32_t>(EditorTool::All);
    }

    void ModeBase::customize_cursor()
    {
        auto tool = active_tool();
        if (tool == EditorTool::Placement || tool == EditorTool::AreaSelection)
        {
            canvas()->set_active_cursor(EditorCursor::Default);
        }

        else if (tool == EditorTool::Movement)
        {
            canvas()->set_active_cursor(EditorCursor::Movement);
        }
        
        else if (tool == EditorTool::Rotation)
        {
            canvas()->set_active_cursor(EditorCursor::Rotation);
        }

        else if (tool == EditorTool::TileSelection)
        {
            canvas()->set_active_cursor(EditorCursor::MagicWand);
        }

        else if (tool == EditorTool::Resize)
        {
            canvas()->set_active_cursor(EditorCursor::Resize);
        }
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
}