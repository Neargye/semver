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

// Ranges
semver::range_set range;
if (semver::parse(">=1.0.0 <2.0.0 || >3.2.1", range)) {
  assert(range.contains(v));
}

// Validate without parsing into a variable
assert(semver::valid("1.0.0"));

// Wider integer types
semver::version<uint64_t> big;
semver::parse("0.0.999999999999", big);
```

More examples in [example/](example/).

## API Reference

### `version<I1, I2, I3>` *(default `I1 = I2 = I3 = uint32_t`)*

```cpp
version();                             // default: 0.1.0 (semver FAQ §4)
version(I1 major, I2 minor, I3 patch); // construct from components

I1          major()          const noexcept; // major number
I2          minor()          const noexcept; // minor number
I3          patch()          const noexcept; // patch number
string_view prerelease_tag() const noexcept; // "alpha.1" or "" if absent
string_view build_metadata() const noexcept; // "build.42" or "" — ignored in ==
string      to_string()      const;          // "1.2.3-pre+build"

version bump_major() const noexcept; // 1.2.3 → 2.0.0, clears pre-release and build
version bump_minor() const noexcept; // 1.2.3 → 1.3.0, clears pre-release and build
version bump_patch() const noexcept; // 1.2.3 → 1.2.4, clears pre-release and build
```

Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=` on C++17; `==` + `<=>` → `std::strong_ordering` on C++20.  
Build metadata is excluded from all comparisons per spec §10.

### `range_set<I1, I2, I3>` *(default `I1 = I2 = I3 = uint32_t`)*

```cpp
bool contains(version v,
              version_compare_option = exclude_prerelease) const noexcept;
```

`exclude_prerelease` (default): a pre-release version only matches if a comparator in the same set explicitly targets the same `[major.minor.patch]` with a pre-release tag.

### Parsing & serialization

```cpp
// Full-string parse — fails on trailing garbage
from_chars_result parse(string_view str, version&   v);
from_chars_result parse(string_view str, range_set& rs);

// Partial parse — stops at first non-version character (std::from_chars convention)
from_chars_result from_chars(const char* first, const char* last, version& v) noexcept;

// Zero-allocation serialize (std::to_chars convention)
from_chars_result to_chars(char* first, char* last, const version& v) noexcept;

// Convenience wrappers
bool              valid(string_view str);       // validate without output
optional<version> try_parse(string_view str);   // nullopt on failure
version           from_string(string_view str); // throws std::system_error on failure
```

### `from_chars_result`

```cpp
struct from_chars_result {
    const char* ptr; // success: one past last consumed/written char
                     // failure: first invalid char
    std::errc   ec;  // errc{} on success; invalid_argument / value_too_large / result_out_of_range
    explicit operator bool() const noexcept; // true on success
};
```

### C++20 additions (feature-guarded)

| Feature | Guard |
|---------|-------|
| `operator<=>` → `std::strong_ordering` | `__cpp_impl_three_way_comparison` |
| `"1.2.3-rc.1"_semver` consteval literal | `SEMVER_HAS_CONSTEVAL_LITERAL == 1` |
| `std::format("{}", v)` | `__cpp_lib_format` |



- Default-constructed `version` is `0.1.0` per [semver FAQ](https://semver.org/#how-should-i-deal-with-revisions-in-the-0yz-initial-development-phase).
- `parse()` returns `from_chars_result{ptr, ec}` — contextually convertible to `bool`. `ptr` points past consumed input on success or at the bad char on failure. Possible error codes: `invalid_argument`, `value_too_large`, `result_out_of_range`.
- On parse failure the output object may be partially updated. Always check the result before using it.
- Range syntax is comparator-based (`>=1.0.0 <2.0.0 || 3.0.0`), not the full npm semver grammar.
- Prerelease versions excluded from range matching by default. Pass `version_compare_option::include_prerelease` to override.
- `std::hash<semver::version<...>>` and `std::formatter` (C++20, under `__cpp_lib_format`) are provided out of the box. The formatter supports `{}` only — format specs such as `{:>20}` throw `std::format_error`.

## Configuration

| Macro | Default | Purpose |
|-------|---------|---------|
| `SEMVER_MAX_INPUT_LENGTH` | `4096` | Max input length; exceeding → `value_too_large` |
| `SEMVER_CONFIG_FILE` | — | Custom config header included early |
| `SEMVER_HAS_CONSTEXPR` | auto | `1` when full constexpr parsing is available, `0` otherwise |
| `SEMVER_CONSTEXPR` | auto | `constexpr` when `SEMVER_HAS_CONSTEXPR=1`, `inline` otherwise |
| `SEMVER_HAS_CONSTEVAL_LITERAL` | auto | `1` when `operator""_semver` is available, `0` otherwise |

## Constexpr support

`SEMVER_HAS_CONSTEXPR` is auto-detected. When `1`, all parsing and accessor functions
become `constexpr` and compile-time evaluation works via the transient-lambda pattern:

```cpp
static_assert([] {
  semver::version<> v;
  (void)semver::parse("1.2.3-alpha+build", v);
  return v.major() == 1 && v.prerelease_tag() == "alpha";
}());
```

| Toolchain | `SEMVER_HAS_CONSTEXPR` |
|-----------|-------------------------|
| GCC + libstdc++ | `1` |
| Clang + libc++ | `1` |
| Clang + libstdc++ < 13 | `0` |

The `"..."_semver` consteval literal is gated by `SEMVER_HAS_CONSTEVAL_LITERAL` (implies
`SEMVER_HAS_CONSTEXPR=1`, and additionally excludes MSVC and Clang+libstdc++ combinations
that do not support it reliably). Always check this macro before using the literal in
portable code.

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
