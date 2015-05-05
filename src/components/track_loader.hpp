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

#pragma once

#ifndef TRACK_LOADER_HPP
#define TRACK_LOADER_HPP

#include <exception>
#include <string>
#include <vector>
#include <istream>
#include <memory>

namespace components
{
    class Track;

    struct BrokenTrackException
        : public std::runtime_error
    {
        BrokenTrackException(std::string missing_file);

    private:
        std::string missing_file_;
    };

    class TrackLoader
    {
    public:
        TrackLoader();
        ~TrackLoader();

        void load_from_file(const std::string& file_name);
        void load_from_stream(std::istream& stream, std::string working_directory);

        Track get_result();
        const std::vector<std::string>& assets() const;

        void include(const std::string& file_name);

        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}

#endif