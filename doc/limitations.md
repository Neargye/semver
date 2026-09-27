# Limitations

## Component types

- `version<I1, I2, I3>` and `range_set<I1, I2, I3>` require unsigned integer types without `const` or `volatile`; `bool` is not supported.
- The default component type is `std::uint32_t`. Use a wider type when a version or parsed range bound can exceed `UINT32_MAX`.
- Constructors throw `std::out_of_range` for negative values or values that do not fit.
- All range bounds must fit the chosen types, including bounds generated from shorthand. For example, `"255"` fails for `range_set<std::uint8_t>` because it needs the upper bound `<256.0.0-0`. Versions tested against a range may use wider types.
- Bump operations throw `std::overflow_error`; `inc` reports overflow with `std::nullopt`.

## Configuration and input size

`SEMVER_MAX_INPUT_LENGTH` limits parser input to `512` bytes by default, including prefixes and surrounding whitespace. Longer input is rejected before parsing.

Override it directly or through a configuration header:

```cpp
#define SEMVER_CONFIG_FILE "my_semver_config.hpp"
#include <semver.hpp>
```

```cpp
// my_semver_config.hpp
#define SEMVER_MAX_INPUT_LENGTH 1024
```

The configuration must be identical in every translation unit that includes `semver.hpp`.

Prerelease and build metadata count toward the same input limit.

## Range grammar

Supported forms are listed under [Ranges](reference.md#ranges). Range syntax is defined by this library, not by SemVer 2.0.0.

Unsupported forms include:

- empty or ASCII-whitespace-only inputs;
- empty union branches such as `||`, `>=1 ||`, or `|| >=1`;
- `==`, commas, and leading-zero numeric components;
- prefixes such as `v1.2.3` and `~>1.2.3`;
- hyphen ranges such as `1.2 - 2.3`;
- build metadata in range boundaries;
- qualifiers on partial versions such as `1.2-alpha`;
- wildcard operands for comparators, tilde, or caret;
- partial operands for `>`, `<=`, `=`, and `!=`.

`min_version` returns the lowest matching version that fits the range's component types, or `std::nullopt` if none does. `intersects` checks whether a common version exists, even outside those types.

`*`, `x`, and `X` match every release by default and every version with `prerelease_policy::include`. A default-constructed `range_set` contains nothing.

See [Prerelease matching](reference.md#prerelease-matching) for how prereleases affect range bounds.

## System `major` and `minor` macros

`semver.hpp` undefines the system macros `major` and `minor` when present, so they do not conflict with `version::major()` and `version::minor()`.

## `constexpr` support

Compile-time use requires C++20 and depends on the compiler and standard library. Check these flags (`0` or `1`):

- `SEMVER_HAS_CONSTEXPR_CORE`: basic version operations.
- `SEMVER_HAS_CONSTEXPR_OPTIONAL`: `try_parse`, `clean`, `coerce`, and `inc`.
- `SEMVER_HAS_CONSTEXPR_RANGES`: range operations.
- `SEMVER_HAS_CONSTEXPR`: same as `SEMVER_HAS_CONSTEXPR_RANGES`.
- `SEMVER_HAS_CONSTEVAL_LITERAL`: the `"..."_semver` literal.

Runtime use requires only C++17.

## SemVer precedence

Build metadata does not affect SemVer precedence. Comparison operators, ranges, and `std::hash` ignore it.

`compare_with_build` uses build metadata to break ties. Use it for ordering builds, not for SemVer precedence.

## Formatting

`std::format` is available when supported by the standard library. It accepts the same format options as `std::string_view`.

`to_chars` does not append a null terminator. The caller must provide enough space for the complete serialized version.
