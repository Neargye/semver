[![Github releases](https://img.shields.io/github/release/Neargye/semver.svg)](https://github.com/Neargye/semver/releases)
[![Vcpkg package](https://img.shields.io/badge/Vcpkg-package-blueviolet)](https://github.com/microsoft/vcpkg/tree/master/ports/neargye-semver)
[![Conan package](https://img.shields.io/badge/Conan-package-blueviolet)](https://conan.io/center/recipes/neargye-semver)
[![License](https://img.shields.io/github/license/Neargye/semver.svg)](LICENSE)

C++ library to compare and manipulate versions in `<major>.<minor>.<patch>[-<prerelease>][+<build>]` format, complying with [Semantic Versioning 2.0.0](https://semver.org). Header-only, C++17, no dependencies.

## [Features & Examples](example/)

* Parse

  ```cpp
  semver::version v1;
  if (semver::parse("1.4.3", v1)) {
    const int patch = v1.patch(); // 3
  }

  semver::version v2;
  if (semver::parse("1.2.4-alpha.10+build.1", v2)) {
    v2.prerelease_tag(); // "alpha.10"
    v2.build_metadata(); // "build.1"
  }

  // Detailed result: ptr points past last consumed char, ec holds the error code
  const auto [ptr, ec] = semver::parse("1.2.3", v1);
  if (ec == std::errc{}) { /* success */ }
  ```

* Construct & serialize

  ```cpp
  semver::version v{1, 2, 3};       // version(major, minor, patch); asserts non-negative
  std::string s = v.to_string();    // "1.2.3"
  std::cout << v << '\n';           // operator<< supported
  ```

* Comparison

  ```cpp
  assert(v1 != v2);
  assert(v1 > v2);
  assert(v1 >= v2);
  assert(v2 < v1);
  assert(v2 <= v1);
  // C++20: three-way comparison
  assert((v1 <=> v2) > 0);
  ```

* Validate

  ```cpp
  const bool result = semver::valid("1.2.3-alpha+build");
  assert(result);
  ```

* Custom integer types

  ```cpp
  // Use a wider type when version numbers exceed int range
  semver::version<int64_t> big;
  semver::parse("0.0.999999999999", big); // patch == 999999999999

  // Independent types per component; mixed-type comparison works
  semver::version<uint32_t> a; semver::parse("2.0.0", a);
  semver::version<int>      b; semver::parse("2.0.0", b);
  assert(a == b);
  ```

* Range matching

  ```cpp
  semver::range_set range;
  if (semver::parse(">=1.0.0 <2.0.0 || >3.2.1", range)) {
    semver::version version;
    if (semver::parse("1.2.3", version)) {
      assert(range.contains(version));
    }

    // Prerelease versions are excluded by default (npm semver spec).
    // Pass include_prerelease to override:
    semver::version pre; semver::parse("1.2.3-rc.1", pre);
    range.contains(pre, semver::version_compare_option::include_prerelease);
  }
  ```

Check the *examples* folder to see more various usage examples

## Default-constructed version

A default-constructed `semver::version{}` represents `0.1.0`, not `0.0.0`.
This is intentional and follows the [Semantic Versioning 2.0.0 FAQ](https://semver.org/#how-should-i-deal-with-revisions-in-the-0yz-initial-development-phase):

> The simplest thing to do is start your initial development release at **0.1.0** and then increment the minor version for each subsequent release.

## Error handling

`semver::parse()` returns a `from_chars_result` with two fields:
- `ptr` — points past the last consumed character on success, or at the offending character on failure.
- `ec` — `std::errc{}` on success; `std::errc::invalid_argument` for syntax errors; `std::errc::value_too_large` if input exceeds `SEMVER_MAX_INPUT_LENGTH`; `std::errc::result_out_of_range` if a numeric component overflows the target integer type.

`from_chars_result` is contextually convertible to `bool` (`true` = success).

> **Note:** On parse failure the output `version`/`range_set` object is left in an indeterminate (partially-written) state. Always check the return value before using the output.

## Configuration

| Macro | Default | Description |
|-------|---------|-------------|
| `SEMVER_MAX_INPUT_LENGTH` | `4096` | Maximum allowed input string length for `parse()`. Strings exceeding this limit are rejected immediately with `std::errc::value_too_large`. Define before including the header to override. |
| `SEMVER_CONFIG_FILE` | *(undefined)* | Path to a user-provided configuration header included early in `semver.hpp`. |

```cpp
// Override before including the header:
#define SEMVER_MAX_INPUT_LENGTH 256
#include <semver.hpp>
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

Requires **C++17**. C++20 enables additional features:
- Full `constexpr` support (`std::string`/`std::vector` become `constexpr`)
- `operator<=>` (three-way comparison)
- `consteval operator""_semver` literal (in `semver::literals` namespace)
- Concepts-constrained template parameters

* Clang/LLVM >= 5
* MSVC++ >= 14.11 / Visual Studio >= 2017
* Xcode >= 10
* GCC >= 7

## Licensed under the [MIT License](LICENSE)
