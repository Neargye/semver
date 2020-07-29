```text
         _____                            _   _
        / ____|                          | | (_)
       | (___   ___ _ __ ___   __ _ _ __ | |_ _  ___
        \___ \ / _ \ '_ ` _ \ / _` | '_ \| __| |/ __|
        ____) |  __/ | | | | | (_| | | | | |_| | (__
       |_____/ \___|_| |_| |_|\__,_|_| |_|\__|_|\___|
__      __           _             _                _____
\ \    / /          (_)           (_)              / ____|_     _
 \ \  / /__ _ __ ___ _  ___  _ __  _ _ __   __ _  | |   _| |_ _| |_
  \ \/ / _ \ '__/ __| |/ _ \| '_ \| | '_ \ / _` | | |  |_   _|_   _|
   \  /  __/ |  \__ \ | (_) | | | | | | | | (_| | | |____|_|   |_|
    \/ \___|_|  |___/_|\___/|_| |_|_|_| |_|\__, |  \_____|
                                            __/ |
                                           |___/
```

[![Github releases](https://img.shields.io/github/release/Neargye/semver.svg)](https://github.com/Neargye/semver/releases)
[![Vcpkg package](https://img.shields.io/badge/Vcpkg-package-blueviolet)](https://github.com/microsoft/vcpkg/tree/master/ports/neargye-semver)
[![License](https://img.shields.io/github/license/Neargye/semver.svg)](LICENSE)
[![Build Status](https://travis-ci.org/Neargye/semver.svg?branch=master)](https://travis-ci.org/Neargye/semver)
[![Build status](https://ci.appveyor.com/api/projects/status/5k62fhf7u1v5h1st/branch/master?svg=true)](https://ci.appveyor.com/project/Neargye/semver/branch/master)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/04b3ef8b2be24f72b670af76855307cc)](https://www.codacy.com/app/Neargye/semver?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=Neargye/semver&amp;utm_campaign=Badge_Grade)

C++ library compare and manipulate versions are available as extensions to the `<major>.<minor>.<patch>-<prerelease_type>.<prerelease_number>` format complying with [Semantic Versioning 2.0.0](semver.org)

## Features

* C++17
* Header-only
* Dependency-free
* Constexpr comparison: <, <=, ==, !=, > >=
* Constexpr from string
* Constexpr to string

## [Examples](example/example.cpp)

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
  semver::version v6;
  v7.from_string(s); // constexpr and may throw.
  bool success = v8.from_string_noexcept(s); // constexpr and no throw.
  ```

## Integration

You should add required file [semver.hpp](include/semver.hpp).

If you are using [vcpkg](https://github.com/Microsoft/vcpkg/) on your project for external dependencies, then you can use the [neargye-semver package](https://github.com/microsoft/vcpkg/tree/master/ports/neargye-semver).

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
