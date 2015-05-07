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

#ifndef PIT_MODE_HPP
#define PIT_MODE_HPP

#include "mode_base.hpp"

#include "core/vector2.hpp"

#include <qevent.h>

#include <SFML/Graphics.hpp>

#include <boost/optional.hpp>

#include <cstdint>

NAMESPACE_INTERFACE_MODES

struct PitMode
    : ModeBase
{
public:
    PitMode(EditorCanvas* canvas);

    void render(sf::RenderTarget& render_target, sf::RenderStates render_states);
    void mouse_press_event(QMouseEvent* event);

    void define_pit(core::Vector2i start, core::Vector2i end);
    void undefine_pit();

private:
    virtual std::uint32_t enabled_tools() const override;
    virtual void on_activate() override;

    boost::optional<core::Vector2i> pit_start_;
};

NAMESPACE_INTERFACE_MODES_END

#endif