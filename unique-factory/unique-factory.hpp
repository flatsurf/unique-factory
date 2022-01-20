/**********************************************************************
 *  This file is part of unique-factory.
 *
 *        Copyright (C) 2020-2022 Julian Rüth
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

#ifndef LIBUNIQUEFACTORY_UNIQUE_FACTORY_HPP
#define LIBUNIQUEFACTORY_UNIQUE_FACTORY_HPP

#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace unique_factory {

namespace {

/// Keeps up to `history` elements alive.
/// This is the default behavior of `UniqueFactory`, i.e.,  elements created by
/// `UniqueFactory` are destroyed as soon as all strong references to them have
/// gone out of scope.
class KeepNothingAlive {
 public:
  template <typename Value>
  void insert(const std::shared_ptr<Value>&) {}

  void clear() {}
};

/// Keeps up to `history` elements alive.
/// This can be set as a template parameter of `UniqueFactory` so that repeated
/// calls to `UniqueFactory.get()` do not need to recreate objects if they have
/// gone out of scope in the meantime.
/// This simply holds on to up to `history` many shared pointers to `Value`.
/// This makes sure that weak pointers to these elements are kept alive.
template <typename Value, size_t history>
class KeepSetAlive {
  std::unordered_set<std::shared_ptr<Value>> workingSet;

 public:
  void insert(const std::shared_ptr<Value>& value) {
    if (workingSet.size() >= history)
      clear();

    workingSet.insert(value);
  }

  void clear() {
    workingSet.clear();
  }
};

/// A cache that maps `Key` to `std::shared_ptr<Value>`.
template <typename Key, typename Value, typename KeepAlive = KeepNothingAlive, typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
class UniqueFactory : KeepAlive {
  std::mutex mutex;

  std::unordered_map<Key, std::weak_ptr<Value>, Hash, KeyEqual> cache;

  class Deleter {
    UniqueFactory *factory;
    Key key;

  public:
    Deleter(UniqueFactory *factory, const Key &key)
        : factory(factory), key(key) {}

    void operator()(Value *value) const {
      if (factory != nullptr) {
        factory->cache.erase(key);
      }
      delete value;
    }

    /// Caled when the factory that created this elemnt was destroyed before
    /// all references to this element were destroyed.
    /// When this element eventually gets destroyed it cannot update its factory anymore.
    void orphan() {
      factory = nullptr;
    }
  };

public:
  UniqueFactory() = default;
  UniqueFactory(const UniqueFactory &) = delete;
  UniqueFactory(UniqueFactory &&) = delete;

  ~UniqueFactory() {
    KeepAlive::clear();
    if (cache.size() != 0) {
#ifndef NDEBUG
      std::cerr << "A unique factory is probably leaking memory. " << cache.size()
                << " objects were created through a C++ unique factory but "
                   "had not been released when the factory was released. These objects might be part of a "
                   "legitimate cache that is (unfortunately) not explicitly "
                   "released upon program termination as is common in "
                   "garbage-collecting languages such as Python."
                << std::endl;
#endif
      for (const auto& leaked : cache)
        std::get_deleter<Deleter>(leaked.second.lock())->orphan();
    }
  }

  UniqueFactory &operator=(const UniqueFactory &) = delete;
  UniqueFactory &operator=(UniqueFactory &&) = delete;

  /// Return a shared pointer to the value with `key`.
  /// If no such value can be found in the cache, one is created by invoking
  /// `create()` first.
  std::shared_ptr<Value> get(const Key &key, std::function<Value *()> create) {
    std::lock_guard<std::mutex> lock(mutex);

    std::shared_ptr<Value> ret;

    auto cached = cache.find(key);
    if (cached != cache.end()) {
      ret = cached->second.lock();
    } else {
      ret = std::shared_ptr<Value>(create(), Deleter(this, key));
      cache[key] = ret;
    }

    KeepAlive::insert(ret);

    return ret;
  }
};

} // namespace unique_factory

} // namespace

#endif
