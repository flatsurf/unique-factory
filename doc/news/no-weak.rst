**Added:**

* Added `get` overloads thats consume the `Key&&`.

**Changed:**

* Changed the `Value` that can be created by a factory. It now must use
  `std::enable_shared_from_this` (or equivalently implement
  `shared_from_this()`).

* Changed how values are stored in the factory cache. They are now stored as
  `const Value*` and not as `std::weak_ptr<Value>` anymore.

* Changed required C++ version. Now we need at least C++17.

**Performance:**

* Improved `get` which now uses only a single lookup and create call.
