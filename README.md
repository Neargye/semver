[![Github releases](https://img.shields.io/github/release/Neargye/semver.svg)](https://github.com/Neargye/semver/releases)
[![Vcpkg package](https://img.shields.io/badge/Vcpkg-package-blueviolet)](https://github.com/microsoft/vcpkg/tree/master/ports/neargye-semver)
[![Conan package](https://img.shields.io/badge/Conan-package-blueviolet)](https://conan.io/center/recipes/neargye-semver)
[![License](https://img.shields.io/github/license/Neargye/semver.svg)](LICENSE)

Header-only C++17 library for [Semantic Versioning 2.0.0](https://semver.org). No dependencies.

## Usage

```cpp
#include <semver.hpp>

semver::version v;
if (semver::parse("1.2.3-alpha+build", v)) {
  v.major();           // 1
  v.prerelease_tag();  // "alpha"
}

semver::version v2{1, 0, 0};
std::cout << v2;  // "1.0.0"
assert(v2 > v);   // 1.0.0 > 1.2.3-alpha

semver::range_set rs;
semver::parse(">=1.0.0 <2.0.0 || >3.2.1", rs);
rs.contains(v);   // true
```

More examples in [example/](example/).

## API

### `version<I1, I2, I3>`

All three type params default to `uint32_t`. Any unsigned integer works -- use `uint8_t` for compact storage or `uint64_t` for large version numbers.

```cpp
version()                                // default-constructs to 0.1.0 (semver FAQ §4)
version(I1 major, I2 minor, I3 patch)

major() / minor() / patch()              // component accessors, all noexcept
prerelease_tag()                         // string_view, empty if absent
build_metadata()                         // string_view -- excluded from == and < per spec §10
to_string()                              // allocates; prefer to_chars() when you can

bump_major() / bump_minor() / bump_patch()  // returns new version, clears pre-release and build
```

Comparison operators: `==`, `!=`, `<`, `<=`, `>`, `>=` on C++17; `<=>` returning `std::strong_ordering` on C++20. Build metadata is always ignored.

### Parsing

```cpp
from_chars_result parse(string_view, version&);      // full parse -- fails on trailing chars
from_chars_result parse(string_view, range_set&);
from_chars_result from_chars(first, last, version&); // partial -- stops at first unknown char
optional<version> try_parse(string_view);            // nullopt on failure
version           from_string(string_view);          // throws std::system_error on failure
optional<version> coerce(string_view);               // lenient: strips "v" prefix, fills missing components with 0
bool              valid(string_view);
```

`from_chars_result` holds `ptr` and `ec` and converts to `bool`. On failure, `ptr` points at the first bad character; `ec` is one of `invalid_argument`, `result_out_of_range`, or `value_too_large`. `to_chars_result` is an alias for the same struct.

`coerce` examples: `"v1.2"` -> `1.2.0`, `"1"` -> `1.0.0`, `"1.2.3.4"` -> `1.2.3`, `"1.2.3garbage"` -> `1.2.3`.

### Serialization

```cpp
to_chars_result to_chars(char* first, char* last, const version&) noexcept;  // zero-alloc, like std::to_chars
string          to_string() const;                                            // member, allocates
```

### Ranges

```cpp
range_set rs;
semver::parse(">=1.0.0 <2.0.0 || 3.0.0", rs);
rs.contains(v);                               // pre-releases excluded by default
rs.contains(v, semver::include_prerelease);   // opt in explicitly
rs.contains(v, semver::include_build_metadata); // non-standard, spec §10
```

`include_prerelease`, `exclude_prerelease`, and `include_build_metadata` are predefined constants in `namespace semver` (aliases for the corresponding `version_compare_option` values).

Range syntax is comparator-based (`<`, `<=`, `>`, `>=`, `=`) with `||`. No tilde/caret/wildcard.

### Utilities

```cpp
version_diff diff(v1, v2);              // major/minor/patch/premajor/preminor/prepatch/prerelease/none
optional<version> inc(v, kind, pre={}); // inverse of diff(); bumps v by kind, sets pre-release to pre
int compare_build(v1, v2);              // -1/0/1, includes build metadata (non-standard extension)
bool satisfies(v, range_str);           // shorthand for parse(range_str) + contains(v)
```

### C++20

| Feature | Guard |
|---------|-------|
| `operator<=>` -> `std::strong_ordering` | `__cpp_impl_three_way_comparison` |
| `"1.2.3-rc.1"_semver` consteval literal | `SEMVER_HAS_CONSTEVAL_LITERAL == 1` |
| `std::format("{}", v)` | `__cpp_lib_format` |
| `std::hash<version>` | always |

## Constexpr

When `SEMVER_HAS_CONSTEXPR == 1`, all parsing and comparisons are `constexpr`:

```cpp
static_assert([] {
  semver::version<> v;
  (void)semver::parse("1.2.3-alpha+build", v);
  return v.major() == 1 && v.prerelease_tag() == "alpha";
}());
```

| Toolchain | `SEMVER_HAS_CONSTEXPR` |
|-----------|------------------------|
| GCC + libstdc++ | 1 |
| Clang + libc++ | 1 |
| Clang + libstdc++ < 13 | 0 |
| MSVC | 0 |

The `_semver` literal requires `__cpp_consteval` and additionally excludes MSVC and older GCC/libstdc++ combinations. Check `SEMVER_HAS_CONSTEVAL_LITERAL` before using it in portable code.

## Configuration

| Macro | Default | Purpose |
|-------|---------|---------|
| `SEMVER_MAX_INPUT_LENGTH` | `4096` | Inputs longer than this fail with `value_too_large` |
| `SEMVER_CONFIG_FILE` | -- | Custom config header, included before anything else |

`SEMVER_HAS_CONSTEXPR`, `SEMVER_CONSTEXPR`, and `SEMVER_HAS_CONSTEVAL_LITERAL` are auto-detected -- don't define them yourself.

## Integration

Drop [`semver.hpp`](include/semver.hpp) into your project, or use a package manager:

- **vcpkg**: `neargye-semver`
- **Conan**: `neargye-semver/x.y.z`
- **CPM**:
  ```cmake
  CPMAddPackage(GITHUB_REPOSITORY Neargye/semver GIT_TAG x.y.z)
  ```

Requires C++17. Tested on GCC >= 7, Clang >= 5, MSVC >= 14.20 (VS 2019).

## [MIT License](LICENSE)