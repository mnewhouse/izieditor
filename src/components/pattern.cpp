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

#include "pattern.hpp"
#include "terrain_library.hpp"

#include "core/stream_utility.hpp"

extern "C"
{
#include <png.h>
}

#include <fstream>
#include <array>

namespace components
{
    namespace png
    {
        struct InfoStruct
        {
            InfoStruct() = default;

            InfoStruct(const InfoStruct&) = delete;
            InfoStruct& operator=(const InfoStruct&) = delete;

            png_structp& png_ptr() { return png_ptr_; }
            const png_structp& png_ptr() const { return png_ptr_; }

            png_infop& info_ptr() { return info_ptr_; }
            const png_infop& info_ptr() const { return info_ptr_; };

        protected:
            explicit InfoStruct(png_structp png_ptr, png_infop info_ptr)
                : png_ptr_(png_ptr),
                  info_ptr_(info_ptr)
            {
            }

        private:
            png_structp png_ptr_ = nullptr;
            png_infop info_ptr_ = nullptr;
        };

        struct ReadInfo
            : InfoStruct
        {
            ReadInfo()
            {
                png_ptr() = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
                info_ptr() = png_create_info_struct(png_ptr());
            }

            ~ReadInfo()
            {
                if (info_ptr())
                {
                    png_destroy_info_struct(png_ptr(), &info_ptr());
                }

                if (png_ptr())
                {
                    png_destroy_read_struct(&png_ptr(), nullptr, nullptr);
                }
            }

            explicit operator bool() const
            {
                return png_ptr() && info_ptr();
            }
        };

        struct WriteInfo
            : InfoStruct
        {
            WriteInfo()
            {
                png_ptr() = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
                info_ptr() = png_create_info_struct(png_ptr());
            }

            ~WriteInfo()
            {
                if (info_ptr())
                {
                    png_destroy_info_struct(png_ptr(), &info_ptr());
                }

                if (png_ptr())
                {
                    png_destroy_write_struct(&png_ptr(), nullptr);
                }
            }
        };

        struct ReaderStruct
        {
            const unsigned char* data_;
            const unsigned char* end_;
        };

        struct WriterStruct
        {
            std::basic_ofstream<unsigned char>* out_;
        };

        void read_using_reader_struct(png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead);
        void write_using_writer_struct(png_structp png_ptr, png_bytep out_bytes, png_size_t byte_count);
        void flush_output(png_structp png_ptr);
    }

    PatternLoadError::PatternLoadError(std::string file_path)
        : std::runtime_error("could not load pattern file " + file_path + " (must be a paletted PNG image)"),
        file_path_(std::move(file_path))
    {
    }

    Pattern::Pattern(const std::string& file_name, core::IntRect rect)
    {
        load_from_file(file_name, rect);
    }

    Pattern::Pattern(core::Vector2u size)
        : size_(size), bytes_(size.x * size.y, 0)
    {
    }

    Pattern::Pattern(Pattern&& other)
        : size_(other.size_),
        bytes_(std::move(other.bytes_))
    {
        other.size_ = {};
    }

    Pattern& Pattern::operator=(Pattern&& other)
    {
        size_ = other.size_;
        bytes_ = std::move(other.bytes_);

        other.size_ = {};
        return *this;
    }

    void png::read_using_reader_struct(png_structp png_ptr, png_bytep out_bytes, png_size_t byte_count)
    {
        auto* reader = static_cast<ReaderStruct*>(png_ptr->io_ptr);
        if (reader)
        {
            png_size_t bytes_left = reader->end_ - reader->data_;
            if (bytes_left >= byte_count)
            {
                std::copy(reader->data_, reader->data_ + byte_count, out_bytes);
                reader->data_ += byte_count;
            }
        }
    }

    void png::write_using_writer_struct(png_structp png_ptr, png_bytep out_bytes, png_size_t byte_count)
    {
        auto writer = static_cast<WriterStruct*>(png_ptr->io_ptr);
        if (writer)
        {
            writer->out_->write(out_bytes, byte_count);
        }
    }

    void png::flush_output(png_structp png_ptr)
    {
        auto writer = static_cast<WriterStruct*>(png_ptr->io_ptr);
        if (writer)
        {
            writer->out_->flush();
        }
    }

