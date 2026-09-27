# Reference

Include the header to use the library:

```cpp
#include <semver.hpp>
```

The API requires C++17. See [limitations](limitations.md) for input limits and [compile-time support](limitations.md#constexpr-support).

## Synopsis

- [`version`](#version) represents and transforms a semantic version.
- [Parsing](#parsing) validates, cleans, or coerces version strings.
- [Comparison](#comparison) implements SemVer precedence and reports version changes.
- [Serialization](#serialization) writes versions to strings, buffers, streams, and formatters.
- [Ranges](#ranges) parses and evaluates version constraints.
- [Incrementing](#incrementing) applies release and prerelease increments.
- [Constants and feature flags](#constants-and-feature-flags) report library configuration.

## `version`

```cpp
template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
class version;
```

Component types must be unsigned integers without `const` or `volatile`; `bool` is not supported. They may differ. `version<>` uses `std::uint32_t` for all three.

### Construction

```cpp
version();

template <typename T1, typename T2, typename T3>
version(T1 major, T2 minor, T3 patch);

template <typename T1, typename T2, typename T3>
version(T1 major, T2 minor, T3 patch, std::string_view prerelease, std::string_view build = {});
```

The default value is `0.1.0`. Integral constructor arguments are checked before conversion. Negative or unrepresentable components throw `std::out_of_range`; invalid prerelease or build identifiers throw `std::invalid_argument`.

Type deduction selects an unsigned type of at least 32 bits for each component, preserving the width of wider arguments:

```cpp
semver::version v{1, 2, 3}; // semver::version<std::uint32_t>
semver::version wide{std::uint64_t{5'000'000'000}, 2, 3};
// semver::version<std::uint64_t, std::uint32_t, std::uint32_t>
```

Versions can be copied and moved.

### Observers

```cpp
I1 major() const noexcept;
I2 minor() const noexcept;
I3 patch() const noexcept;

std::string_view prerelease_tag() const noexcept;
std::string_view build_metadata() const noexcept;
bool is_prerelease() const noexcept;
bool has_build_metadata() const noexcept;
std::string to_string() const;
```

Returned string views remain valid until the version is modified or destroyed.

### Transformations

```cpp
version bump_major() const;
version bump_minor() const;
version bump_patch() const;
version without_prerelease() const;
version without_build_metadata() const;
```

These functions return a new version. Bumps clear both qualifiers, reset lower components to zero, and throw `std::overflow_error` on overflow. Each `without_*` function removes only the named qualifier.

```cpp
const semver::version<> current{1, 2, 3, "rc.1", "ci"};
const auto next = current.bump_minor(); // 1.3.0
const auto release = current.without_prerelease(); // 1.2.3+ci
const auto reproducible = current.without_build_metadata(); // 1.2.3-rc.1
```

`swap(version&, version&)` is available through argument-dependent lookup and is `noexcept`.

## Result types

```cpp
struct from_chars_result {
  const char* ptr;
  std::errc ec;
  explicit operator bool() const noexcept;
};

struct to_chars_result {
  char* ptr;
  std::errc ec;
  explicit operator bool() const noexcept;
};
```

A result converts to `true` when `ec == std::errc{}`. Parse errors use `invalid_argument`, `result_out_of_range`, or `value_too_large`.

## Parsing

### Strict parsing

```cpp
template <typename I1, typename I2, typename I3>
from_chars_result parse(std::string_view input, version<I1, I2, I3>& output);

template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
from_chars_result from_chars(const char* first, const char* last, version<I1, I2, I3>& output);

template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
std::optional<version<I1, I2, I3>> try_parse(std::string_view input);

template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
version<I1, I2, I3> from_string(std::string_view input);

template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
bool valid(std::string_view input);
```

All parsing respects the component types and `SEMVER_MAX_INPUT_LENGTH`.

- `parse` requires the whole string to be valid.
- `from_chars` reads the longest SemVer prefix; `ptr` points past it. If no complete version prefix exists, it returns `invalid_argument` with `ptr == first`.
- `try_parse` returns `std::nullopt` on a parse error.
- `from_string` throws `std::system_error` on a parse error.
- `valid` checks whether the whole string is valid and fits the chosen limits.

`parse` and `from_chars` leave `output` unchanged on failure. Parsing may allocate; allocation errors propagate to the caller.

```cpp
semver::version<> v;
const auto result = semver::parse("1.2.3-alpha+build.7", v);
if (!result) {
  // result.ptr points at the failing byte.
}
```

### Cleaning and coercion

```cpp
template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
std::optional<version<I1, I2, I3>> clean(std::string_view input);

template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
std::optional<version<I1, I2, I3>> coerce(std::string_view input);
```

`clean` removes outer spaces, an optional `=`, and an optional `v`/`V`, in that order. Spaces between prefixes are allowed. It then parses strictly.

`coerce` also accepts leading zeros and fills missing minor or patch components with zero. It reads from the start, ignores trailing text, and keeps prerelease and build metadata if the suffix is valid.

```cpp
semver::from_string("1.0"); // throws: strict parsing requires MAJOR.MINOR.PATCH
semver::coerce("1.0");      // returns an optional containing 1.0.0
```

## Comparison

All relational operators accept versions with different component types. They implement SemVer precedence and ignore build metadata.

```cpp
bool operator==(const version<...>& lhs, const version<...>& rhs) noexcept;
bool operator!=(const version<...>& lhs, const version<...>& rhs) noexcept;
bool operator<(const version<...>& lhs, const version<...>& rhs) noexcept;
bool operator<=(const version<...>& lhs, const version<...>& rhs) noexcept;
bool operator>(const version<...>& lhs, const version<...>& rhs) noexcept;
bool operator>=(const version<...>& lhs, const version<...>& rhs) noexcept;
```

On C++20, `operator<=>` returns `std::weak_ordering`. Versions that differ only in build metadata are equivalent.

### Comparison utilities

```cpp
int compare(const version<...>& lhs, const version<...>& rhs) noexcept;
int compare_with_build(const version<...>& lhs, const version<...>& rhs) noexcept;
```

Both functions return `-1`, `0`, or `1`. `compare` uses SemVer precedence. `compare_with_build` also compares build metadata lexicographically when precedence is equal.

```cpp
enum class version_change : std::uint8_t {
  none,
  major,
  minor,
  patch,
  premajor,
  preminor,
  prepatch,
  prerelease
};

version_change diff(const version<...>& lhs, const version<...>& rhs) noexcept;
```

`diff` reports the first differing component, using a `pre*` value when the newer version is a prerelease. It ignores build metadata and returns `none` for equal precedence. Use the same enum with `inc` to request a change.

## Serialization

```cpp
template <typename I1, typename I2, typename I3>
to_chars_result to_chars(char* first, char* last, const version<I1, I2, I3>& value) noexcept;

template <typename Traits, typename I1, typename I2, typename I3>
std::basic_ostream<char, Traits>& operator<<(std::basic_ostream<char, Traits>& stream, const version<I1, I2, I3>& value);
```

`to_chars` writes without allocating or adding a null terminator. On success, `ptr` points past the written bytes. A null, reversed, or undersized buffer returns `value_too_large` without writing.

`version::to_string()` returns the canonical `MAJOR.MINOR.PATCH[-prerelease][+build]` form.

`operator<<` writes to narrow character streams. Numeric base flags are ignored; width and alignment apply to the whole version.

When available, `std::format("{}", value)` accepts standard string format options, including width, fill, alignment, and precision.

`std::hash<semver::version<...>>` ignores build metadata, just like equality.

## Ranges

This library defines the following range syntax; SemVer 2.0.0 does not define ranges.

```cpp
template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
class range_set {
public:
  template <typename J1, typename J2, typename J3>
  bool contains(const version<J1, J2, J3>& value, prerelease_policy policy = prerelease_policy::exclude) const noexcept;
};

template <typename I1, typename I2, typename I3>
from_chars_result parse(std::string_view input, range_set<I1, I2, I3>& output);

template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
std::optional<range_set<I1, I2, I3>> try_parse_range(std::string_view input);
```

Range bounds must fit the `range_set` component types. Versions tested against a range may use different or wider types and are compared without narrowing. A default-constructed `range_set` contains nothing; use `*`, `x`, or `X` to match any release.

`parse` leaves `output` unchanged on failure and reports the error code and position in the input. A bound that does not fit returns `std::errc::result_out_of_range`. `try_parse_range` returns `std::nullopt` on a parse error.

Separate constraints with whitespace to require all of them (AND). Join branches with `||` to accept either (OR). All ASCII whitespace is accepted around ranges, between constraints, and after operators. Empty input and empty `||` branches are invalid.

- Use a complete version (`1.2.3`) or a partial version (`1` or `1.2`). Components are decimal integers without leading zeros.
- `>=`, `<`, `~`, and `^` accept complete or partial versions. `>`, `<=`, `=`, and `!=` require complete versions.
- `*`, `x`, and `X` work alone or as the last component (`1.x`, `1.2.*`), without an operator.
- Prerelease tags require a complete version. Build metadata is not accepted in range bounds.

The table shows bounds for the default policy. See [Prerelease matching](#prerelease-matching) for `prerelease_policy::include`.

| Form | Expansion or effect |
| --- | --- |
| `*`, `x`, or `X` | no bounds |
| `1.2.3` | `=1.2.3` |
| `1`, `1.*`, or `1.x` | `>=1.0.0 <2.0.0-0` |
| `1.2`, `1.2.*`, or `1.2.x` | `>=1.2.0 <1.3.0-0` |
| `>=1.2` | `>=1.2.0` |
| `<2` | `<2.0.0-0` |
| `!=1.2.3` | excludes `1.2.3` by SemVer precedence; does not enable prerelease matching |
| `~1` | `>=1.0.0 <2.0.0-0` |
| `~1.2` | `>=1.2.0 <1.3.0-0` |
| `^1.2` | `>=1.2.0 <2.0.0-0` |
| `^0.2.3` | `>=0.2.3 <0.3.0-0` |
| `^0.0.3` | `>=0.0.3 <0.0.4-0` |

```cpp
const auto range = semver::try_parse_range(" >=1.2 <2 || ~3.1 ");
assert(range);

range->contains(semver::version<>{1, 5, 0}); // true
range->contains(semver::version<>{2, 0, 0}); // false
```

### Prerelease matching

```cpp
enum class prerelease_policy : std::uint8_t {
  exclude,
  include
};
```

By default, a prerelease must satisfy all constraints in a branch that explicitly names a prerelease with the same major, minor, and patch. A `!=` constraint does not enable prerelease matching.

For example, `>=1.2.3-alpha <2` can match `1.2.3-beta`, but not `1.3.0-beta`.

`prerelease_policy::include` removes this filter; `*`, `x`, and `X` then match every version. Shorthand bounds work as follows:

- Partial ranges and partial `>=` constraints start at `-0`; for example, `>=1.2` starts at `1.2.0-0`.
- Partial `<` constraints end at `-0`; `<2` means `<2.0.0-0`.
- Caret ranges start at `-0` when components are omitted or the major version is zero.
- Tilde ranges, complete comparators, and complete caret ranges with a nonzero major keep their release lower bound.

An explicit prerelease tag always remains the exact bound.

Partial, tilde, and caret ranges exclude the next version line and its prereleases. For example, `1.2` ends at `<1.3.0-0`.

### Range utilities

```cpp
bool satisfies(const version<...>& value, std::string_view range, prerelease_policy policy = prerelease_policy::exclude);

ForwardIt min_satisfying(ForwardIt first, ForwardIt last, const range_set<...>& range, prerelease_policy policy = prerelease_policy::exclude);

ForwardIt max_satisfying(ForwardIt first, ForwardIt last, const range_set<...>& range, prerelease_policy policy = prerelease_policy::exclude);

std::optional<version<...>> min_version(const range_set<...>& range, prerelease_policy policy = prerelease_policy::exclude);

bool intersects(const range_set<...>& lhs, const range_set<...>& rhs, prerelease_policy policy = prerelease_policy::exclude);
```

`satisfies` parses the range using the version's component types and returns `false` on a parse error. To reuse a parsed range, call `range.contains(value, policy)`. `min_satisfying` and `max_satisfying` return `last` when no element matches.

`min_version` returns the lowest matching version that fits the range's component types, or `std::nullopt` if none does. For `range_set<std::uint8_t>`, `>1.2.255` has minimum `1.3.0`, while `>255.255.255` has no representable match.

`intersects` checks whether two ranges share any version. Their component types may differ, and each range applies its own prerelease filter. The shared version need not fit the stored types: two `range_set<std::uint8_t>` values parsed from `>255.255.255` still intersect.

## Incrementing

```cpp
template <typename I1, typename I2, typename I3>
std::optional<version<I1, I2, I3>> inc(const version<I1, I2, I3>& value, version_change change, std::string_view prerelease = {});
```

```cpp
const semver::version<> current{1, 2, 3};
const auto next = semver::inc(current, semver::version_change::patch);
```

For `major`, `minor`, and `patch`, the prerelease argument must be empty. Other changes accept a complete prerelease tag without adding a `.0` suffix. Invalid tags, unsupported changes (including `none`), and component overflow return `std::nullopt`.

`major`, `minor`, and `patch` work like the corresponding `bump_*` member and clear both qualifiers. For example, a patch increment of `1.2.3-rc.1` produces `1.2.4`.

Without an explicit tag, `premajor`, `preminor`, and `prepatch` use `0`. `prerelease` increments the last identifier if it is numeric (`alpha.9` becomes `alpha.10`), or appends `.0` otherwise. For a release, it bumps patch and adds `-0`.

## Literals

When `SEMVER_HAS_CONSTEVAL_LITERAL == 1`, the literal is available in `semver::literals`:

```cpp
using namespace semver::literals;
constexpr auto v = "1.2.3-alpha"_semver;
```

Invalid literals fail constant evaluation.

## Constants and feature flags

```cpp
#define SEMVER_VERSION_MAJOR ...
#define SEMVER_VERSION_MINOR ...
#define SEMVER_VERSION_PATCH ...
#define SEMVER_MAX_INPUT_LENGTH 512 // configurable
#define SEMVER_HAS_CONSTEXPR 0-or-1
#define SEMVER_HAS_CONSTEXPR_CORE 0-or-1
#define SEMVER_HAS_CONSTEXPR_OPTIONAL 0-or-1
#define SEMVER_HAS_CONSTEXPR_RANGES 0-or-1
#define SEMVER_HAS_CONSTEVAL_LITERAL 0-or-1

inline constexpr std::size_t max_input_length;
inline const version<> library_version;
```

Define `SEMVER_CONFIG_FILE` to a quoted header path before including `semver.hpp` to load configuration first. See [limitations](limitations.md#configuration-and-input-size).
