/**********************************************************************
 *  This file is part of unique-factory.
 *
 *        Copyright (C) 2019 Julian Rüth
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

#include <benchmark/benchmark.h>
#include <gtest/gtest.h>
#include <boost/lexical_cast.hpp>
#include <memory>

#include <unique_factory.hpp>

using unique_factory::UniqueFactory;

TEST(Factory, NonPointerKeyNonPointerValue) {
  // A factory int -> int
  using Key = int;
  using Value = int;
  UniqueFactory<Value, Key> factory;
  EXPECT_EQ(0, factory.get(0, []() { return 0; }));
  // Nothing ever gets dropped from the cache automatically since we are not
  // using shared/weak_ptr on either key or value.
  EXPECT_EQ(0, factory.get(0, []() { return 1; }));
}

TEST(Factory, NonPointerKeySharedValue) {
  // A factory int -> shared_ptr<int>
  using Key = int;
  using  Value = std::shared_ptr<int>;
  // This means you are likely doing something wrong, nothing ever gets
  // dropped so you probably wanted to specify a weak_ptr<int> as the value
  // type.
  UniqueFactory<Value, Key> factory;

  EXPECT_EQ(0, *factory.get(0, []() { return std::make_shared<int>(0); }));
  // Nothing ever gets dropped from the cache automatically since we are not
  // using shared/weak_ptr on either key or value.
  EXPECT_EQ(0, *factory.get(0, []() { return std::make_shared<int>(1); }));
}

TEST(Factory, NonPointerKeyWeakValue) {
  // A factory int -> shared_ptr<int> that does not keep values alive; for
  // technical reasons, a shared_ptr is returned and not a weak_ptr, namely,
  // we hold a weak_ptr of the value, if we returned a weak_ptr, the value
  // would be garbage collected at that moment; if you have some bizarre
  // reason to want a weak_ptr, create one from the returned value.
  using Key = int;
  using  Value = std::weak_ptr<int>;
  UniqueFactory<Value, Key> factory;

  EXPECT_EQ(0, *factory.get(0, []() { return std::make_shared<int>(0); }));
  // The value is garbage collected if nothing keeps it alive anymore.
  EXPECT_EQ(1, *factory.get(0, []() { return std::make_shared<int>(1); }));
  // If something holds a reference to the value, it stays alive.
  auto value = factory.get(0, []() { return std::make_shared<int>(0); });
  EXPECT_EQ(0, *value);
  EXPECT_EQ(0, *factory.get(0, []() { return std::make_shared<int>(1); }));
}

TEST(Factory, SharedKeyNonPointerValue) {
  // A factory shared_ptr<int> -> int
  using Key = shared_ptr<int>;
  using Value = int;
  // Just like with a shared_ptr value, a shared_ptr key has no special
  // semantics different from a non-pointer.
  UniqueFactory<Value, Key> factory;
  EXPECT_EQ(0, factory.get(std::make_shared<int>(0), []() { return 0; }));
  // Shared pointers are compared by their address so the above entry is now
  // forever lost inside the factory.
  EXPECT_EQ(1, factory.get(std::make_shared<int>(0), []() { return 1; }));

  auto key = std::make_shared<int>(0);
  EXPECT_EQ(2, factory.get(key, []() { return 2; }));
  EXPECT_EQ(2, factory.get(key, []() { return 3; }));
}

TEST(Factory, WeakKeyNonPointerValue) {
  // A factory weak_ptr<int> -> int that only keep the value alive as long as
  // the key is.
  using Key = weak_ptr<int>;
  using Value = int;
  UniqueFactory<Value, Key> factory;
  auto key = std::make_shared<int>(0);
  auto weak = Key(key);
  EXPECT_EQ(0, factory.get(weak, []() { return 0; }));
  // The value stays alive while the key is.
  EXPECT_EQ(0, factory.get(Key(key), []() { return 1; }));

  key.reset();
    
  // The factory does not keep the key alive.
  EXPECT_TRUE(weak.expired());
}

TEST(Factory, WeakKeyWeakValue) {
  // A factory weak_ptr<int> -> shared_ptr<int> that does not keep the value
  // alive and also only as long as the key is.
  using Key = weak_ptr<int>;
  using Value = weak_ptr<int>;
  UniqueFactory<Value, Key> factory;
  auto key = std::make_shared<int>(0);
  auto weak = Key(key);

  EXPECT_EQ(0, *factory.get(weak, []() { return 0; }));
  // The value is not being kept alive.
  EXPECT_EQ(1, *factory.get(weak, []() { return 1; }));
  
  auto value = factory.get(weak, []() { return 0; });
  EXPECT_EQ(0, *value);
  EXPECT_EQ(0, *factory.get(weak, []() { return 1; }));

  // But the value goes away when the key does.
  key.reset();
  auto weak_value = Value(value);
  EXPECT_FALSE(weak_value.expired());
  value.reset();
  EXPECT_TRUE(weak_value.expired());
}

TEST(Factory, MixedKeyNonPointerValue) {
  // A factory (weak_ptr<int>, int) -> int
  using Value = int;
  UniqueFactory<Value, weak_ptr<int>, int> factory;

  auto key = std::make_shared<int>(0);
  auto weak = std::shared_ptr<int>(key);

  EXPECT_EQ(0, factory.get(weak, 0, []() { return 0; }));
  // Values are cached while the key is alive
  EXPECT_EQ(0, factory.get(weak, 0, []() { return 1; }));

  // When any part of the key dies, the entry is gone from the cache.
  key.reset();
  EXPECT_EQ(1, factory.get(weak, 1, []() { return 1; }));

  // All parts of the key are taken into account.
  EXPECT_EQ(2, factory.get(weak, 2, []() { return 2; }));
}

#include "main.hpp"
