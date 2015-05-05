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

#include "include_path.hpp"

#include <boost/filesystem.hpp>

namespace components
{
    std::string find_include_path(const std::string& file_name, std::initializer_list<std::string> search_paths)
    {
        auto result = find_include_directory(file_name, search_paths);
        if (result.empty())
        {
            return result;
        }

        boost::filesystem::path path = result;
        path /= file_name;
        return path.string();
    }

    std::string find_include_directory(const std::string& file_name, std::initializer_list<std::string> search_paths)
    {
        for (const auto& path : search_paths)
        {
            boost::filesystem::path full_path = path;
            full_path /= file_name;

            if (is_regular_file(full_path)) return path;
        }

        return std::string();
    }
}