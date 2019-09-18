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

namespace {
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
// essentially a reimplementation of SageMath's UniqueFactory. The values are
// stored with a weak_ptr so they are not kept alive by the factory. However,
// the keys are stored without a weak_ptr so the factory does keep the keys
// alive until the values disappear.

template <typename V, typename... K>
class UniqueFactory {
  std::mutex mutex;

  using Key = tuple<K...>;
  static constexpr bool weakKeys = std::disjunction_v<is_weak_ptr<K>...>;
  using Value = std::conditional_t<weakKeys, std::shared_ptr<V>, std::weak_ptr<V>>;
  using KeyValuePair = pair<Key, Value>;
  list<KeyValuePair> cache;

  template <typename T>
  static bool isAlive(const Value& value) {
    return !value.expired();
  }

  template <typename T>
  static decltype(auto) target(T&& ptr) {
    if constexpr (is_weak_ptr<std::decay_t<T>>::value) {
      assert(!ptr.expired());
      return ptr.lock();
    } else {
      return std::forward<T>(ptr);
    }
  }

  template<std::size_t I = 0, typename... Tp>
  inline typename std::enable_if<I == sizeof...(Tp), bool>::type
  eqKey(std::tuple<Tp...>&, std::tuple<Tp...>&) { return true; }

  template<std::size_t I = 0, typename... Tp>
  inline typename std::enable_if<I < sizeof...(Tp), bool>::type
  eqKey(std::tuple<Tp...>& s, std::tuple<Tp...>& t)
  {
    return eqKeyEntry(std::get<I>(s), std::get<I>(t)) && eqKey<I + 1, Tp...>(s, t);
  }

  template <typename T>
  static bool eqKeyEntry(T&& s, T&& t) {
    if constexpr (is_shared_ptr<std::decay_t<T>>::value || is_unique_ptr<std::decay_t<T>>::value) {
      return (s == nullptr && t == nullptr) || *s == *t;
    } else if constexpr (is_weak_ptr<std::decay_t<T>>::value) {
      assert(!s.expired());
      assert(!t.expired());
      return s.lock() == t.lock();
    } else {
      return s == t;
    }
  }

  bool isExpired(const tuple<K...>& k) {
    return isExpired(std::get<K>(k)...);
  }

  template <typename T>
  bool isExpired(const T&) {
    return false;
  }

  template <typename T>
  bool isExpired(const std::weak_ptr<T>& ptr) {
    return ptr.expired();
  }

 public:
  UniqueFactory() {
    static_assert(!is_weak_ptr<V>::value && !is_shared_ptr<V>::value && !is_unique_ptr<V>::value && !std::is_pointer<V>::value, "V must be a non-pointer type");
  }

  UniqueFactory(const UniqueFactory&) = delete;
  UniqueFactory(UniqueFactory&&) = delete;
  UniqueFactory& operator=(const UniqueFactory&) = delete;
  UniqueFactory& operator=(UniqueFactory&&) = delete;

  shared_ptr<const V> get(const K&... k, function<V()> create) {
    return get(k..., [&]() {
      return shared_ptr<V>(create());
    });
  }

  shared_ptr<const V> get(const K&... k, function<V*()> create) {
    return get(k..., [&]() {
      return shared_ptr<V>(create());
    });
  }

  shared_ptr<const V> get(const K&... k, function<std::shared_ptr<V>()> create) {
    std::lock_guard<std::mutex> lock(mutex);

    Key key{k...};

    auto it = cache.begin();
    while (it != cache.end()) {
      if constexpr(weakKeys) {
        if (isExpired(it->first)) {
          auto jt = it;
          it++;
          cache.erase(jt);
          continue;
        }
        if (eqKey(it->first, key)) {
          return it->second;
        }
      } else {
        if (it->second.expired()) {
          auto jt = it;
          it++;
          cache.erase(jt);
          continue;
        }
        if (eqKey(it->first, key)) {
          return it->second.lock();
        }
      }

      ++it;
    }

    shared_ptr<V> ret = create();
    cache.emplace_front(KeyValuePair(key, ret));
    return ret;
  }
};

}

#endif
