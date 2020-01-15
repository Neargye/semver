# Semantic Versioning C++

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
* Constexpr to String

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
constexpr semver::version v{1, 2, 3, prerelease::rc, 4};

// To string.
auto s = v.to_string(); // "1.2.3-rc.4"

// Constexpr to chars.
std::array<char, 32> str;
v.to_chars(str.data(), str.data() + str.size());
```

* From string

```cpp
// Constexpr from chars.
constexpr semver::version v1("1.2.3-rc.4");
constexpr semver::version v2 = "1.2.3-rc.4"_version;

// From string.
std::string s = "1.2.3-rc.4";
version v3 = semver::from_string(s);
```

## Synopsis

```
TODO
```

## Integration

You should add required file [semver.hpp](include/semver.hpp).

## Compiler compatibility

* Clang/LLVM >= 5
* MSVC++ >= 14.11 / Visual Studio >= 2017
* Xcode >= 10
* GCC >= 7

## Licensed under the [MIT License](LICENSE)
