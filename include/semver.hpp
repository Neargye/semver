//          _____                            _   _
//         / ____|                          | | (_)
//        | (___   ___ _ __ ___   __ _ _ __ | |_ _  ___
//         \___ \ / _ \ '_ ` _ \ / _` | '_ \| __| |/ __|
//         ____) |  __/ | | | | | (_| | | | | |_| | (__
//        |_____/ \___|_| |_| |_|\__,_|_| |_|\__|_|\___|
// __      __           _             _                _____
// \ \    / /          (_)           (_)              / ____|_     _
//  \ \  / /__ _ __ ___ _  ___  _ __  _ _ __   __ _  | |   _| |_ _| |_
//   \ \/ / _ \ '__/ __| |/ _ \| '_ \| | '_ \ / _` | | |  |_   _|_   _|
//    \  /  __/ |  \__ \ | (_) | | | | | | | | (_| | | |____|_|   |_|
//     \/ \___|_|  |___/_|\___/|_| |_|_|_| |_|\__, |  \_____|
// https://github.com/Neargye/semver           __/ |
// version 1.0.0                              |___/
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2026 Daniil Goncharov <neargye@gmail.com>.
// Copyright (c) 2020 - 2025 Alexander Gorbunov <naratzul@gmail.com>.
//
// Permission is hereby  granted, free of charge, to any  person obtaining a copy
// of this software and associated  documentation files (the "Software"), to deal
// in the Software  without restriction, including without  limitation the rights
// to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
// copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
// IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
// FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
// AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
// LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef NEARGYE_SEMANTIC_VERSIONING_HPP
#define NEARGYE_SEMANTIC_VERSIONING_HPP

#define SEMVER_VERSION_MAJOR 1
#define SEMVER_VERSION_MINOR 0
#define SEMVER_VERSION_PATCH 0

#if defined(SEMVER_CONFIG_FILE)
#  include SEMVER_CONFIG_FILE
#endif

#ifndef SEMVER_MAX_INPUT_LENGTH
#  define SEMVER_MAX_INPUT_LENGTH 512
#endif

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(__has_include) && __has_include(<version>)
#  include <version>
#endif

#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
#  include <format>
#endif

#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

#if defined(__cpp_impl_three_way_comparison) && __cpp_impl_three_way_comparison >= 201907L
#  include <compare>
#endif

// Clang with libstdc++ < 13 cannot constexpr-evaluate std::string.
#if defined(__cpp_lib_constexpr_string) && __cpp_lib_constexpr_string >= 201907L && defined(__cpp_lib_constexpr_vector) && __cpp_lib_constexpr_vector >= 201907L
#  if defined(__clang__) && defined(__GLIBCXX__) && (!defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE < 13)
#    define SEMVER_HAS_CONSTEXPR 0
#    define SEMVER_CONSTEXPR inline
#  else
#    define SEMVER_HAS_CONSTEXPR 1
#    define SEMVER_CONSTEXPR constexpr
#  endif
#else
#  define SEMVER_HAS_CONSTEXPR 0
#  define SEMVER_CONSTEXPR inline
#endif

// MSVC cannot return version<> from a consteval literal.
// GCC with libstdc++ < 14 cannot propagate consteval parse failures.
#if defined(__cpp_consteval) && __cpp_consteval >= 201811L && SEMVER_HAS_CONSTEXPR && !defined(_MSC_VER) && !(defined(__GLIBCXX__) && !defined(__clang__) && (!defined(__GNUC__) || __GNUC__ < 14 || !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE < 14))
#  define SEMVER_HAS_CONSTEVAL_LITERAL 1
#else
#  define SEMVER_HAS_CONSTEVAL_LITERAL 0
#endif

// Avoid system major/minor macros.
#ifdef major
#  undef major
#endif
#ifdef minor
#  undef minor
#endif

namespace semver {

  inline constexpr std::size_t max_input_length = SEMVER_MAX_INPUT_LENGTH;

  namespace detail {

    constexpr bool is_digit(char c) noexcept {
      return c >= '0' && c <= '9';
    }

    constexpr bool is_letter(char c) noexcept {
      return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
    }

    constexpr bool is_identifier_char(char c) noexcept {
      return is_digit(c) || is_letter(c) || c == '-';
    }

    constexpr bool is_numeric_identifier(std::string_view id) noexcept {
      if (id.empty())
        return false;

      for (char c : id) {
        if (!is_digit(c))
          return false;
      }
      return true;
    }

    constexpr bool validate_identifier(std::string_view id, bool reject_numeric_leading_zero) noexcept {
      if (id.empty())
        return false;

      for (char c : id) {
        if (!is_identifier_char(c))
          return false;
      }

      return !reject_numeric_leading_zero || id.size() == 1 || id.front() != '0' || !is_numeric_identifier(id);
    }

    template <typename Int>
    constexpr std::size_t length(Int n) noexcept {
      auto un = n;
      std::size_t digits = 0;
      do { ++digits; un /= 10; } while (un != 0);
      return digits;
    }

    template <typename OutputIt, typename Int>
    constexpr void uint_write_backward(OutputIt dest, Int n) noexcept {
      auto un = n;
      do {
        *(--dest) = static_cast<char>('0' + (un % 10));
        un /= 10;
      } while (un != 0);
    }

    template <typename I1, typename I2, typename I3>
    constexpr std::size_t serialized_length(I1 major, I2 minor, I3 patch, std::string_view prerelease, std::string_view build) noexcept {
      return length(major) + length(minor) + length(patch) + 2 + (prerelease.empty() ? 0 : prerelease.size() + 1) + (build.empty() ? 0 : build.size() + 1);
    }

    constexpr bool validate_identifiers(std::string_view tag, bool reject_numeric_leading_zero) noexcept {
      if (tag.empty())
        return true;

      std::size_t start = 0;
      for (;;) {
        const auto dot = tag.find('.', start);
        const auto len = (dot == std::string_view::npos) ? tag.size() - start : dot - start;
        if (len == 0)
          return false; // empty identifier (leading/trailing/double dot)

        if (!validate_identifier(tag.substr(start, len), reject_numeric_leading_zero))
          return false;
        if (dot == std::string_view::npos)
          break;
        start = dot + 1;
      }
      return true;
    }

    constexpr bool validate_prerelease_tag(std::string_view tag) noexcept {
      return validate_identifiers(tag, true);
    }

    // Leading zeroes are allowed.
    constexpr bool validate_build_metadata(std::string_view tag) noexcept {
      return validate_identifiers(tag, false);
    }

    // Syntax-only check used when a component does not fit the destination type.
    constexpr bool validate_version_syntax(std::string_view str) noexcept {
      std::size_t pos = 0;
      const auto parse_component = [&]() constexpr {
        if (pos >= str.size() || !is_digit(str[pos]))
          return false;

        if (str[pos] == '0' && pos + 1 < str.size() && is_digit(str[pos + 1]))
          return false;

        do { ++pos; } while (pos < str.size() && is_digit(str[pos]));
        return true;
      };

      const auto consume_dot = [&]() constexpr {
        if (pos >= str.size() || str[pos] != '.')
          return false;

        ++pos;
        return true;
      };

      if (!parse_component() || !consume_dot() ||
          !parse_component() || !consume_dot() ||
          !parse_component())
        return false;

      if (pos == str.size())
        return true;

      if (str[pos] == '-') {
        const auto start = ++pos;
        while (pos < str.size() && str[pos] != '+')
          ++pos;
        if (pos == start || !validate_prerelease_tag(str.substr(start, pos - start)))
          return false;
      }

      if (pos < str.size() && str[pos] == '+') {
        const auto start = ++pos;
        if (pos == str.size() || !validate_build_metadata(str.substr(start)))
          return false;
        pos = str.size();
      }

      return pos == str.size();
    }

