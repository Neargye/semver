# Limitations

## Component types

- `version<I1, I2, I3>` and `range_set<I1, I2, I3>` require unqualified unsigned integral component types other than `bool`.
- The default component type is `std::uint32_t`. Use a wider type when a version or parsed range bound can exceed `UINT32_MAX`.
- Constructors check source values before conversion. Negative and unrepresentable values throw `std::out_of_range`.
- Every explicit or generated range bound must fit the corresponding component type of the `range_set`. For example, `"255"` cannot be stored in `range_set<std::uint8_t>` because its expansion requires the generated upper bound `<256.0.0-0`. A mixed-type candidate may be wider and is compared without narrowing, but this does not widen stored range bounds.
- Bump operations throw `std::overflow_error`; `inc` reports overflow with `std::nullopt`.

## Configuration and input size

`SEMVER_MAX_INPUT_LENGTH` limits raw parser input, including wrappers and outer whitespace. The default is `512` bytes. Inputs above the limit fail before trimming or tokenization.

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

There is no separate identifier-size macro. Prerelease and build metadata share the total input limit and are stored in `std::string`.

## Range grammar

The library accepts only the forms documented in the [Ranges](reference.md#ranges) section. Range syntax is a library contract, not part of the SemVer 2.0.0 specification.

Unsupported forms include:

- loose or coercive range parsing;
- empty or ASCII-whitespace-only inputs;
- empty union branches such as `||`, `>=1 ||`, or `|| >=1`;
- `==`, commas, and leading-zero numeric components;
- legacy range prefixes and aliases such as `v1.2.3` and `~>1.2.3`;
- hyphen ranges such as `1.2 - 2.3`;
- build metadata in range boundaries;
- qualifiers on partial versions such as `1.2-alpha`;
- wildcard operands for comparators, tilde, or caret;
- partial operands for `>`, `<=`, `=`, and `!=`.

The API focuses on parsing, matching, minimum-version lookup, and intersection checks. It does not provide subset or outside queries, or range simplification.

`min_version` returns only a version representable by the component types of its `range_set`. It returns `std::nullopt` when the set is empty or its mathematical minimum lies beyond those types. `intersects` can still report that two such unbounded sets intersect because it does not need to materialize the matching version.

`*`, `x`, and `X` are the explicit any range: they match every release by default and every version with `prerelease_policy::include`. A default-constructed, unparsed `range_set` contains nothing.

For the implemented range grammar, `prerelease_policy::include` uses the generated prerelease boundaries documented in the reference. Other range syntaxes are not accepted implicitly.

## System `major` and `minor` macros

Some system headers define function-like macros named `major` and `minor`, which conflict with `version::major()` and `version::minor()`. To keep the public accessors usable, `semver.hpp` intentionally undefines both macros when present.

This changes preprocessor state for the including translation unit. Include or restore the platform-specific device-number macros after `semver.hpp` only when they are required and account for the resulting name conflict at semver accessor call sites. See [Neargye/semver#57](https://github.com/Neargye/semver/pull/57) for the OpenBSD failure that motivated this behavior.

## `constexpr` support

The runtime API requires only C++17. Compile-time parsing and the literal depend on the compiler and standard library's C++20 constexpr container support; this ordinary toolchain variation does not affect the runtime API.

- `SEMVER_HAS_CONSTEXPR` reports whether the full parsing and range path is constexpr-enabled.
- `SEMVER_HAS_CONSTEVAL_LITERAL` reports whether `"..."_semver` is available.

Use the feature macros rather than compiler-version assumptions.

## SemVer precedence

Build metadata does not participate in SemVer precedence. Consequently, normal comparison operators, equality, ranges, and `std::hash` ignore build metadata.

`compare_with_build` provides deterministic artifact ordering by comparing complete build metadata lexicographically after normal precedence. It may distinguish SemVer-equivalent versions and must not be used as SemVer precedence.

## Formatting

`std::format` integration is available only when the standard library reports C++20 format support. The formatter accepts `{}` and rejects additional format specifications.

`to_chars` does not append a null terminator. The caller must provide enough space for the complete serialized version.
