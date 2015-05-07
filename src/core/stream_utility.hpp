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

#ifndef STREAM_UTILITY_HPP
#define STREAM_UTILITY_HPP

#include <istream>
#include <vector>

namespace core
{
    template <typename CharType>
    std::vector<CharType> read_stream_contents(std::basic_istream<CharType>& stream);

    template <typename CharType = unsigned char>
    std::vector<CharType> read_file_contents(const std::string& file_name);
}   

template <typename CharType>
std::vector<CharType> core::read_stream_contents(std::basic_istream<CharType>& stream)
{
    auto current_pos = stream.tellg();
    stream.seekg(0, std::istream::end);

    auto num_bytes = static_cast<std::size_t>(stream.tellg() - current_pos);

    stream.seekg(current_pos);
    std::vector<CharType> result(num_bytes);

    stream.read(result.data(), num_bytes);

    return result;
}

template <typename CharType>
std::vector<CharType> core::read_file_contents(const std::string& file_name)
{
    std::ifstream stream(file_name, std::istream::in | std::istream::binary);
    return read_stream_contents(stream);
}

#endif