    template<class T, class U>
    constexpr bool cmp_less(T t, U u) noexcept {
      if constexpr (std::is_signed_v<T> == std::is_signed_v<U>)
        return t < u;
      else if constexpr (std::is_signed_v<T>)
        return t < 0 || std::make_unsigned_t<T>(t) < u;
      else
        return u >= 0 && t < std::make_unsigned_t<U>(u);
    }

    template<class T, class U>
    constexpr int compare_numbers(T lhs, U rhs) noexcept {
      return cmp_less(lhs, rhs) ? -1 : cmp_less(rhs, lhs) ? 1 : 0;
    }

    template<typename R, typename T>
    constexpr bool number_in_range(T t) noexcept {
      return !cmp_less(t, std::numeric_limits<R>::min()) && !cmp_less(std::numeric_limits<R>::max(), t);
    }

    template <typename R1, typename R2, typename R3, typename T1, typename T2, typename T3>
    constexpr bool version_components_in_range(T1 major, T2 minor, T3 patch) noexcept {
      return number_in_range<R1>(major) && number_in_range<R2>(minor) && number_in_range<R3>(patch);
    }

    template <typename T>
    inline constexpr bool is_component_source_v = std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>;

    template <typename... T>
    using enable_if_component_sources_t = std::enable_if_t<(is_component_source_v<T> && ...), int>;

    template <typename T>
    using deduced_component_t = std::make_unsigned_t<std::common_type_t<std::uint32_t, std::remove_cv_t<T>>>;

    template <typename T>
    inline constexpr bool is_component_type_v = is_component_source_v<T> && std::is_unsigned_v<T> && std::is_same_v<T, std::remove_cv_t<T>>;

    template <typename... T>
    inline constexpr bool are_component_types_v = (is_component_type_v<T> && ...);

    template <typename R, typename T>
    inline constexpr bool is_nothrow_component_cast_v = std::is_unsigned_v<std::remove_cv_t<T>> && std::numeric_limits<std::remove_cv_t<T>>::digits <= std::numeric_limits<std::remove_cv_t<R>>::digits;

    template <typename R1, typename R2, typename R3, typename T1, typename T2, typename T3>
    inline constexpr bool are_nothrow_component_casts_v = is_nothrow_component_cast_v<R1, T1> && is_nothrow_component_cast_v<R2, T2> && is_nothrow_component_cast_v<R3, T3>;

    template <typename R, typename T>
    constexpr R component_cast(T value) noexcept(is_nothrow_component_cast_v<R, T>) {
      if constexpr (!is_nothrow_component_cast_v<R, T>) {
        if (!number_in_range<R>(value))
          throw std::out_of_range{"semver: version component out of range"};
      }
      return static_cast<R>(value);
    }

    class version_parser;
  }

  template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
#if defined(__cpp_concepts) && __cpp_concepts >= 201907L
    requires detail::are_component_types_v<I1, I2, I3>
#endif
  class version {
    static_assert(detail::is_component_type_v<I1>, "semver: I1 must be an unsigned integral type");
    static_assert(detail::is_component_type_v<I2>, "semver: I2 must be an unsigned integral type");
    static_assert(detail::is_component_type_v<I3>, "semver: I3 must be an unsigned integral type");

    friend class detail::version_parser;

  public:
    version() = default; // default is 0.1.0, see semver.org FAQ §4

    template <typename T1, typename T2, typename T3, detail::enable_if_component_sources_t<T1, T2, T3> = 0>
    SEMVER_CONSTEXPR version(T1 major, T2 minor, T3 patch) noexcept(detail::are_nothrow_component_casts_v<I1, I2, I3, T1, T2, T3>) : major_(detail::component_cast<I1>(major)), minor_(detail::component_cast<I2>(minor)), patch_(detail::component_cast<I3>(patch)) {}

    template <typename T1, typename T2, typename T3, detail::enable_if_component_sources_t<T1, T2, T3> = 0>
    SEMVER_CONSTEXPR version(T1 major, T2 minor, T3 patch, std::string_view prerelease, std::string_view build = {}) : version(major, minor, patch) {
      if (!detail::validate_prerelease_tag(prerelease))
        throw std::invalid_argument{"semver: invalid prerelease identifier"};
      if (!detail::validate_build_metadata(build))
        throw std::invalid_argument{"semver: invalid build metadata"};

      prerelease_tag_.assign(prerelease);
      build_metadata_.assign(build);
    }

    SEMVER_CONSTEXPR friend void swap(version& a, version& b) noexcept {
      using std::swap;
      swap(a.major_, b.major_);
      swap(a.minor_, b.minor_);
      swap(a.patch_, b.patch_);
      swap(a.prerelease_tag_, b.prerelease_tag_);
      swap(a.build_metadata_, b.build_metadata_);
    }

    [[nodiscard]] SEMVER_CONSTEXPR I1 major() const noexcept { return major_; }
    [[nodiscard]] SEMVER_CONSTEXPR I2 minor() const noexcept { return minor_; }
    [[nodiscard]] SEMVER_CONSTEXPR I3 patch() const noexcept { return patch_; }

    // Returns (major+1).0.0, clears qualifiers, and throws on overflow.
    [[nodiscard]] SEMVER_CONSTEXPR version<I1, I2, I3> bump_major() const {
      if (major_ == std::numeric_limits<I1>::max()) {
        throw std::overflow_error{"semver: bump_major overflow"};
      }
      return version<I1, I2, I3>{static_cast<I1>(major_ + I1{1}), I2{}, I3{}};
    }
    // Returns major.(minor+1).0, clears qualifiers, and throws on overflow.
    [[nodiscard]] SEMVER_CONSTEXPR version<I1, I2, I3> bump_minor() const {
      if (minor_ == std::numeric_limits<I2>::max()) {
        throw std::overflow_error{"semver: bump_minor overflow"};
      }
      return version<I1, I2, I3>{major_, static_cast<I2>(minor_ + I2{1}), I3{}};
    }
    // Returns major.minor.(patch+1), clears qualifiers, and throws on overflow.
    [[nodiscard]] SEMVER_CONSTEXPR version<I1, I2, I3> bump_patch() const {
      if (patch_ == std::numeric_limits<I3>::max()) {
        throw std::overflow_error{"semver: bump_patch overflow"};
      }
      return version<I1, I2, I3>{major_, minor_, static_cast<I3>(patch_ + I3{1})};
    }

    // Removes the pre-release tag and preserves build metadata.
    [[nodiscard]] SEMVER_CONSTEXPR version<I1, I2, I3> without_prerelease() const {
      return version<I1, I2, I3>{major_, minor_, patch_, {}, build_metadata_};
    }

    // Removes build metadata and preserves the pre-release tag.
    [[nodiscard]] SEMVER_CONSTEXPR version<I1, I2, I3> without_build_metadata() const {
      return version<I1, I2, I3>{major_, minor_, patch_, prerelease_tag_};
    }

    // Pre-release identifier (e.g. "alpha.1"). Empty if absent.
    [[nodiscard]] SEMVER_CONSTEXPR std::string_view prerelease_tag() const noexcept { return prerelease_tag_; }
    // Build metadata (e.g. "build.42"). Empty if absent. Excluded from comparisons and hash (spec §10).
    [[nodiscard]] SEMVER_CONSTEXPR std::string_view build_metadata() const noexcept { return build_metadata_; }

    [[nodiscard]] SEMVER_CONSTEXPR bool is_prerelease()      const noexcept { return !prerelease_tag_.empty(); }
    [[nodiscard]] SEMVER_CONSTEXPR bool has_build_metadata() const noexcept { return !build_metadata_.empty(); }

