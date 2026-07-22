# Reference

`semver` is a header-only C++17 library. Include the public header before using any API:

```cpp
#include <semver.hpp>
```

Read the [limitations](limitations.md) for component bounds, input limits, range grammar, feature detection, and system macro behavior.

Functions marked `SEMVER_CONSTEXPR` in the header are `constexpr` when `SEMVER_HAS_CONSTEXPR == 1` and otherwise remain inline runtime APIs. The signatures below show `noexcept` wherever it is guaranteed.

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

The component types must be unqualified unsigned integral types other than `bool`. The three types may differ. `version<>` uses `std::uint32_t` for every component.

### Construction

```cpp
version();

template <typename T1, typename T2, typename T3>
version(T1 major, T2 minor, T3 patch);

template <typename T1, typename T2, typename T3>
version(T1 major, T2 minor, T3 patch, std::string_view prerelease, std::string_view build = {});
```

The default value is `0.1.0`. Integral constructor arguments are checked before conversion. Negative or unrepresentable components throw `std::out_of_range`; invalid prerelease or build identifiers throw `std::invalid_argument`.

Class template argument deduction uses at least `std::uint32_t` for each component and widens components whose source type is wider:

```cpp
semver::version v{1, 2, 3}; // semver::version<std::uint32_t>
semver::version wide{std::uint64_t{5'000'000'000}, 2, 3};
// semver::version<std::uint64_t, std::uint32_t, std::uint32_t>
```

Every deduced component type is unsigned. A wider signed source therefore selects the corresponding unsigned width, while a negative value is still rejected.

`version` is copyable and movable. Copy and move operations preserve the components, prerelease tag, and build metadata.

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

The functions return a new version and do not modify the source. The bump functions clear prerelease and build metadata, reset lower components as required, and throw `std::overflow_error` when the incremented component is already at its maximum. `without_prerelease` and `without_build_metadata` remove only the selected qualifier and preserve the other one.

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

Strict parsing is bounded by the selected component types and `SEMVER_MAX_INPUT_LENGTH`. `parse` requires the entire string and preserves `output` on failure. `from_chars` follows standard pointer/error and longest-prefix semantics: when no complete SemVer prefix exists, it returns `invalid_argument` with `ptr == first`. It may allocate and propagate allocation failures. `try_parse` returns `std::nullopt`; `from_string` throws `std::system_error`. `valid` returns whether the complete input is a valid SemVer representable by the selected component types and configured input limit.

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

`clean` removes outer spaces and optional `=` and `v`/`V` prefixes, allowing spaces between them, then parses a complete SemVer string strictly. `coerce` accepts the same wrappers, leading zeros, and missing minor or patch components. It reads from the beginning of the cleaned input, ignores trailing text, and preserves a valid qualifier suffix when possible.

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

`compare` returns `-1`, `0`, or `1`. `compare_with_build` first applies normal SemVer precedence, then compares the complete build metadata lexicographically. It provides deterministic artifact ordering and may distinguish versions that are equivalent under normal SemVer comparison.

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

`version_change` is shared by `diff` and `inc`: it describes the observed or requested version change. `diff` reports the first differing component, returns a `pre*` value when the newer operand has a prerelease tag, and ignores build metadata. Versions with equal SemVer precedence therefore produce `none`.

## Serialization

```cpp
template <typename I1, typename I2, typename I3>
to_chars_result to_chars(char* first, char* last, const version<I1, I2, I3>& value) noexcept;

template <typename Traits, typename I1, typename I2, typename I3>
std::basic_ostream<char, Traits>& operator<<(std::basic_ostream<char, Traits>& stream, const version<I1, I2, I3>& value);
```

`to_chars` performs no allocation. On success, `ptr` points one past the last written byte; no null terminator is added. A null, reversed, or undersized buffer returns `value_too_large` and does not partially serialize the version.

`version::to_string()` returns the canonical `MAJOR.MINOR.PATCH[-prerelease][+build]` form.

Stream insertion supports narrow character streams and writes the canonical version as one value. Numeric base flags do not affect components, and width and alignment apply to the complete version.

When `<format>` support is available, `std::formatter<semver::version<...>>` enables `std::format("{}", value)`. Format specifications other than `{}` are rejected.

`std::hash<semver::version<...>>` is provided. It follows equality and therefore excludes build metadata.

## Ranges

Range syntax is defined by this library and is not part of the SemVer 2.0.0 specification. This section is the normative range contract.

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

`range_set` component types control the representable explicit and generated bounds. Candidate versions may use different component types and are compared without conversion or narrowing. A default-constructed `range_set` contains nothing. `*`, `x`, and `X` are the any range. Empty and whitespace-only inputs are invalid.

Range parsing is transactional: failure preserves `output`. The result pointer addresses the original input, the raw input-length limit is checked before whitespace handling, and an unrepresentable stored bound reports `std::errc::result_out_of_range`. `try_parse_range` returns `std::nullopt` on any parse error; use `parse` when the error code or position is needed.

The grammar is intentionally compact:

