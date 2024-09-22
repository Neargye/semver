[![Github releases](https://img.shields.io/github/release/Neargye/semver.svg)](https://github.com/Neargye/semver/releases)
[![Vcpkg package](https://img.shields.io/badge/Vcpkg-package-blueviolet)](https://github.com/microsoft/vcpkg/tree/master/ports/neargye-semver)
[![Conan package](https://img.shields.io/badge/Conan-package-blueviolet)](https://conan.io/center/recipes/neargye-semver)
[![License](https://img.shields.io/github/license/Neargye/semver.svg)](LICENSE)

C++ library compare and manipulate versions are available as extensions to the `<major>.<minor>.<patch>-<prerelease_type>.<prerelease_number>` format complying with [Semantic Versioning 2.0.0](https://semver.org)

## [Features & Examples](example/)

* Create

  ```cpp
  constexpr version v1 = version{1, 2, 3, prerelease::rc, 4};
  constexpr version v2 = v1;
  ```

* Сomparison

  ```cpp
  constexpr semver::version v1{1, 4, 3};
  constexpr semver::version v2{1, 2, 4, semver::prerelease::alpha, 10};
  static_assert(v1 != v2);
  static_assert(v1 > v2);
  static_assert(v1 >= v2);
  static_assert(v2 < v1);
  static_assert(v2 <= v1);
  ```

* To string

  ```cpp
  semver::version v{1, 2, 3, prerelease::rc, 4};

  // To string.
  std::string s1 = v.to_string(); // may throw.

  // Non-member to string.
  std::string s2 = semver::to_string(v); // may throw.

  std::array<char, 32> str = {};

  // constexpr to chars, like <https://en.cppreference.com/w/cpp/utility/to_chars>.
  auto [p, ec] = v.to_chars(str.data(), str.data() + str.size()); // constexpr and no throw.

  // Non-member constexpr to chars, like <https://en.cppreference.com/w/cpp/utility/to_chars>.
  auto [p, ec] = semver::to_chars(str.data(), str.data() + str.size(), v); // constexpr and no throw.
  ```

* From string

  ```cpp
  std::string_view s = "1.2.3-rc.4";

  // From chars.
  semver::version v1{s}; // constexpr and may throw.

  // User-defined literals '_version'.
  semver::version v2 = "1.2.3-rc.4"_version; // constexpr and may throw.

  // constexpr from_chars, like <https://en.cppreference.com/w/cpp/utility/from_chars>.
  semver::version v3;
  auto [p, ec] = v3.to_chars(str.data(), str.data() + str.size()); // constexpr and no throw.

  // Non-member constexpr from chars, like <https://en.cppreference.com/w/cpp/utility/from_chars>.
  semver::version v4;
  auto [p, ec] = semver::to_chars(str.data(), str.data() + str.size(), v4); // constexpr and no throw.

  // Non-member from string.
  semver::version v5 = semver::from_string(s); // constexpr and may throw.
  std::optional<version> v6 = semver::from_string_noexcept(s); // constexpr and no throw.

  // From string.
  semver::version v7;
  v7.from_string(s); // constexpr and may throw.
  bool success = v7.from_string_noexcept(s); // constexpr and no throw.
  ```
  
* Range matching

  ```cpp
  constexpr auto range = semver::range(">=1.0.0 <2.0.0 || >3.2.1");
  constexpr auto version = semver::version("1.2.3");
  if (range.satisfies(version)) {
    // Do something.
  }
  ```
  
* Range matching with prerelease tag

  ```cpp
  constexpr auto range = semver::range(">1.2.3-beta.1");
  constexpr auto version = semver::version("3.4.5-alpha.0");

  // By default, version is allowed to satisfy range if at least one comparator with the same [major, minor, patch] has a prerelease tag.
  static_assert(!range.satisfies(version));
  // Suppress this behavior and treat prerelease versions as normal.
  static_assert(range.satisfies(version, semver::range::option::include_prerelease));
  ```
  
## Integration

You should add required file [semver.hpp](include/semver.hpp).

If you are using [vcpkg](https://github.com/Microsoft/vcpkg/) on your project for external dependencies, then you can use the [neargye-semver](https://github.com/microsoft/vcpkg/tree/master/ports/neargye-semver).

If you are using [Conan](https://www.conan.io/) to manage your dependencies, merely add `neargye-semver/x.y.z` to your conan's requires, where `x.y.z` is the release version you want to use.


Alternatively, you can use something like [CPM](https://github.com/TheLartians/CPM) which is based on CMake's `Fetch_Content` module.

```cmake
CPMAddPackage(
    NAME semver
    GITHUB_REPOSITORY Neargye/semver
    GIT_TAG x.y.z # Where `x.y.z` is the release version you want to use.
)
```

## Compiler compatibility

* Clang/LLVM >= 5
* MSVC++ >= 14.11 / Visual Studio >= 2017
* Xcode >= 10
* GCC >= 7

## Licensed under the [MIT License](LICENSE)