    // Serializes to "MAJOR.MINOR.PATCH[-prerelease][+build]".
    [[nodiscard]] SEMVER_CONSTEXPR std::string to_string() const {
      std::string result(detail::serialized_length(major_, minor_, patch_, prerelease_tag_, build_metadata_), '\0');
      (void)to_chars(result.data(), result.data() + result.size(), *this);
      return result;
    }

  private:
    I1 major_ = 0;
    I2 minor_ = 1;
    I3 patch_ = 0;
    std::string prerelease_tag_;
    std::string build_metadata_;
  };

  template <typename T1, typename T2, typename T3, detail::enable_if_component_sources_t<T1, T2, T3> = 0>
  version(T1, T2, T3) -> version<detail::deduced_component_t<T1>, detail::deduced_component_t<T2>, detail::deduced_component_t<T3>>;

  template <typename T1, typename T2, typename T3, detail::enable_if_component_sources_t<T1, T2, T3> = 0>
  version(T1, T2, T3, std::string_view) -> version<detail::deduced_component_t<T1>, detail::deduced_component_t<T2>, detail::deduced_component_t<T3>>;

  template <typename T1, typename T2, typename T3, detail::enable_if_component_sources_t<T1, T2, T3> = 0>
  version(T1, T2, T3, std::string_view, std::string_view) -> version<detail::deduced_component_t<T1>, detail::deduced_component_t<T2>, detail::deduced_component_t<T3>>;

  // Follows std::from_chars conventions.
  struct from_chars_result {
    const char* ptr;
    std::errc ec;

    [[nodiscard]] explicit constexpr operator bool() const noexcept { return ec == std::errc{}; }
  };

  // Follows std::to_chars conventions.
  struct to_chars_result {
    char* ptr;
    std::errc ec;

    [[nodiscard]] explicit constexpr operator bool() const noexcept { return ec == std::errc{}; }
  };

  // Controls pre-release filtering in range matching.
  enum class prerelease_policy : std::uint8_t {
    exclude, // pre-release only matches if a comparator explicitly targets the same M.m.p pre-release
    include  // disables the default pre-release filter
  };

  // Returned by diff() and accepted by inc(); describes a version change.
  enum class version_change : std::uint8_t {
    none,
    major,
    minor,
    patch,
    premajor,    // major differs, newer version has pre-release tag
    preminor,    // minor differs, newer version has pre-release tag
    prepatch,    // patch differs, newer version has pre-release tag
    prerelease   // same major.minor.patch, only pre-release tag differs
  };

namespace detail {

constexpr from_chars_result success(const char* ptr) noexcept {
  return from_chars_result{ ptr, std::errc{} };
}

constexpr from_chars_result failure(const char* ptr, std::errc error_code = std::errc::invalid_argument) noexcept {
  return from_chars_result{ ptr, error_code };
}

constexpr to_chars_result success(char* ptr) noexcept {
  return to_chars_result{ ptr, std::errc{} };
}

constexpr to_chars_result failure(char* ptr, std::errc error_code = std::errc::value_too_large) noexcept {
  return to_chars_result{ ptr, error_code };
}

constexpr void trim_leading_spaces(std::string_view& str) noexcept {
  while (!str.empty() && str.front() == ' ')
    str.remove_prefix(1);
}

constexpr void strip_version_prefixes(std::string_view& str) noexcept {
  trim_leading_spaces(str);
  if (!str.empty() && str.front() == '=') {
    str.remove_prefix(1);
    trim_leading_spaces(str);
  }
  if (!str.empty() && (str.front() == 'v' || str.front() == 'V')) {
    str.remove_prefix(1);
    trim_leading_spaces(str);
  }
}

constexpr int compare_lexicographically(std::string_view lhs, std::string_view rhs) noexcept {
  const auto result = lhs.compare(rhs);
  return result < 0 ? -1 : result > 0 ? 1 : 0;
}

constexpr int compare_numerically(std::string_view lhs, std::string_view rhs) noexcept {
  // Parsing already rejected leading zeros.
  if (lhs.size() != rhs.size())
    return lhs.size() < rhs.size() ? -1 : 1;

  return compare_lexicographically(lhs, rhs);
}

enum class range_operator : std::uint8_t {
  less,
  less_or_equal,
  greater,
  greater_or_equal,
  equal,
  not_equal
};

class cursor {
public:
  explicit SEMVER_CONSTEXPR cursor(std::string_view text) noexcept : text_{text} {}

  [[nodiscard]] SEMVER_CONSTEXPR bool at_end() const noexcept { return current_ == text_.size(); }

  [[nodiscard]] SEMVER_CONSTEXPR bool has(std::size_t offset = 0) const noexcept {
    return offset < text_.size() - current_;
  }

  [[nodiscard]] SEMVER_CONSTEXPR char peek(std::size_t offset = 0) const noexcept {
    assert(has(offset) && "semver parser cursor read past end of input");
    return text_[current_ + offset];
  }

  [[nodiscard]] SEMVER_CONSTEXPR const char* ptr() const noexcept {
    return ptr_at(current_);
  }

  [[nodiscard]] SEMVER_CONSTEXPR const char* ptr_at(std::size_t position) const noexcept {
    assert(position <= text_.size() && "semver parser cursor position past end of input");
    return position == 0 ? text_.data() : text_.data() + position;
  }

  [[nodiscard]] SEMVER_CONSTEXPR std::size_t position() const noexcept { return current_; }

  SEMVER_CONSTEXPR char advance() noexcept {
    const auto c = peek();
    ++current_;
    return c;
  }

  SEMVER_CONSTEXPR bool consume(char c) noexcept {
    if (!has() || peek() != c)
      return false;

    ++current_;
    return true;
  }

