[![Github releases](https://img.shields.io/github/release/Neargye/semver.svg)](https://github.com/Neargye/semver/releases)
[![Vcpkg package](https://img.shields.io/badge/Vcpkg-package-blueviolet)](https://github.com/microsoft/vcpkg/tree/master/ports/neargye-semver)
[![Conan package](https://img.shields.io/badge/Conan-package-blueviolet)](https://conan.io/center/recipes/neargye-semver)
[![License](https://img.shields.io/github/license/Neargye/semver.svg)](LICENSE)

Header-only C++17 library for parsing, comparing and matching [Semantic Versioning 2.0.0](https://semver.org) strings. No dependencies.

## Usage

```cpp
#include <semver.hpp>

// Parse
semver::version v;
if (semver::parse("1.2.3-alpha+build", v)) {
  v.major();          // 1
  v.minor();          // 2
  v.patch();          // 3
  v.prerelease_tag(); // "alpha"
  v.build_metadata(); // "build"
}

// from_chars-style result
const auto [ptr, ec] = semver::parse("1.2.3", v);

// Construct directly
semver::version v2{1, 0, 0};
std::cout << v2 << '\n';       // "1.0.0"
std::string s = v2.to_string();

// Compare
assert(v2 > v);   // 1.0.0 > 1.2.3-alpha (prerelease < release)

// Ranges (npm-style)
semver::range_set range;
if (semver::parse(">=1.0.0 <2.0.0 || >3.2.1", range)) {
  assert(range.contains(v));
}

// Validate without parsing into a variable
assert(semver::valid("1.0.0"));

// Wider integer types
semver::version<int64_t> big;
semver::parse("0.0.999999999999", big);
```

More examples in [example/](example/).

## Notes

- Default-constructed `version` is `0.1.0` per [semver FAQ](https://semver.org/#how-should-i-deal-with-revisions-in-the-0yz-initial-development-phase).
- `parse()` returns `from_chars_result{ptr, ec}` — contextually convertible to `bool`. `ptr` points past consumed input on success or at the bad char on failure. Possible error codes: `invalid_argument`, `value_too_large`, `result_out_of_range`.
- On parse failure the output object is left in an unspecified state (like `std::from_chars`). Always check the result before using the output.
- Prerelease versions excluded from range matching by default. Pass `version_compare_option::include_prerelease` to override.
- `std::hash<semver::version<...>>` and `std::formatter` (C++20) are provided out of the box.

## Configuration

| Macro | Default | Purpose |
|-------|---------|---------|
| `SEMVER_MAX_INPUT_LENGTH` | `4096` | Max input length; exceeding → `value_too_large` |
| `SEMVER_CONFIG_FILE` | — | Custom config header included early |
| `SEMVER_FULL_CONSTEXPR` | auto | `1` when full constexpr parsing is available, `0` otherwise |
| `SEMVER_CONSTEXPR` | auto | `constexpr` when `SEMVER_FULL_CONSTEXPR=1`, `inline` otherwise |

## Constexpr support

`SEMVER_FULL_CONSTEXPR` is auto-detected at compile time. It is set to `1` when both
`__cpp_lib_constexpr_string >= 201907L` and `__cpp_lib_constexpr_vector >= 201907L` are
defined, except for known broken combinations:

| Toolchain | `SEMVER_FULL_CONSTEXPR` | Notes |
|-----------|-------------------------|-------|
| GCC + libstdc++ | `1` | All recent versions |
| Clang + libc++ ≥ 18 | `1` | Full support including `_semver` literal |
| Clang + libc++ < 18 | `0` | Incomplete constexpr string |
| Clang + libstdc++ < 14 | `0` | `construct_at` SSO union bug |
| MSVC 19.29+ | `1` | With `/std:c++20` or later |

When `SEMVER_FULL_CONSTEXPR = 1`, parsing and comparisons work at compile time.
All constexpr tests use the **transient-lambda pattern** — `version<>` objects live
only inside the lambda body, so heap allocations (long prerelease/build strings) and
SSO self-referential pointers are fully contained within the evaluation:

```cpp
static_assert([] {
  semver::version<> v;
  (void)semver::parse("1.2.3-alpha+build", v);
  return v.major() == 1 && v.prerelease_tag() == "alpha";
}());
```

The `"..."_semver` consteval literal (C++20, requires `__cpp_consteval`) is available
when `SEMVER_FULL_CONSTEXPR = 1` and is tested on Clang + libc++ ≥ 18. On GCC +
libstdc++ the literal compiles but must be called from inside a `consteval` function —
GCC treats a `consteval` call from a `constexpr` lambda as an "immediate invocation"
whose result (`version<>` with a self-referential SSO pointer) must itself be a constant
expression, which libstdc++'s `std::string` cannot satisfy.

## Integration

Copy [semver.hpp](include/semver.hpp) into your project, or use a package manager:

- **vcpkg**: `neargye-semver`
- **Conan**: `neargye-semver/x.y.z`
- **CPM**:
  ```cmake
  CPMAddPackage(GITHUB_REPOSITORY Neargye/semver GIT_TAG x.y.z)
  ```

## Compiler support

C++17 required. C++20 adds `operator<=>`, concepts, and, when the toolchain provides working `constexpr std::string`/`std::vector`, full compile-time parsing and `consteval` literals.

* GCC >= 7
* Clang >= 5
* MSVC >= 14.20 (VS 2019)
* Xcode >= 10

## [MIT License](LICENSE)
