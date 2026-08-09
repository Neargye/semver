# Changelog

## 1.0.1

- Fixed compilation after Windows headers define the `min` and `max` macros ([#63](https://github.com/Neargye/semver/issues/63)).
- CMake installations now include the project license.

## 1.0.0

- SemVer parsing and validation, cleaning and coercion, comparison, serialization, hashing, streams, and optional C++20 formatting.
- Configurable unsigned component types with checked construction and mixed-type comparison.
- Documented version ranges, increments, intersection checks, and minimum-version queries.
- Dependency-free single header with the installable `semver::semver` CMake target.

## Migration from `v0.3.1`

- `version` changed from a fixed `std::uint16_t` structure with public fields and a predefined prerelease enum to `version<I1, I2, I3>` with accessors, arbitrary SemVer prerelease identifiers, and build metadata.
- Replace member parsing and the `_version` literal with the free `parse`, `try_parse`, `try_parse_range`, `from_string`, `from_chars`, and `to_chars` APIs and the `_semver` literal where supported.
- Replace `comparators_option`, `satisfies_option`, and `semver::range::satisfies()` with normal comparison operators or `compare()`, parsed `range_set::contains()`, text-based `satisfies()`, and `prerelease_policy`.

## Migration from `v1.0.0-rc`

- Component types must be unsigned; the defaults changed from `int` to `std::uint32_t`. Construction is checked, and versions with different component types compare and match ranges without narrowing.
- Parsing now includes `try_parse`, `try_parse_range`, `from_string`, `from_chars`, `to_chars`, `clean`, and `coerce`. Parse result types convert to `bool` explicitly, and qualifier accessors return non-owning `std::string_view` values.
- `version_compare_option` was replaced by `prerelease_policy::{exclude, include}` for range matching only. Normal comparison follows SemVer prerelease precedence and ignores build metadata; `compare_with_build()` adds an explicit deterministic metadata tie-break.
- Range syntax now supports `*`/`x`/`X`, partial versions, `!=`, `~`, `^`, whitespace intersections, and `||`. Hyphen ranges are not supported.
- Added `diff()` and `inc()` with the shared `version_change` enum, `min_satisfying()`, `max_satisfying()`, `min_version()`, `intersects()`, hashing, streams, and optional C++20 formatting.
- CMake package metadata now reports `1.0.0`, requests C++17, uses same-major compatibility, and is architecture-independent.

Requires C++17. CMake integration requires CMake 3.15+.