  SEMVER_CONSTEXPR bool consume(std::string_view text) noexcept {
    if (text_.size() - current_ < text.size() || text_.substr(current_, text.size()) != text)
      return false;

    current_ += text.size();
    return true;
  }

private:
  std::string_view text_;
  std::size_t current_ = 0;
};

constexpr bool is_space(char c) noexcept {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

constexpr std::string_view next_identifier(std::string_view& tag) noexcept {
  const auto dot = tag.find('.');
  const auto id = tag.substr(0, dot);
  tag.remove_prefix(dot == std::string_view::npos ? tag.size() : dot + 1);
  return id;
}

SEMVER_CONSTEXPR int compare_prerelease_tags(std::string_view lhs, std::string_view rhs) noexcept {
  while (!lhs.empty() && !rhs.empty()) {
    const auto lhs_id = next_identifier(lhs);
    const auto rhs_id = next_identifier(rhs);

    const auto lhs_numeric = is_numeric_identifier(lhs_id);
    const auto rhs_numeric = is_numeric_identifier(rhs_id);

    int cmp = 0;
    if (lhs_numeric && rhs_numeric) {
      cmp = compare_numerically(lhs_id, rhs_id);
    } else if (!lhs_numeric && !rhs_numeric) {
      cmp = compare_lexicographically(lhs_id, rhs_id);
    } else {
      return lhs_numeric ? -1 : 1;
    }

    if (cmp != 0)
      return cmp;
  }

  if (lhs.empty() && rhs.empty())
    return 0;

  return lhs.empty() ? -1 : 1;
}

// Optional components used for range expansion; nullopt means omitted or '*'.
struct partial_version {
  std::optional<std::uint64_t> major, minor, patch;
  std::string prerelease; // empty if absent
  bool wildcard = false;

  SEMVER_CONSTEXPR bool is_partial() const noexcept {
    return !major.has_value() || !minor.has_value() || !patch.has_value();
  }
};

class version_parser {
public:
  explicit SEMVER_CONSTEXPR version_parser(cursor& input) noexcept : stream{input} {}

  template <typename I1, typename I2, typename I3>
  SEMVER_CONSTEXPR from_chars_result parse(version<I1, I2, I3>& out) {
    auto result = parse_number(out.major_);
    if (!result)
      return result;

    if (!stream.consume('.'))
      return failure(stream.ptr());

    result = parse_number(out.minor_);
    if (!result)
      return result;

    if (!stream.consume('.'))
      return failure(stream.ptr());

    result = parse_number(out.patch_);
    if (!result)
      return result;

    if (stream.consume('-')) {
      result = parse_prerelease_tag(out.prerelease_tag_);
      if (!result)
        return result;
    }

    if (stream.consume('+')) {
      result = parse_build_metadata(out.build_metadata_);
      if (!result)
        return result;
    }

    return result;
  }

  SEMVER_CONSTEXPR from_chars_result parse_partial(partial_version& out) {
    if (consume_wildcard()) {
      out.wildcard = true;
      return success(stream.ptr());
    }

    std::uint64_t n = 0;
    if (auto r = parse_number(n); !r)
      return r;

    out.major = n;

    if (!stream.consume('.'))
      return success(stream.ptr()); // bare M

    if (consume_wildcard()) {
      out.wildcard = true;
      return success(stream.ptr()); // M.<wildcard>
    }
    if (auto r = parse_number(n); !r)
      return r;

    out.minor = n;

    if (!stream.consume('.'))
      return success(stream.ptr()); // M.m

    if (consume_wildcard()) {
      out.wildcard = true;
      return success(stream.ptr()); // M.m.<wildcard>
    }
    if (auto r = parse_number(n); !r)
      return r;

    out.patch = n;

    if (stream.consume('-')) {
      if (const auto r = parse_prerelease_tag(out.prerelease); !r)
        return r;
    }

    return success(stream.ptr());
  }

private:
  cursor& stream;

  SEMVER_CONSTEXPR bool consume_wildcard() noexcept {
    return stream.consume('*') || stream.consume('x') || stream.consume('X');
  }

  template <typename Int>
  SEMVER_CONSTEXPR from_chars_result parse_number(Int& out) {
    if (!stream.has() || !is_digit(stream.peek()))
      return failure(stream.ptr());

    auto last_digit_pos = stream.position();
    const auto first_digit = static_cast<std::uint8_t>(stream.advance() - '0');
    std::uint64_t result = first_digit;

    if (first_digit == 0) {
      if (stream.has() && is_digit(stream.peek()))
        return failure(stream.ptr()); // leading zero in numeric version component

      out = static_cast<Int>(0);
      return success(stream.ptr());
    }

    while (stream.has() && is_digit(stream.peek())) {
      last_digit_pos = stream.position();
      const auto d = static_cast<std::uint8_t>(stream.advance() - '0');
      if (result > (std::numeric_limits<std::uint64_t>::max() - d) / 10)
        return failure(stream.ptr_at(last_digit_pos), std::errc::result_out_of_range);

      result = result * 10 + d;
    }

    if (detail::number_in_range<Int>(result)) {
      out = static_cast<Int>(result);
      return success(stream.ptr());
    }

    return failure(stream.ptr_at(last_digit_pos), std::errc::result_out_of_range);
  }

  SEMVER_CONSTEXPR from_chars_result parse_tag(std::string& out, bool check_leading_zeros) {
    do {
      if (!out.empty())
        out.push_back('.');
      const auto id_start = out.size();
      if (const auto res = parse_identifier(out, check_leading_zeros); !res) {
        if (id_start > 0)
          out.resize(id_start - 1); // roll back '.'
        return res;
      }
    } while (stream.consume('.'));
    return success(stream.ptr());
  }

  SEMVER_CONSTEXPR from_chars_result parse_prerelease_tag(std::string& out) { return parse_tag(out, true); }
  SEMVER_CONSTEXPR from_chars_result parse_build_metadata(std::string& out) { return parse_tag(out, false); }

  SEMVER_CONSTEXPR from_chars_result parse_identifier(std::string& out, bool check_leading_zeros) {
    const auto first = stream.position();
    while (stream.has() && is_identifier_char(stream.peek()))
      stream.advance();

    const auto id = std::string_view{stream.ptr_at(first), stream.position() - first};
    if (!validate_identifier(id, check_leading_zeros))
      return failure(stream.ptr_at(first));

    out.append(id.data(), id.size());
    return success(stream.ptr());
  }
};

template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
SEMVER_CONSTEXPR int compare_prerelease(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  const auto lhs_tag = lhs.prerelease_tag();
  const auto rhs_tag = rhs.prerelease_tag();
  if (lhs_tag.empty() != rhs_tag.empty())
    return lhs_tag.empty() ? 1 : -1;
  if (lhs_tag.empty())
    return 0;

  return compare_prerelease_tags(lhs_tag, rhs_tag);
}

template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
SEMVER_CONSTEXPR int compare_core(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  if (const auto major = detail::compare_numbers(lhs.major(), rhs.major()); major != 0)
    return major;
  if (const auto minor = detail::compare_numbers(lhs.minor(), rhs.minor()); minor != 0)
    return minor;

  return detail::compare_numbers(lhs.patch(), rhs.patch());
}

template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
SEMVER_CONSTEXPR int compare_parsed(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  const auto core = compare_core(lhs, rhs);
  return core != 0 ? core : compare_prerelease(lhs, rhs);
}

// Shared transactional full-input parse for versions and range sets.
template <typename Parser, typename Output>
[[nodiscard]] SEMVER_CONSTEXPR from_chars_result parse_full(std::string_view str, Output& out) {
  if (str.size() > SEMVER_MAX_INPUT_LENGTH)
    return failure(str.data(), std::errc::value_too_large);

  cursor input{str};
  Output tmp;
  const auto result = Parser{input}.parse(tmp);
  if (!result)
    return result;

  if (!input.at_end())
    return failure(input.ptr());

  out = std::move(tmp);
  return success(input.ptr());
}

} // namespace semver::detail

// SemVer precedence ignores build metadata.
template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR bool operator==(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs) == 0;
}

#if defined(__cpp_impl_three_way_comparison) && __cpp_impl_three_way_comparison >= 201907L
template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR std::weak_ordering operator<=>(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs) <=> 0;
}
#else
template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR bool operator!=(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs) != 0;
}

template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR bool operator>(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs) > 0;
}

template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR bool operator>=(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs) >= 0;
}

template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR bool operator<(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs) < 0;
}

template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR bool operator<=(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs) <= 0;
}
#endif

// Strict full-string parse; leaves output unchanged on failure.
template <typename I1, typename I2, typename I3>
[[nodiscard]] SEMVER_CONSTEXPR from_chars_result parse(std::string_view str, version<I1, I2, I3>& output) {
  return detail::parse_full<detail::version_parser>(str, output);
}

// Like std::from_chars, parses as far as possible without requiring full input consumption.
template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
[[nodiscard]] SEMVER_CONSTEXPR from_chars_result from_chars(const char* first, const char* last, version<I1, I2, I3>& v) {
  if (!first || !last || last < first)
    return detail::failure(first, std::errc::invalid_argument);

  const auto len = static_cast<std::size_t>(last - first);
  if (len > SEMVER_MAX_INPUT_LENGTH)
    return detail::failure(first, std::errc::value_too_large);

  detail::cursor input{std::string_view{first, len}};
  version<I1, I2, I3> tmp;
  const auto res = detail::version_parser{input}.parse(tmp);
  if (!res) {
    // Find the longest valid SemVer prefix.
    for (auto prefix_len = len; prefix_len > 0; --prefix_len) {
      const auto prefix_str = std::string_view{first, prefix_len};
      if (!detail::validate_version_syntax(prefix_str))
        continue;

      version<I1, I2, I3> prefix;
      const auto prefix_result = parse(prefix_str, prefix);
      if (prefix_result) {
        v = std::move(prefix);
        return detail::success(first + prefix_len);
      }

      return detail::failure(first + prefix_len, prefix_result.ec);
    }
    return detail::failure(first, std::errc::invalid_argument);
  }

  v = std::move(tmp);
  return res;
}

