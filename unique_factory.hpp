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

namespace unique_factory {

template <typename T>
struct is_shared_ptr : std::false_type {
  using weak = T;
};

template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {
  using weak = std::weak_ptr<T>;
};

template <typename T>
constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

template <typename T>
struct is_weak_ptr : std::false_type {
  using shared = T;
};

template <typename T>
struct is_weak_ptr<std::weak_ptr<T>> : std::true_type {
  using shared = std::shared_ptr<T>;
};

template <typename T>
constexpr bool is_weak_ptr_v = is_weak_ptr<T>::value;

template <typename T>
struct is_unique_ptr : std::false_type {};

template <typename T>
struct is_unique_ptr<std::unique_ptr<T>> : std::true_type {};

template <typename T>
constexpr bool is_unique_ptr_v = is_unique_ptr<T>::value;

// A thread-safe factory that creates unique cached objects. This
// is essentially trying to reimplement SageMath's UniqueFactory
// with C++ smart pointers.
template <typename V, typename... K>
class UniqueFactory {
  std::mutex mutex;

  using InternalKey = tuple<K...>;
  using InternalValue = V;
  using ExposedValue = typename is_weak_ptr<V>::shared;

  using Storage = pair<InternalKey, InternalValue>;

  list<Storage> cache;

  static bool isAlive(const Storage& entry) {
    return isAlive(entry.first) && isAlive(entry.second);
  }

  template<std::size_t I = 0, typename... T>
  static std::enable_if_t<I == sizeof...(T), bool> isAlive(const std::tuple<T...>&) {
    return true;
  }

  template<std::size_t I = 0, typename... T>
  static std::enable_if_t<I < sizeof...(T), bool> isAlive(const std::tuple<T...>& key) {
    return isAlive(std::get<I>(key)) && isAlive<I + 1, T...>(key);
  }

  template <typename T>
  static bool isAlive(const T&) {
    return true;
  }

  template <typename T>
  static bool isAlive(const std::weak_ptr<T>& value) {
    return !value.expired();
  }

  template <typename T>
  static constexpr bool false_v = false;

  template<std::size_t I = 0, typename... T>
  std::enable_if_t<I < sizeof...(T), bool> isAlive(const std::tuple<T...>& lhs, const std::tuple<T...>& rhs) {
    return eq(std::get<I>(lhs), std::get<I>(rhs)) && eq<I + 1, T...>(lhs, rhs);
  }

  template<std::size_t I = 0, typename... T>
  std::enable_if_t<I == sizeof...(T), bool> isAlive(const std::tuple<T...>& lhs, const std::tuple<T...>& rhs) {
    return true;
  }

  template <typename T>
  static bool eq(const T& lhs, const T& rhs) {
    return lhs == rhs;
  }

  template <typename T>
  static bool eq(const std::weak_ptr<T>& lhs, const std::weak_ptr<T>& rhs) {
    return !lhs.expired() && !rhs.expired() && lhs.lock() == rhs.lock();
  }

  template <typename T>
  static ExposedValue get(const T& value) {
    return value;
  }

  template <typename T>
  static ExposedValue get(const std::weak_ptr<T>& value) {
    return value.lock();
  }

 public:
  UniqueFactory() = default;
  UniqueFactory(const UniqueFactory&) = delete;
  UniqueFactory(UniqueFactory&&) = delete;
  UniqueFactory& operator=(const UniqueFactory&) = delete;
  UniqueFactory& operator=(UniqueFactory&&) = delete;

  ExposedValue get(const K&... k, function<ExposedValue()> create) {
    std::lock_guard<std::mutex> lock(mutex);

    InternalKey key{k...};

    auto it = cache.begin();
    while (it != cache.end()) {
      if (!isAlive(*it)) {
        auto jt = it;
        it++;
        cache.erase(jt);
        continue;
      }
      if (eq(it->first, key)) {
        return get(it->second);
      }

      ++it;
    }

    auto ret = create();
    cache.emplace_front(Storage(key, ret));
    return ret;
  }

  template <typename T = ExposedValue> 
  T get(const K&... k, function<typename T::element_type*()> create) {
    return get(k..., [&]() {
      return std::shared_ptr<typename T::element_type>(create());
    });
  }

  template <typename T = ExposedValue> 
  T get(const K&... k, function<typename T::element_type()> create) {
    return get(k..., [&]() {
      return std::make_shared<typename T::element_type>(create());
    });
  }
};

}

}

#endif
