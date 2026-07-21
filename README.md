[![Github releases](https://img.shields.io/github/release/Neargye/semver.svg)](https://github.com/Neargye/semver/releases)
[![Vcpkg package](https://img.shields.io/badge/Vcpkg-package-blueviolet)](https://github.com/microsoft/vcpkg/tree/master/ports/neargye-semver)
[![Conan package](https://img.shields.io/badge/Conan-package-blueviolet)](https://conan.io/center/recipes/neargye-semver)
[![License](https://img.shields.io/github/license/Neargye/semver.svg)](LICENSE)

Header-only C++17 library for [Semantic Versioning 2.0.0](https://semver.org). No dependencies.

## Usage

```cpp
#include <cassert>
#include <iostream>
#include <semver.hpp>

semver::version v;
if (semver::parse("1.2.3-alpha+build", v)) {
  v.major();  // 1
  v.prerelease_tag();  // "alpha"
}

semver::version v2{2, 0, 0};
std::cout << v2;  // "2.0.0"
assert(v2 > v);   // 2.0.0 > 1.2.3-alpha

semver::range_set rs;
semver::parse(">=1.0.0 <2.0.0 || >3.2.1", rs);
rs.contains(v);  // false: prereleases are excluded by default
rs.contains(v, semver::include_prerelease);  // true
```

More examples are available in [example/](example/).

## API

### `version<I1, I2, I3>`

All component types default to `std::uint32_t`, so `version<>` and `version<std::uint32_t>` are equivalent. The parser uses 64-bit intermediate values only for checked conversion;
this does not change the storage size of `version<>`. Use another unsigned type explicitly only when the storage or numeric range must differ.

```cpp
version()  // 0.1.0, as recommended by SemVer FAQ section 4
version(I1 major, I2 minor, I3 patch)
version(I1 major, I2 minor, I3 patch, std::string_view prerelease)  // prerelease must be valid

semver::version v{1, 2, 3};  // version<> with uint32_t components
semver::version<std::uint64_t> wide{1, 2, 3};  // explicit 64-bit component storage

major() / minor() / patch()  // noexcept component accessors
prerelease_tag()  // empty if absent
build_metadata()  // ignored by precedence and equality
is_prerelease() / has_build_metadata()
to_string()  // allocates; prefer to_chars() when practical
bump_major() / bump_minor() / bump_patch()  // clear qualifiers and throw on overflow
```

Comparison operators are `==`, `!=`, `<`, `<=`, `>`, and `>=` on C++17. On C++20, `<=>` returns `std::strong_ordering`.
Build metadata is ignored by equality and precedence comparisons.

The prerelease constructor is intended for trusted input and asserts its precondition in debug builds. Use `try_parse()` or `from_string()` for untrusted input.
`bump_major()`, `bump_minor()`, and `bump_patch()` throw `std::overflow_error` instead of wrapping a component.

### Parsing

```cpp
from_chars_result parse(std::string_view, version<>&);  // strict full-string parse
from_chars_result parse(std::string_view, range_set<>&);
from_chars_result from_chars(const char* first, const char* last, version<>&);  // partial parse
std::optional<version<>> try_parse(std::string_view);  // nullopt on failure
version<> from_string(std::string_view);  // throws std::system_error
std::optional<version<>> coerce(std::string_view);  // lenient partial-version conversion
std::optional<version<>> clean(std::string_view);  // removes wrappers, then parses strictly
bool valid(std::string_view);
```

`from_chars_result` contains `ptr` and `ec` and converts explicitly to `bool`. On failure, `ptr` identifies the first bad character.
`ec` is `invalid_argument`, `result_out_of_range`, or `value_too_large`. `to_chars_result` aliases this type.

`coerce` examples: `"v1.2"` -> `1.2.0`, `"1"` -> `1.0.0`, `"1.2.3.4"` -> `1.2.3`, and `"1.2.3garbage"` -> `1.2.3`.
All parsing helpers reject inputs longer than `SEMVER_MAX_INPUT_LENGTH`.

### Serialization

```cpp
to_chars_result to_chars(char* first, char* last, const version&) noexcept;  // zero-allocation
std::string to_string() const;  // allocating member
```

### Ranges

```cpp
range_set rs;
semver::parse(">=1.0.0 <2.0.0 || 3.0.0", rs);
rs.contains(v);  // prereleases are excluded by default
rs.contains(v, semver::include_prerelease);  // opt in explicitly
```

`include_prerelease` and `exclude_prerelease` are aliases for the corresponding `version_compare_option` values.

Range syntax is a strict, node-semver-inspired subset. It supports primitive comparators (`<`, `<=`, `>`, `>=`, `=`), OR sets with `||`, and X-ranges (`*`, `x`, `X`).
Tilde ranges (`~1.2`), caret ranges (`^1.2.3`), and hyphen ranges (`1.2.3 - 2.0.0`) are also supported.

Important differences from node-semver:

- The empty string and leading or trailing whitespace are invalid ranges.
- Primitive comparators require a complete version: use `1`, `1.2`, or `1.x` as X-ranges, but not `>=1`, `>=1.2`, or `>=1.x`.
- Prerelease and build identifiers must be valid SemVer. Build metadata is validated and then ignored for range precedence.

This API does not claim drop-in parser parity with the JavaScript `node-semver` package.

### Utilities

```cpp
int compare(v1, v2);  // -1, 0, or 1
int rcompare(v1, v2);  // reverse comparison
int compare_with_build(v1, v2);  // includes build metadata
version_diff diff(v1, v2);  // first semantic difference
std::optional<version<>> inc(v, kind, pre = {});  // increments a selected component
bool satisfies(v, std::string_view range);  // parses and checks a range
bool satisfies(v, const range_set<>& range);  // checks a pre-parsed range
ForwardIt min_satisfying(first, last, const range_set<>& range);
ForwardIt max_satisfying(first, last, const range_set<>& range);
```

`inc()` increments numeric prerelease identifiers without converting them to `std::uint64_t`, so valid identifiers are limited by `SEMVER_MAX_INPUT_LENGTH` rather than an integer conversion. Unlike direct `bump_*()` calls, `inc()` reports component overflow with `std::nullopt`.

### Feature guards

| Feature | Guard |
|---------|-------|
| `operator<=>` -> `std::strong_ordering` | `__cpp_impl_three_way_comparison` |
| `"1.2.3-rc.1"_semver` consteval literal | `SEMVER_HAS_CONSTEVAL_LITERAL == 1` |
| `std::format("{}", v)` | `defined(__cpp_lib_format) && __cpp_lib_format >= 202110L` |
| `std::hash<version>` | always |

`semver.hpp` includes `<format>` only when `<version>` reports `__cpp_lib_format >= 202110L`; C++17 consumers do not gain an unconditional `<format>` dependency.

## Constexpr

When `SEMVER_HAS_CONSTEXPR == 1`, parsing, serialization, and comparisons are `constexpr`:

```cpp
static_assert([] {
  semver::version<> v;
  (void)semver::parse("1.2.3-alpha+build", v);
  return v.major() == 1 && v.prerelease_tag() == "alpha";
}());
```

`SEMVER_HAS_CONSTEXPR` is enabled when the standard library provides constexpr `std::string` and `std::vector`.
Clang with libstdc++ older than 13 is disabled because of a known constexpr string issue.

The `_semver` literal also requires `__cpp_consteval`. It is currently disabled on MSVC and on GCC with libstdc++.
Check `SEMVER_HAS_CONSTEVAL_LITERAL` before using it in portable code.

## Configuration

| Macro | Default | Purpose |
|-------|---------|---------|
| `SEMVER_MAX_INPUT_LENGTH` | `512` | Inputs longer than this fail with `value_too_large` |
| `SEMVER_CONFIG_FILE` | n/a | Custom config header, included before library defaults and standard headers |

`SEMVER_HAS_CONSTEXPR` and `SEMVER_HAS_CONSTEVAL_LITERAL` are auto-detected user-queryable flags.
`SEMVER_CONSTEXPR` is an internal implementation macro and is undefined at the end of the header. Do not define any of them.

For compatibility with system headers that expose device-number macros, including OpenBSD and glibc headers, `semver.hpp` undefines macros named `major` and `minor`.

`library_version` exposes the current library version as `version<>`.
`library_version_major`, `library_version_minor`, and `library_version_patch` provide the same values as compile-time integers.

## Integration

Drop [`semver.hpp`](include/semver.hpp) into your project, or use a package manager:

- **vcpkg**: `neargye-semver`
- **Conan**: `neargye-semver/x.y.z`
- **CPM**:
  ```cmake
  CPMAddPackage(GITHUB_REPOSITORY Neargye/semver GIT_TAG x.y.z)
  ```
- **CMake package**:
  ```cmake
  find_package(semver CONFIG REQUIRED)
  target_link_libraries(your_target PRIVATE semver::semver)
  ```

Requires C++17. CMake integration requires CMake >= 3.22.

The release CI matrix covers GCC 13-15, Clang 17-20, Visual Studio 2022/2025 (Win32 and x64), and Apple Clang on macOS 14/15 in Debug and Release configurations.
CI also installs the CMake package and builds a standalone consumer against it.

## [MIT License](LICENSE)