template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
[[nodiscard]] SEMVER_CONSTEXPR bool valid(std::string_view str) {
  version<I1, I2, I3> v{};
  return static_cast<bool>(parse(str, v));
}

// Zero-allocation serialization following std::to_chars conventions.
template <typename I1, typename I2, typename I3>
[[nodiscard]] constexpr to_chars_result to_chars(char* first, char* last, const version<I1, I2, I3>& v) noexcept {
  const auto pre = v.prerelease_tag();
  const auto meta = v.build_metadata();
  const auto needed = detail::serialized_length(v.major(), v.minor(), v.patch(), pre, meta);
  if (!first || !last || last < first)
    return detail::failure(last);

  const auto avail = static_cast<std::size_t>(last - first);
  if (avail < needed)
    return detail::failure(last);

  auto write_num = [](char* p, auto n) noexcept -> char* {
    const auto len = detail::length(n);
    detail::uint_write_backward(p + len, n);
    return p + len;
  };

  auto* p = first;
  p = write_num(p, v.major()); *p++ = '.';
  p = write_num(p, v.minor()); *p++ = '.';
  p = write_num(p, v.patch());
  if (!pre.empty()) {
    *p++ = '-';
    for (char c : pre) *p++ = c;
  }
  if (!meta.empty()) {
    *p++ = '+';
    for (char c : meta) *p++ = c;
  }
  return detail::success(p);
}

template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
[[nodiscard]] SEMVER_CONSTEXPR std::optional<version<I1, I2, I3>> try_parse(std::string_view str) {
  version<I1, I2, I3> v;
  if (parse(str, v))
    return v;

  return std::nullopt;
}

// Throws std::system_error on failure.
template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
[[nodiscard]] SEMVER_CONSTEXPR version<I1, I2, I3> from_string(std::string_view str) {
  version<I1, I2, I3> v;
  if (const auto res = parse(str, v); !res)
    throw std::system_error(std::make_error_code(res.ec), std::string{str});

  return v;
}

// Permissive parsing with optional =/v prefixes, missing components, and leading zeros.
template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
[[nodiscard]] SEMVER_CONSTEXPR std::optional<version<I1, I2, I3>> coerce(std::string_view str) {
  if (str.size() > SEMVER_MAX_INPUT_LENGTH)
    return std::nullopt;

  detail::strip_version_prefixes(str);
  if (str.empty() || !detail::is_digit(str.front()))
    return std::nullopt;

  const auto* p = str.data();
  const auto* end = str.data() + str.size();
  bool component_overflow = false;

  const auto parse_component = [&](std::uint64_t& out) noexcept -> bool {
    if (p >= end || !detail::is_digit(*p))
      return false;

    out = 0;
    while (p < end && detail::is_digit(*p)) {
      const auto d = static_cast<std::uint64_t>(static_cast<unsigned char>(*p) - '0');
      if (out > (std::numeric_limits<std::uint64_t>::max() - d) / 10) {
        component_overflow = true;
        return false;
      }

      out = out * 10 + d;
      ++p;
    }
    return true;
  };

  std::uint64_t maj = 0, min_v = 0, pat = 0;
  if (!parse_component(maj))
    return std::nullopt;

  if (p < end && *p == '.') {
    const auto* saved = p++;
    if (!parse_component(min_v)) {
      if (component_overflow)
        return std::nullopt;
      p = saved;
      min_v = 0;
    } else if (p < end && *p == '.') {
      const auto* saved2 = p++;
      if (!parse_component(pat)) {
        if (component_overflow)
          return std::nullopt;
        p = saved2;
        pat = 0;
      }
    }
  }

  if (!detail::version_components_in_range<I1, I2, I3>(maj, min_v, pat))
    return std::nullopt;

  version<I1, I2, I3> result{static_cast<I1>(maj), static_cast<I2>(min_v), static_cast<I3>(pat)};

  if (p < end && (*p == '-' || *p == '+')) {
    std::string_view prerelease, build;
    if (*p == '-') {
      const auto* first = ++p;
      while (p < end && *p != '+')
        ++p;
      prerelease = std::string_view{first, static_cast<std::size_t>(p - first)};
      if (prerelease.empty() || !detail::validate_prerelease_tag(prerelease))
        return result;
    }

    if (p < end && *p == '+') {
      const auto* first = ++p;
      build = std::string_view{first, static_cast<std::size_t>(end - first)};
      if (build.empty() || !detail::validate_build_metadata(build))
        return result;
    }

    return version<I1, I2, I3>{static_cast<I1>(maj), static_cast<I2>(min_v), static_cast<I3>(pat), prerelease, build};
  }

  return result;
}

// Writes "MAJOR.MINOR.PATCH[-prerelease][+build]" to a narrow stream.
template <typename Traits, typename I1, typename I2, typename I3>
std::basic_ostream<char, Traits>& operator<<(std::basic_ostream<char, Traits>& os, const version<I1, I2, I3>& v) {
  return os << v.to_string();
}

// SemVer precedence with a lexicographic build-metadata tie-breaker.
template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR int compare_with_build(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  const auto c = detail::compare_parsed(lhs, rhs);
  if (c != 0)
    return c;

  return detail::compare_lexicographically(lhs.build_metadata(), rhs.build_metadata());
}

// Returns which component differs between lhs and rhs.
template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR version_change diff(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  const auto cmp = detail::compare_parsed(lhs, rhs);
  if (cmp == 0)
    return version_change::none;

  const auto pre = cmp > 0 ? lhs.is_prerelease() : rhs.is_prerelease();

  if (lhs.major() != rhs.major())
    return pre ? version_change::premajor : version_change::major;

  if (lhs.minor() != rhs.minor())
    return pre ? version_change::preminor : version_change::minor;

  if (lhs.patch() != rhs.patch())
    return pre ? version_change::prepatch : version_change::patch;

  return version_change::prerelease;
}

// Returns -1, 0, or 1. Build metadata excluded.
template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR int compare(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs);
}

namespace detail {
  template <typename I1, typename I2, typename I3>
  struct range_comparator {
    SEMVER_CONSTEXPR range_comparator(const version<I1, I2, I3>& value, range_operator operation, bool include_from_zero = false) : bound(value), op(operation), include_prerelease_from_zero(include_from_zero) {}

    template <typename J1, typename J2, typename J3>
    SEMVER_CONSTEXPR bool contains(const version<J1, J2, J3>& other, prerelease_policy policy) const noexcept {
      auto comparison = detail::compare_parsed(other, bound);
      if (policy == prerelease_policy::include && include_prerelease_from_zero) {
        comparison = detail::compare_core(other, bound);
        if (comparison == 0)
          comparison = other.prerelease_tag().empty() ? 1 : detail::compare_prerelease_tags(other.prerelease_tag(), "0");
      }
      switch (op) {
      case range_operator::less:
        return comparison < 0;
      case range_operator::less_or_equal:
        return comparison <= 0;
      case range_operator::greater:
        return comparison > 0;
      case range_operator::greater_or_equal:
        return comparison >= 0;
      case range_operator::equal:
        return comparison == 0;
      case range_operator::not_equal:
        return comparison != 0;
      default:
        return false;
      }
    }

    version<I1, I2, I3> bound;
    range_operator op;
    bool include_prerelease_from_zero;
  };

