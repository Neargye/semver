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

Branch | Linux/OSX | Windows | License | Codacy
-------|-----------|---------|---------|-------
master |[![Build Status](https://travis-ci.org/Neargye/semver.svg?branch=master)](https://travis-ci.org/Neargye/semver)|[![Build status](https://ci.appveyor.com/api/projects/status/5k62fhf7u1v5h1st/branch/master?svg=true)](https://ci.appveyor.com/project/Neargye/semver/branch/master)|[![License](https://img.shields.io/github/license/Neargye/semver.svg)](LICENSE)|[![Codacy Badge](https://api.codacy.com/project/badge/Grade/04b3ef8b2be24f72b670af76855307cc)](https://www.codacy.com/app/Neargye/semver?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=Neargye/semver&amp;utm_campaign=Badge_Grade)

C++ library compare and manipulate versions are available as extensions to the `<major>.<minor>.<patch>-<prerelease>.<prereleaseversion>` format complying with [Semantic Versioning 2.0.0](semver.org)

## Features

* C++11
* Header-only
* Dependency-free
* Constexpr comparison: <, <=, ==, !=, > >=
* From/To string

## [Examples](example/example.cpp)

* Create

```cpp
constexpr Version v;
constexpr Version v1 = Version(1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4);
constexpr Version v2 = v1;
```

* Сomparison

```cpp
constexpr semver::Version v1(1, 4, 3);
constexpr semver::Version v2(1, 2, 4, semver::Version::PreReleaseType::kAlpha, 10);
static_assert(v1 != v2, "");
static_assert(!(v1 == v2), "");
static_assert(v1 > v2, "");
static_assert(v1 >= v2, "");
static_assert(v2 < v1, "");
static_assert(v2 <= v1, "");
```

* To string

```cpp
constexpr semver::Version v(1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4);
const std::string s = v.ToString(); // "1.2.3-rc.4"
```

* From string

```cpp
const std::string s("1.2.3-rc.4");
Version v1(s);
Version v2;
v2.FromString(s);
Version v3 = "1.2.3-rc.4"_version;
```

* To char array

```cpp
constexpr semver::Version v(1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4);
char s1[kVersionStringLength];
v.ToString(s1); // "1.2.3-rc.4"
```

* From char array

```cpp
const char s[] = "1.2.3-rc.4";
Version v1(s);
Version v2;
v2.FromString(s);
```

## Integration

You should add required file [semver.hpp](include/semver.hpp) and switch to C++11.

## Compiler compatibility

* GCC
* Clang
* MSVC

## Licensed under the [MIT License](LICENSE)
