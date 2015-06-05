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

#include "pattern_store.hpp"
#include "pattern.hpp"

#include "tile_library.hpp"
#include "tile_definition.hpp"

namespace components
{
    std::shared_ptr<Pattern> PatternStore::load_from_file(const std::string& file_name)
    {
        auto it = loaded_patterns_.find(file_name);
        if (it != loaded_patterns_.end()) {
            return it->second;
        }

        auto pattern = std::make_shared<Pattern>(file_name);
        loaded_patterns_.insert(std::make_pair(file_name, pattern));

        return pattern;
    }

    PatternStore load_pattern_files(const TileLibrary& tile_library)
    {
        PatternStore result;

        for (auto tile = tile_library.first_tile(); tile; tile = tile_library.next_tile(tile->id))
        {
            result.load_from_file(tile->pattern_file);
        }

        return result;
    }
}