  template <typename I1, typename I2, typename I3>
  SEMVER_CONSTEXPR bool push_comparator(std::uint64_t maj, std::uint64_t min_, std::uint64_t pat, std::string_view prerelease, range_operator op, std::vector<range_comparator<I1, I2, I3>>& out, bool include_prerelease_from_zero = false) {
    if (!version_components_in_range<I1, I2, I3>(maj, min_, pat))
      return false;

    out.emplace_back(version<I1, I2, I3>{static_cast<I1>(maj), static_cast<I2>(min_), static_cast<I3>(pat), prerelease}, op, include_prerelease_from_zero);
    return true;
  }

  template <typename I1, typename I2, typename I3>
  SEMVER_CONSTEXPR bool push_upper(std::uint64_t maj, std::uint64_t min_, std::uint64_t pat, std::vector<range_comparator<I1, I2, I3>>& out) {
    return push_comparator<I1, I2, I3>(maj, min_, pat, "0", range_operator::less, out);
  }

  enum class upper_component : std::uint8_t { major, minor, patch };

  // Increment the selected component without carrying between SemVer fields.
  template <typename I1, typename I2, typename I3>
  SEMVER_CONSTEXPR bool push_next_upper(std::uint64_t maj, std::uint64_t min_, std::uint64_t pat, upper_component component, std::vector<range_comparator<I1, I2, I3>>& out) {
    if (!version_components_in_range<I1, I2, I3>(maj, min_, pat))
      return false;

    if (component == upper_component::patch)
      return pat < std::numeric_limits<I3>::max() && push_upper<I1, I2, I3>(maj, min_, pat + 1, out);
    if (component == upper_component::minor)
      return min_ < std::numeric_limits<I2>::max() && push_upper<I1, I2, I3>(maj, min_ + 1, 0, out);

    return maj < std::numeric_limits<I1>::max() && push_upper<I1, I2, I3>(maj + 1, 0, 0, out);
  }

  template <typename I1, typename I2, typename I3>
  SEMVER_CONSTEXPR bool expand_to_next_line(const partial_version& pv, std::vector<range_comparator<I1, I2, I3>>& out, bool include_prerelease_from_zero) {
    if (!pv.major.has_value())
      return true;

    const auto maj = pv.major.value_or(0);
    const auto min_ = pv.minor.value_or(0);
    const auto pat = pv.patch.value_or(0);
    if (!push_comparator<I1, I2, I3>(maj, min_, pat, pv.prerelease, range_operator::greater_or_equal, out, include_prerelease_from_zero))
      return false;

    const auto component = pv.minor.has_value() ? upper_component::minor : upper_component::major;
    return push_next_upper<I1, I2, I3>(maj, min_, pat, component, out);
  }

  // Lock the leftmost non-zero component.
  template <typename I1, typename I2, typename I3>
  SEMVER_CONSTEXPR bool expand_caret(const partial_version& pv, std::vector<range_comparator<I1, I2, I3>>& out) {
    const auto maj = pv.major.value_or(0);
    const auto min_ = pv.minor.value_or(0);
    const auto pat = pv.patch.value_or(0);
    const auto include_prerelease_from_zero = pv.prerelease.empty() && (pv.is_partial() || maj == 0);
    if (!push_comparator<I1, I2, I3>(maj, min_, pat, pv.prerelease, range_operator::greater_or_equal, out, include_prerelease_from_zero))
      return false;

    if (maj > 0 || !pv.minor.has_value())
      return push_next_upper<I1, I2, I3>(maj, min_, pat, upper_component::major, out);
    if (min_ > 0 || !pv.patch.has_value())
      return push_next_upper<I1, I2, I3>(maj, min_, pat, upper_component::minor, out);

    return push_next_upper<I1, I2, I3>(maj, min_, pat, upper_component::patch, out);
  }

  template <typename I1, typename I2, typename I3>
  SEMVER_CONSTEXPR bool expand_comparator(const partial_version& pv, range_operator op, std::vector<range_comparator<I1, I2, I3>>& out) {
    const auto maj = *pv.major;
    const auto min_ = pv.minor.value_or(0);
    const auto pat = pv.patch.value_or(0);
    const auto prerelease = pv.is_partial() && op == range_operator::less ? std::string_view{"0"} : std::string_view{pv.prerelease};
    const auto include_prerelease_from_zero = pv.is_partial() && op == range_operator::greater_or_equal;
    return push_comparator<I1, I2, I3>(maj, min_, pat, prerelease, op, out, include_prerelease_from_zero);
  }

  class range_parser;
  struct range_set_access;

  template <typename I1, typename I2, typename I3>
  class range {
  public:
    friend class detail::range_parser;

    template <typename J1, typename J2, typename J3>
    SEMVER_CONSTEXPR bool contains(const version<J1, J2, J3>& v, prerelease_policy policy) const noexcept {
      if (policy == prerelease_policy::exclude && !allows_prerelease(v))
        return false;

      for (const auto& rc : ranges_comparators) {
        if (!rc.contains(v, policy))
          return false;
      }
      return true;
    }

    SEMVER_CONSTEXPR const std::vector<range_comparator<I1, I2, I3>>& comparators() const noexcept {
      return ranges_comparators;
    }

  private:
    std::vector<range_comparator<I1, I2, I3>> ranges_comparators;

    template <typename J1, typename J2, typename J3>
    SEMVER_CONSTEXPR bool allows_prerelease(const version<J1, J2, J3>& v) const noexcept {
      if (v.prerelease_tag().empty())
        return true;

      for (const auto& rc : ranges_comparators) {
        if (rc.op != range_operator::not_equal && rc.bound.is_prerelease() && detail::compare_core(v, rc.bound) == 0)
          return true;
      }
      return false;
    }
  };
}

template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
#if defined(__cpp_concepts) && __cpp_concepts >= 201907L
  requires detail::are_component_types_v<I1, I2, I3>
#endif
class range_set {
  static_assert(detail::is_component_type_v<I1>, "semver: I1 must be an unsigned integral type");
  static_assert(detail::is_component_type_v<I2>, "semver: I2 must be an unsigned integral type");
  static_assert(detail::is_component_type_v<I3>, "semver: I3 must be an unsigned integral type");

public:
  friend class detail::range_parser;
  friend struct detail::range_set_access;

  // A prerelease matches by default only when its comparator targets the same M.m.p.
  template <typename J1, typename J2, typename J3>
  [[nodiscard]] SEMVER_CONSTEXPR bool contains(const version<J1, J2, J3>& v, prerelease_policy policy = prerelease_policy::exclude) const noexcept {
    for (const auto& range : ranges) {
      if (range.contains(v, policy))
        return true;
    }
    return false;
  }

private:
  std::vector<detail::range<I1, I2, I3>> ranges;
};

namespace detail {
  struct range_set_access {
    template <typename I1, typename I2, typename I3>
    static SEMVER_CONSTEXPR const std::vector<detail::range<I1, I2, I3>>& ranges(const range_set<I1, I2, I3>& value) noexcept {
      return value.ranges;
    }
  };

  class range_parser {
  public:
    explicit SEMVER_CONSTEXPR range_parser(cursor& input) noexcept : stream(input) {}

    template <typename I1, typename I2, typename I3>
    SEMVER_CONSTEXPR from_chars_result parse(range_set<I1, I2, I3>& out) {
      skip_whitespaces();
      if (stream.at_end())
        return failure(stream.ptr());

      do {
        detail::range<I1, I2, I3> rng;
        if (const auto res = parse_range(rng); !res)
          return res;

        out.ranges.push_back(std::move(rng));
        skip_whitespaces();

      } while (stream.consume("||"));

      return success(stream.ptr());
    }

  private:
    cursor& stream;

    enum class term_kind : std::uint8_t { bare, comparator, tilde, caret };