```text
range-set    = intersection ("||" intersection)*
intersection = term (whitespace term)*
term         = wildcard-any | selector | comparator | tilde | caret
selector     = version | partial | wildcard
comparator   = (">=" | "<") (version | partial)
             | (">" | "<=" | "=" | "!=") version
tilde        = "~" (version | partial)
caret        = "^" (version | partial)
version      = M "." m "." p ["-" prerelease]
partial      = M | M "." m
wildcard-any = "*" | "x" | "X"
wildcard     = M "." wildcard-any | M "." m "." wildcard-any
```

`M`, `m`, and `p` are decimal components without leading zeros. Prerelease qualifiers require a complete version. Build metadata is not accepted in range boundaries because it does not affect SemVer precedence.

Spaces are optional after range operators. All six ASCII whitespace characters are accepted around the range and between intersection terms. Empty input, empty `||` branches, wildcard operands for `~`, `^`, or comparators, and partial operands for `>`, `<=`, `=`, or `!=` are invalid.

| Form | Expansion or effect |
| --- | --- |
| `*`, `x`, or `X` | no bounds |
| `1`, `1.*`, or `1.x` | `>=1.0.0 <2.0.0-0` |
| `1.2`, `1.2.*`, or `1.2.x` | `>=1.2.0 <1.3.0-0` |
| `>=1.2` | `>=1.2.0` |
| `<2` | `<2.0.0` |
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

By default, a prerelease candidate matches only when a non-exclusion comparator in the same intersection explicitly contains a prerelease with the same major, minor, and patch tuple. A `!=` comparator does not enable prerelease matching by itself.

Consequently, `*`, `x`, and `X` match every release by default and every version with `prerelease_policy::include`.

For the supported grammar, `prerelease_policy::include` uses explicit generated prerelease boundaries. Partial ranges and partial `>=` comparators use a generated `-0` lower bound, and partial `<` comparators use a `-0` upper boundary. Caret ranges use a `-0` lower bound when components are omitted or the complete major version is zero. Tilde ranges, complete comparators, and complete caret ranges with a nonzero major retain their ordinary release lower bound. An explicitly written prerelease remains the exact boundary.

Generated upper bounds use the next version line with prerelease `0`. This keeps the next line and all of its prereleases outside partial, tilde, and caret ranges. For example, `1.2` has the upper bound `<1.3.0-0`.

### Range utilities

```cpp
bool satisfies(const version<...>& value, std::string_view range, prerelease_policy policy = prerelease_policy::exclude);

ForwardIt min_satisfying(ForwardIt first, ForwardIt last, const range_set<...>& range, prerelease_policy policy = prerelease_policy::exclude);

ForwardIt max_satisfying(ForwardIt first, ForwardIt last, const range_set<...>& range, prerelease_policy policy = prerelease_policy::exclude);

std::optional<version<...>> min_version(const range_set<...>& range, prerelease_policy policy = prerelease_policy::exclude);

bool intersects(const range_set<...>& lhs, const range_set<...>& rhs, prerelease_policy policy = prerelease_policy::exclude);
```

The string `satisfies` overload parses into the candidate's component types and returns `false` for invalid input. For a pre-parsed range, use `range.contains(value, policy)`. `min_satisfying` and `max_satisfying` return `last` when no element matches.

`min_version` returns the lowest matching version representable by the component types of the `range_set`. It returns `std::nullopt` for an empty set or when the mathematical minimum cannot be represented, such as `>255.255.255` in `range_set<std::uint8_t>`.

`intersects` accepts independently typed range sets and returns whether any semantic version can satisfy both. Each range applies its own prerelease filter. Unlike `min_version`, the result is not limited by the component type needed to return a concrete version, so two unbounded `>255.255.255` ranges intersect even when stored with `std::uint8_t` components.

## Incrementing

```cpp
template <typename I1, typename I2, typename I3>
std::optional<version<I1, I2, I3>> inc(const version<I1, I2, I3>& value, version_change change, std::string_view prerelease = {});
```

```cpp
const semver::version<> current{1, 2, 3};
const auto next = semver::inc(current, semver::version_change::patch);
```

`inc` returns `std::nullopt` for `version_change::none`, an unsupported change, component overflow, an invalid prerelease argument, or any prerelease argument supplied with `major`, `minor`, or `patch`. When accepted, the argument is the complete prerelease tag, not an identifier prefix, and does not receive an automatic `.0` suffix.

For `major`, `minor`, and `patch`, `inc` has the same arithmetic behavior as the corresponding `bump_*` member and clears existing qualifiers. In particular, incrementing `1.2.3-rc.1` as `patch` produces `1.2.4`, not `1.2.3`.

Without an explicit tag, `premajor`, `preminor`, and `prepatch` use `0`. `prerelease` increments a final numeric identifier as arbitrary-length decimal text, appends `.0` to a final non-numeric identifier, or bumps patch and adds `-0` when the source is a release.

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
#define SEMVER_HAS_CONSTEVAL_LITERAL 0-or-1

inline constexpr std::size_t max_input_length;
inline const version<> library_version;
```

Define `SEMVER_CONFIG_FILE` to a quoted header path before including `semver.hpp` to load configuration first. See [limitations](limitations.md#configuration-and-input-size).
