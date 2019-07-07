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

#ifndef LIBUNIQUEFACTORY_UNIQUE_FACTORY_HPP
#define LIBUNIQUEFACTORY_UNIQUE_FACTORY_HPP

#include <cassert>
#include <functional>
#include <list>
#include <memory>
#include <mutex>

using std::function;
using std::list;
using std::pair;
using std::shared_ptr;
using std::tuple;
using std::weak_ptr;

namespace exactreal {
template <typename T>
struct is_shared_ptr : std::false_type {
  using weak = T;
};

template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {
  using weak = std::weak_ptr<T>;
};

template <typename T>
struct is_weak_ptr : std::false_type {};

template <typename T>
struct is_weak_ptr<std::weak_ptr<T>> : std::true_type {};

template <typename T>
struct is_unique_ptr : std::false_type {};

template <typename T>
struct is_unique_ptr<std::unique_ptr<T>> : std::true_type {};

// A thread-safe factory that creates unique cached objects. This is
// essentially a reimplementation of SageMath's UniqueFactory. The keys and
// values are stored with weak_ptr so this factory does neither keep keys nor
// values alive.
//
// We are not really using the weakness of the keys yet. For it to really make
// sense, we would have to turn things such as vector<shared_ptr> into
// vector<weak_ptr> …. But maybe we don't actually need that part and could
// simplify this quite a bit.
template <typename V, typename... K>
class UniqueFactory {
  std::mutex mutex;

  using WeakKey = tuple<typename is_shared_ptr<K>::weak...>;
  using KeyValuePair = pair<WeakKey, weak_ptr<V>>;
  list<KeyValuePair> cache;

  template <typename T>
  static bool isAlive(const T& ptr) {
    if constexpr (is_weak_ptr<T>::value) {
      return !ptr.expired;
    } else {
      return true;
    }
  }

  template <typename T>
  static decltype(auto) target(T&& ptr) {
    if constexpr (is_weak_ptr<T>::value) {
      assert(!ptr.expired);
      assert(ptr != nullptr);
      return *ptr;
    } else {
      return std::forward<T>(ptr);
    }
  }

  bool isValidKey(const WeakKey& key) {
    return std::apply([](auto&&... args) {
      return (isAlive(args) && ...);
    },
                      key);
  }

  bool eq(const WeakKey& a, const WeakKey& b) {
    // The use of tuple here is probably not a good idea as it has to invoke
    // copy constructors on everything which might be expensive.
    return std::apply([](auto&&... args) { return tuple(target(std::forward<decltype(args)>(args))...); }, a) ==
           std::apply([](auto&&... args) { return tuple(target(std::forward<decltype(args)>(args))...); }, b);
  }

 public:
  UniqueFactory() {
    static_assert(!is_weak_ptr<V>::value && !is_shared_ptr<V>::value && !is_unique_ptr<V>::value && !std::is_pointer<V>::value, "V must be a non-pointer type");
    static_assert(std::conjunction_v<std::negation<is_weak_ptr<K>>...>, "K must be not contain weak pointers");
  }

  UniqueFactory(const UniqueFactory&) = delete;
  UniqueFactory(UniqueFactory&&) = delete;
  UniqueFactory& operator=(const UniqueFactory&) = delete;
  UniqueFactory& operator=(UniqueFactory&&) = delete;

  shared_ptr<V> get(const K&... k, function<V*(const K&... k)> create) {
    std::lock_guard<std::mutex> lock(mutex);

    WeakKey key{k...};
    assert(isValidKey(key) && "Parts of the key were garbage collected while creating its entry in the cache which shouldn't be easily possible in C++.");

    auto it = cache.begin();
    while (it != cache.end()) {
      if (!isValidKey(it->first) || it->second.expired()) {
        auto jt = it;
        it++;
        cache.erase(jt);
        continue;
      }

      if (eq(it->first, key)) {
        return static_cast<shared_ptr<V>>(it->second);
      }

      ++it;
    }

    shared_ptr<V> ret(create(k...));
    cache.emplace_front(KeyValuePair(key, weak_ptr<V>(ret)));
    return ret;
  }
};

}  // namespace exactreal

#endif