    SEMVER_CONSTEXPR bool can_start_term() const noexcept {
      if (!stream.has())
        return false;

      const auto c = stream.peek();
      return c == '<' || c == '>' || c == '=' || c == '!' || c == '~' || c == '^' || c == '*' || c == 'x' || c == 'X' || is_digit(c);
    }

    SEMVER_CONSTEXPR bool consume_operator(range_operator& op) noexcept {
      if (!stream.has())
        return false;

      switch (stream.peek()) {
      case '<':
        stream.advance();
        op = stream.consume('=') ? range_operator::less_or_equal : range_operator::less;
        return true;
      case '>':
        stream.advance();
        op = stream.consume('=') ? range_operator::greater_or_equal : range_operator::greater;
        return true;
      case '=':
        stream.advance();
        op = range_operator::equal;
        return true;
      case '!':
        if (!stream.consume("!="))
          return false;
        op = range_operator::not_equal;
        return true;
      default:
        return false;
      }
    }

    template <typename I1, typename I2, typename I3>
    SEMVER_CONSTEXPR from_chars_result parse_range(detail::range<I1, I2, I3>& out) {
      do {
        skip_whitespaces();
        const auto* first = stream.ptr();
        auto kind = term_kind::bare;
        auto op = range_operator::equal;

        if (stream.consume('~'))
          kind = term_kind::tilde;
        else if (stream.consume('^'))
          kind = term_kind::caret;
        else if (consume_operator(op))
          kind = term_kind::comparator;

        if (kind != term_kind::bare)
          skip_whitespaces();

        const auto* version_start = stream.ptr();
        detail::partial_version pv;
        version_parser parser{stream};
        if (const auto result = parser.parse_partial(pv); !result)
          return result;

        if (kind != term_kind::bare && (!pv.major.has_value() || pv.wildcard))
          return failure(version_start);

        bool expanded = false;
        switch (kind) {
        case term_kind::bare:
          expanded = pv.is_partial()
            ? detail::expand_to_next_line<I1, I2, I3>(pv, out.ranges_comparators, true)
            : detail::expand_comparator<I1, I2, I3>(pv, range_operator::equal, out.ranges_comparators);
          break;
        case term_kind::tilde:
          expanded = detail::expand_to_next_line<I1, I2, I3>(pv, out.ranges_comparators, false);
          break;
        case term_kind::caret:
          expanded = detail::expand_caret<I1, I2, I3>(pv, out.ranges_comparators);
          break;
        case term_kind::comparator:
          if (pv.is_partial() && op != range_operator::greater_or_equal && op != range_operator::less)
            return failure(version_start);
          expanded = detail::expand_comparator<I1, I2, I3>(pv, op, out.ranges_comparators);
          break;
        }

        if (!expanded)
          return failure(first, std::errc::result_out_of_range);

        const auto has_separator = skip_whitespaces();
        if (can_start_term() && !has_separator)
          return failure(stream.ptr());
      } while (can_start_term());

      return success(stream.ptr());
    }

    SEMVER_CONSTEXPR bool skip_whitespaces() noexcept {
      bool skipped = false;
      while (stream.has() && is_space(stream.peek())) {
        stream.advance();
        skipped = true;
      }
      return skipped;
    }
  };

  template <typename O1, typename O2, typename O3, typename I1, typename I2, typename I3>
  SEMVER_CONSTEXPR version<O1, O2, O3> convert_range_version(const version<I1, I2, I3>& value) {
    return version<O1, O2, O3>{static_cast<O1>(value.major()), static_cast<O2>(value.minor()), static_cast<O3>(value.patch()), value.prerelease_tag()};
  }

  template <typename I1, typename I2, typename I3>
  SEMVER_CONSTEXPR std::optional<version<I1, I2, I3>> next_core(const version<I1, I2, I3>& value) {
    if (value.patch() < std::numeric_limits<I3>::max())
      return value.bump_patch();
    if (value.minor() < std::numeric_limits<I2>::max())
      return value.bump_minor();
    if (value.major() < std::numeric_limits<I1>::max())
      return value.bump_major();

    return std::nullopt;
  }

  template <typename O1, typename O2, typename O3, typename I1, typename I2, typename I3, typename Visitor>
  SEMVER_CONSTEXPR bool visit_range_candidates(const range<I1, I2, I3>& range, Visitor&& visitor) {
    // Intersections change only at bounds, successors, or core edges.
    for (const auto& comparator : range.comparators()) {
      const auto bound = convert_range_version<O1, O2, O3>(comparator.bound);
      if (visitor(bound))
        return true;
      if (bound.prerelease_tag() != "0" && visitor(version<O1, O2, O3>{bound.major(), bound.minor(), bound.patch(), "0"}))
        return true;

      if (bound.is_prerelease()) {
        if (visitor(version<O1, O2, O3>{bound.major(), bound.minor(), bound.patch()}))
          return true;

        auto prerelease = std::string{bound.prerelease_tag()};
        prerelease += ".0";
        if (visitor(version<O1, O2, O3>{bound.major(), bound.minor(), bound.patch(), prerelease}))
          return true;
      }

      if (const auto next = next_core(bound)) {
        if (visitor(*next) || visitor(version<O1, O2, O3>{next->major(), next->minor(), next->patch(), "0"}))
          return true;
      }
    }
    return false;
  }

  template <typename I1, typename I2, typename I3>
  SEMVER_CONSTEXPR bool has_upper_bound(const range<I1, I2, I3>& range) noexcept {
    for (const auto& comparator : range.comparators()) {
      const auto op = comparator.op;
      if (op == range_operator::less || op == range_operator::less_or_equal || op == range_operator::equal)
        return true;
    }
    return false;
  }
} // namespace semver::detail

template <typename I1, typename I2, typename I3>
[[nodiscard]] SEMVER_CONSTEXPR from_chars_result parse(std::string_view str, range_set<I1, I2, I3>& out) {
  return detail::parse_full<detail::range_parser>(str, out);
}

template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
[[nodiscard]] SEMVER_CONSTEXPR std::optional<range_set<I1, I2, I3>> try_parse_range(std::string_view str) {
  range_set<I1, I2, I3> rs;
  if (parse(str, rs))
    return rs;

  return std::nullopt;
}

// Parses range_str and checks containment. Pre-parse the range for hot paths.
template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
[[nodiscard]] SEMVER_CONSTEXPR bool satisfies(const version<I1, I2, I3>& v, std::string_view range_str, prerelease_policy policy = prerelease_policy::exclude) {
  range_set<I1, I2, I3> rs;
  if (!parse(range_str, rs))
    return false;

  return rs.contains(v, policy);
}

// Returns the lowest representable version satisfying rs, or nullopt if none exists.
template <typename I1, typename I2, typename I3>
[[nodiscard]] SEMVER_CONSTEXPR std::optional<version<I1, I2, I3>> min_version(const range_set<I1, I2, I3>& rs, prerelease_policy policy = prerelease_policy::exclude) {
  const auto& ranges = detail::range_set_access::ranges(rs);
  if (ranges.empty())
    return std::nullopt;

  std::optional<version<I1, I2, I3>> result;
  const auto consider = [&](const version<I1, I2, I3>& candidate) {
    if (rs.contains(candidate, policy) && (!result || candidate < *result))
      result = candidate;
    return false;
  };

  consider(version<I1, I2, I3>{I1{}, I2{}, I3{}});
  consider(version<I1, I2, I3>{I1{}, I2{}, I3{}, "0"});
  for (const auto& range : ranges)
    detail::visit_range_candidates<I1, I2, I3>(range, consider);

  return result;
}

