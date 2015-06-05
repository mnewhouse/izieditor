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

#ifndef EDITOR_MODES_HPP
#define EDITOR_MODES_HPP

#include <cstdint>

namespace interface
{
    enum class EditorMode
    {
        None,
        Tiles,
        ControlPoints,
        StartPoints,
        Pit,
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
        Resize = 32,
        All = 63
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

    enum class EditorCursor
    {
        None,
        Default,

        MagicWand,
        MagicWandPlus,
        MagicWandMinus,

        Hand,
        Movement,
        Rotation,
        Resize
    };
};

#endif