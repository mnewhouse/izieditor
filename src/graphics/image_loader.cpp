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

#include "image_loader.hpp"

#include <fstream>

namespace graphics
{
    ImageLoadError::ImageLoadError(std::string file_path)
        : std::runtime_error("failed to open image " + file_path),
        file_path_(std::move(file_path))
    {
    }

    const std::string& ImageLoadError::file_path() const
    {
        return file_path_;
    }

    const sf::Image* ImageLoader::load_from_file(const std::string& file_name, std::nothrow_t)
    {
        auto it = image_map_.find(file_name);
        if (it != image_map_.end())
        {
            return &it->second;
        }

        return load_from_file_impl(file_name);
    }

    const sf::Image& ImageLoader::load_from_file(const std::string& file_name)
    {
        auto* image = load_from_file(file_name, std::nothrow);
        if (!image)
        {
            throw ImageLoadError(file_name);
        }

        return *image;
    }

    const sf::Image* ImageLoader::load_from_file_impl(const std::string& file_name)
    {
        std::ifstream stream(file_name, std::ios::binary | std::ios::in);
        if (stream)
        {
            stream.seekg(0, std::ios::end);
            std::size_t size = static_cast<std::size_t>(stream.tellg());
            file_buffer_.resize(size);

            stream.seekg(0);
            stream.read(file_buffer_.data(), file_buffer_.size());

            auto& image = image_map_[file_name];
            if (image.loadFromMemory(file_buffer_.data(), file_buffer_.size()))
            {
                return &image;
            }

            image_map_.erase(file_name);
        }

        return nullptr;
    }
}