// Returns true if a version can satisfy both range sets.
template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR bool intersects(const range_set<L1, L2, L3>& lhs, const range_set<R1, R2, R3>& rhs, prerelease_policy policy = prerelease_policy::exclude) {
  using I1 = std::conditional_t<(std::numeric_limits<L1>::digits >= std::numeric_limits<R1>::digits), L1, R1>;
  using I2 = std::conditional_t<(std::numeric_limits<L2>::digits >= std::numeric_limits<R2>::digits), L2, R2>;
  using I3 = std::conditional_t<(std::numeric_limits<L3>::digits >= std::numeric_limits<R3>::digits), L3, R3>;

  for (const auto& lhs_range : detail::range_set_access::ranges(lhs)) {
    for (const auto& rhs_range : detail::range_set_access::ranges(rhs)) {
      // Unbounded ranges share a sufficiently large release.
      if (!detail::has_upper_bound(lhs_range) && !detail::has_upper_bound(rhs_range))
        return true;

      const auto matches = [&](const version<I1, I2, I3>& candidate) {
        return lhs_range.contains(candidate, policy) && rhs_range.contains(candidate, policy);
      };

      if (matches(version<I1, I2, I3>{I1{}, I2{}, I3{}}) || matches(version<I1, I2, I3>{I1{}, I2{}, I3{}, "0"}) ||
          detail::visit_range_candidates<I1, I2, I3>(lhs_range, matches) || detail::visit_range_candidates<I1, I2, I3>(rhs_range, matches))
        return true;
    }
  }
  return false;
}

// Returns nullopt for an unsupported change, overflow, or an invalid prerelease when used.
template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
[[nodiscard]] SEMVER_CONSTEXPR std::optional<version<I1, I2, I3>> inc(const version<I1, I2, I3>& v, version_change change, std::string_view pre = {}) {
  if (!pre.empty() && (change == version_change::major || change == version_change::minor || change == version_change::patch))
    return std::nullopt;

  version<I1, I2, I3> base;
  switch (change) {
    case version_change::major:
    case version_change::premajor:
      if (v.major() == std::numeric_limits<I1>::max())
        return std::nullopt;

      base = v.bump_major();
      if (change == version_change::major)
        return base;
      break;
    case version_change::minor:
    case version_change::preminor:
      if (v.minor() == std::numeric_limits<I2>::max())
        return std::nullopt;

      base = v.bump_minor();
      if (change == version_change::minor)
        return base;
      break;
    case version_change::patch:
    case version_change::prepatch:
      if (v.patch() == std::numeric_limits<I3>::max())
        return std::nullopt;

      base = v.bump_patch();
      if (change == version_change::patch)
        return base;
      break;
    case version_change::prerelease:
      if (v.prerelease_tag().empty()) {
        if (v.patch() == std::numeric_limits<I3>::max())
          return std::nullopt;

        base = v.bump_patch();
      } else {
        base = version<I1, I2, I3>{v.major(), v.minor(), v.patch()};
      }
      break;
    case version_change::none:
    default: return std::nullopt;
  }

  if (!pre.empty()) {
    if (!detail::validate_prerelease_tag(pre))
      return std::nullopt;
    return version<I1, I2, I3>{base.major(), base.minor(), base.patch(), pre};
  }

  if (change != version_change::prerelease || v.prerelease_tag().empty())
    return version<I1, I2, I3>{base.major(), base.minor(), base.patch(), "0"};

  auto tag = std::string{v.prerelease_tag()};
  const auto dot_pos = tag.rfind('.');
  const auto last_start = (dot_pos == std::string::npos) ? 0 : dot_pos + 1;
  const auto last_id = std::string_view{tag}.substr(last_start);
  if (detail::is_numeric_identifier(last_id)) {
    auto digit_pos = tag.size();
    while (digit_pos > last_start && tag[digit_pos - 1] == '9') {
      tag[digit_pos - 1] = '0';
      --digit_pos;
    }
    if (digit_pos == last_start) {
      tag.insert(last_start, 1, '1');
    } else {
      ++tag[digit_pos - 1];
    }
  } else {
    tag += ".0";
  }

  return version<I1, I2, I3>{base.major(), base.minor(), base.patch(), std::string_view{tag}};
}

// Returns iterator to the highest version in [first, last) satisfying rs, or last if none.
template <typename ForwardIt, typename I1, typename I2, typename I3>
[[nodiscard]] SEMVER_CONSTEXPR ForwardIt max_satisfying(ForwardIt first, ForwardIt last, const range_set<I1, I2, I3>& rs, prerelease_policy policy = prerelease_policy::exclude) {
  auto result = last;
  for (auto it = first; it != last; ++it) {
    if (rs.contains(*it, policy) && (result == last || *result < *it))
      result = it;
  }
  return result;
}

// Returns iterator to the lowest version in [first, last) satisfying rs, or last if none.
template <typename ForwardIt, typename I1, typename I2, typename I3>
[[nodiscard]] SEMVER_CONSTEXPR ForwardIt min_satisfying(ForwardIt first, ForwardIt last, const range_set<I1, I2, I3>& rs, prerelease_policy policy = prerelease_policy::exclude) {
  auto result = last;
  for (auto it = first; it != last; ++it) {
    if (rs.contains(*it, policy) && (result == last || *it < *result))
      result = it;
  }
  return result;
}

// Trims wrappers and spaces, then parses strictly.
template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
[[nodiscard]] SEMVER_CONSTEXPR std::optional<version<I1, I2, I3>> clean(std::string_view str) {
  if (str.size() > SEMVER_MAX_INPUT_LENGTH)
    return std::nullopt;

  detail::strip_version_prefixes(str);
  while (!str.empty() && str.back() == ' ')
    str.remove_suffix(1);
  return try_parse<I1, I2, I3>(str);
}

inline const version<> library_version{SEMVER_VERSION_MAJOR, SEMVER_VERSION_MINOR, SEMVER_VERSION_PATCH};

#if SEMVER_HAS_CONSTEVAL_LITERAL
namespace literals {
  consteval version<> operator""_semver(const char* str, std::size_t len) {
    version<> v;
    const auto result = parse(std::string_view{str, len}, v);
    if (!result) {
      throw "invalid semver literal";
    }
    return v;
  }
} // namespace literals
#endif

} // namespace semver

// Keep only the public feature flags.
#undef SEMVER_CONSTEXPR

namespace std {
  template <typename I1, typename I2, typename I3>
  struct hash<semver::version<I1, I2, I3>> {
    std::size_t operator()(const semver::version<I1, I2, I3>& v) const noexcept {
      // Build metadata is excluded by SemVer §10.
#if SIZE_MAX > UINT32_MAX
      static constexpr auto hash_combine_constant = std::size_t{0x9e3779b97f4a7c15ULL};
#else
      static constexpr auto hash_combine_constant = std::size_t{0x9e3779b9U};
#endif
      static constexpr auto hash_combine = [](std::size_t seed, std::size_t value) noexcept -> std::size_t {
        return seed ^ (value + hash_combine_constant + (seed << 6) + (seed >> 2));
      };
      auto h = std::hash<I1>{}(v.major());
      h = hash_combine(h, std::hash<I2>{}(v.minor()));
      h = hash_combine(h, std::hash<I3>{}(v.patch()));
      h = hash_combine(h, std::hash<std::string_view>{}(v.prerelease_tag()));
      return h;
    }
  };
} // namespace std

#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
namespace std {
  template <typename I1, typename I2, typename I3>
  struct formatter<semver::version<I1, I2, I3>> : formatter<string_view> {
    template <typename FormatContext>
    auto format(const semver::version<I1, I2, I3>& v, FormatContext& ctx) const {
      const auto s = v.to_string();
      return formatter<string_view>::format(s, ctx);
    }
  };
} // namespace std
#endif

#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic pop
#endif

#endif // NEARGYE_SEMANTIC_VERSIONING_HPP
