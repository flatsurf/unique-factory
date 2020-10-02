/**********************************************************************
 *  This file is part of unique-factory.
 *
 *        Copyright (C) 2020 Julian Rüth
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

#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace {

namespace unique_factory {

template <typename Key, typename Value, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>>
class UniqueFactory {
  std::mutex mutex;

  std::unordered_map<Key, std::weak_ptr<Value>, Hash, KeyEqual> cache;

  class Deleter {
    UniqueFactory* factory;
    Key key;

   public:
    Deleter(UniqueFactory* factory, const Key& key) :
      factory(factory),
      key(key) {}

    void operator()(Value* value) const {
      factory->cache.erase(key);
      delete value;
    }
  };

 public:
  UniqueFactory() = default;
  UniqueFactory(const UniqueFactory&) = delete;
  UniqueFactory(UniqueFactory&&) = delete;

  ~UniqueFactory() {
    assert(cache.size() == 0 && "UniqueFactory is leaking memory.");
  }
  
  UniqueFactory& operator=(const UniqueFactory&) = delete;
  UniqueFactory& operator=(UniqueFactory&&) = delete;

  std::shared_ptr<Value> get(const Key& key, std::function<Value*()> create) {
    std::lock_guard<std::mutex> lock(mutex);

    std::shared_ptr<Value> ret;

    auto cached = cache.find(key);
    if (cached != cache.end()) {
      ret = cached->second.lock();
    } else {
      ret = std::shared_ptr<Value>(create(), Deleter(this, key));
      cache[key] = ret;
    }

    return ret;
  }
};

}

}

#endif
