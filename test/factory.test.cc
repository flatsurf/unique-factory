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

struct UniqueInt : public std::enable_shared_from_this<UniqueInt> {
  UniqueInt(int value): value(value) {}

  int value;
};

TEST_CASE("Factory", "[factory]"){
  SECTION("Values are Cached") {
    UniqueFactory<int, UniqueInt> factory;
    const auto cached = factory.get(0, []() { return new UniqueInt{0}; });

    REQUIRE(factory.get(0, []() { return new UniqueInt{1}; })->value == 0);

    REQUIRE_NOTHROW(factory.get(0, []() -> UniqueInt* { throw new std::logic_error("should be created from cache"); }));

    REQUIRE(factory.get(0, []() { return new UniqueInt{0}; }) == cached);
  }

  SECTION("Values are not Kept Alive") {
    UniqueFactory<int, UniqueInt> factory;
    factory.get(0, []() { return new UniqueInt{0}; });

    REQUIRE(factory.get(0, []() { return new UniqueInt{1}; })->value == 1);
  }

  SECTION("Values can be Kept Alive") {
    UniqueFactory<int, UniqueInt, KeepSetAlive<UniqueInt, 1>> factory;

    factory.get(0, []() { return new UniqueInt{0}; });

    REQUIRE(factory.get(0, []() { return new UniqueInt{1}; })->value == 0);

    factory.get(1, []() { return new UniqueInt{0}; });

    REQUIRE(factory.get(0, []() { return new UniqueInt{1}; })->value == 1);
  }

  SECTION("Factory can be Destroyed before the Values") {
    std::shared_ptr<const UniqueInt> value;

    {
      UniqueFactory<int, UniqueInt> factory;

      value = factory.get(0, []() { return new UniqueInt{0}; });
    }
  }

  SECTION("Factory can handle Failures") {
    UniqueFactory<int, UniqueInt, KeepSetAlive<UniqueInt, 1>> factory;

    REQUIRE_THROWS_AS(factory.get(0, []() -> UniqueInt* { throw std::logic_error("failure"); }), std::logic_error);

    REQUIRE(factory.get(0, []() { return new UniqueInt{0}; })->value == 0);
  }
}

}
}
