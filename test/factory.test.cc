/**********************************************************************
 *  This file is part of unique-factory.
 *
 *        Copyright (C) 2019-2022 Julian Rüth
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *********************************************************************/

#include "external/catch2/single_include/catch2/catch.hpp"

#include "../unique-factory/unique-factory.hpp"

namespace unique_factory {
namespace test {

TEST_CASE("Factory", "[factory]"){
  SECTION("Values are Cached") {
    UniqueFactory<int, int> factory;
    const auto cached = factory.get(0, []() { return new int{0}; });

    REQUIRE(*factory.get(0, []() { return new int{1}; }) == 0);

    REQUIRE_NOTHROW(factory.get(0, []() -> int* { throw new std::logic_error("should be created from cache"); }));

    REQUIRE(factory.get(0, []() { return new int{0}; }) == cached);
  }

  SECTION("Values are not Kept Alive") {
    UniqueFactory<int, int> factory;
    factory.get(0, []() { return new int{0}; });

    REQUIRE(*factory.get(0, []() { return new int{1}; }) == 1);
  }
}

}
}