    void Pattern::load_from_file(const std::string& file_name, core::IntRect rect)
    {
        std::basic_ifstream<unsigned char> stream(file_name, std::ifstream::in | std::ifstream::binary);
        if (stream)
        {
            auto file_contents = core::read_stream_contents(stream);
            if (file_contents.size() && png_check_sig(file_contents.data(), 8))
            {
                png::ReadInfo png_info;
                auto& read_ptr = png_info.png_ptr();
                auto& info_ptr = png_info.info_ptr();

                if (png_info && setjmp(png_jmpbuf(read_ptr)) == 0)
                {
                    png::ReaderStruct reader;
                    reader.data_ = file_contents.data() + 8;
                    reader.end_ = file_contents.data() + file_contents.size();

                    png_set_read_fn(read_ptr, static_cast<void*>(&reader), png::read_using_reader_struct);
                    png_set_sig_bytes(read_ptr, 8);
                    png_read_info(read_ptr, info_ptr);

                    // Must be paletted image
                    if (png_get_color_type(read_ptr, info_ptr) == PNG_COLOR_TYPE_PALETTE)
                    {
                        std::size_t image_width = png_get_image_width(read_ptr, info_ptr);
                        std::size_t image_height = png_get_image_height(read_ptr, info_ptr);

                        std::vector<png_byte> byte_array(image_width * image_height);
                        std::vector<png_bytep> row_pointers(image_height);

                        for (std::size_t row = 0; row != image_height; ++row)
                        {
                            row_pointers[row] = &byte_array[row * image_width];
                        }

                        if (rect.width == 0)
                        {
                            rect.left = 0;
                            rect.width = image_width;
                        }

                        if (rect.height == 0)
                        {
                            rect.top = 0;
                            rect.height = image_height;
                        }

                        png_read_image(read_ptr, row_pointers.data());
                        png_read_end(read_ptr, info_ptr);

                        if (image_width >= static_cast<std::size_t>(rect.right()) && image_height >= static_cast<std::size_t>(rect.bottom()))
                        {
                            // Resize
                            resize(rect.width, rect.height);

                            // And copy the bytes over
                            auto* data_ptr = &bytes_[0];
                            for (std::uint32_t y = rect.top, bottom = rect.bottom(); y != bottom; ++y)
                            {
                                auto source_ptr = &byte_array[y * image_width + rect.left];

                                for (auto row_end = data_ptr + rect.width; data_ptr != row_end; ++data_ptr, ++source_ptr)
                                {
                                    *data_ptr = *source_ptr;
                                }
                            }

                            // Only if we reach this point, we can avoid throwing an exception.
                            // Pretty god-awful, so many levels of indentation.
                            return;
                        }
                    }
                }
            }
        }

        throw PatternLoadError(file_name);
    }

    Pattern::const_iterator Pattern::begin() const
    {
        return bytes_.data();
    }

    Pattern::const_iterator Pattern::end() const
    {
        return bytes_.data();
    }

    Pattern::const_iterator Pattern::row_begin(std::uint32_t y) const
    {
        return bytes_.data() + y * size_.x;
    }

    Pattern::const_iterator Pattern::row_end(std::uint32_t y) const
    {
        return bytes_.data() + y * size_.x + size_.x;
    }


    Pattern::iterator Pattern::begin()
    {
        return bytes_.data();
    }

    Pattern::iterator Pattern::end()
    {
        return bytes_.data();
    }

    Pattern::iterator Pattern::row_begin(std::uint32_t y)
    {
        return bytes_.data() + y * size_.x;
    }

    Pattern::iterator Pattern::row_end(std::uint32_t y)
    {
        return bytes_.data() + y * size_.x + size_.x;
    }

    const TerrainId& Pattern::operator()(std::uint32_t x, std::uint32_t y) const
    {
        return bytes_[x + y * size_.x];
    }

    TerrainId& Pattern::operator()(std::uint32_t x, std::uint32_t y)
    {
        return bytes_[x + y * size_.x];
    }

    core::Vector2u Pattern::size() const
    {
        return size_;
    }

    void Pattern::resize(core::Vector2u new_size)
    {
        bytes_.resize(new_size.x * new_size.y);

        size_.x = new_size.x;
        size_.y = new_size.y;
    }

    void Pattern::resize(std::uint32_t width, std::uint32_t height)
    {
        resize(core::Vector2u(width, height));
    }

    std::array<png_color, 256> create_palette(const TerrainLibrary& terrain_library)
    {
        std::array<png_color, 256> palette;
        for (std::uint32_t terrain_id = 0; terrain_id != 256; ++terrain_id)
        {
            const auto& terrain = terrain_library.terrain_by_id(terrain_id);
            palette[terrain_id].red = terrain.red;
            palette[terrain_id].green = terrain.green;
            palette[terrain_id].blue = terrain.blue;
        }

        return palette;
    }

    PatternSaveError::PatternSaveError(const std::string& file_name)
        : std::runtime_error("could not save pattern file to " + file_name)
    {

    }

    void save_pattern(const Pattern& pattern, const TerrainLibrary& terrain_library, const std::string& file_name)
    {
        std::basic_ofstream<unsigned char> out(file_name, std::ios::binary | std::ios::out);

        if (!out) throw PatternSaveError(file_name);
        png::WriterStruct writer{};
        writer.out_ = &out;
        png::WriteInfo write_info{};

        auto& png_ptr = write_info.png_ptr();
        auto& info_ptr = write_info.info_ptr();

        png_set_write_fn(png_ptr, &writer, png::write_using_writer_struct, png::flush_output);

        auto pattern_size = pattern.size();


        std::vector<png_bytep> pattern_rows(pattern_size.y);
        for (std::uint32_t y = 0; y != pattern_size.y; ++y)
        {
            // Oh my.
            pattern_rows[y] = const_cast<png_byte*>(pattern.row_begin(y));
        }

        png_set_IHDR(png_ptr, info_ptr, pattern_size.x, pattern_size.y, 8, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

        auto palette = create_palette(terrain_library);
        png_set_PLTE(png_ptr, info_ptr, palette.data(), palette.size());

        png_write_info(png_ptr, info_ptr);
        png_write_image(png_ptr, pattern_rows.data());
        png_write_end(png_ptr, info_ptr);
    }
}