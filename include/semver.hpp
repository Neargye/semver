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

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <charconv>
#include <utility>
#include <vector>

#ifndef SEMVER_MAX_INPUT_LENGTH
#define SEMVER_MAX_INPUT_LENGTH 512
#endif

#if defined(SEMVER_CONFIG_FILE)
#include SEMVER_CONFIG_FILE
#endif

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wmissing-braces" // "suggest braces around subobject initializer" false positives
#endif

#if __cpp_impl_three_way_comparison >= 201907L
#include <compare>
#endif

#if __cpp_concepts >= 201907L
#include <concepts>
#endif

// Clang + libstdc++ < 13: broken constexpr std::string (construct_at SSO bug).
#if __cpp_lib_constexpr_string >= 201907L && __cpp_lib_constexpr_vector >= 201907L
  #if defined(__clang__) && defined(__GLIBCXX__) && (!defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE < 13)
    #define SEMVER_HAS_CONSTEXPR 0
    #define SEMVER_CONSTEXPR inline
  #else
    #define SEMVER_HAS_CONSTEXPR 1
    #define SEMVER_CONSTEXPR constexpr
  #endif
#else
  #define SEMVER_HAS_CONSTEXPR 0
  #define SEMVER_CONSTEXPR inline
#endif

// MSVC 19.3x: consteval + constexpr std::string triggers ICE.
// GCC + libstdc++ (non-Clang): consteval throw propagation broken before GCC 14.
#if __cpp_consteval >= 201811L && SEMVER_HAS_CONSTEXPR && !defined(_MSC_VER) && !(defined(__GLIBCXX__) && !defined(__clang__))
  #define SEMVER_HAS_CONSTEVAL_LITERAL 1
#else
  #define SEMVER_HAS_CONSTEVAL_LITERAL 0
#endif

// Some system headers (e.g. glibc <sys/sysmacros.h>, OpenBSD <sys/types.h>)
// define `major` and `minor` as macros for device-number extraction.
#ifdef major
#  undef major
#endif
#ifdef minor
#  undef minor
#endif

namespace semver {

  // Max input length for all parse functions. Configurable via SEMVER_MAX_INPUT_LENGTH.
  inline constexpr std::size_t max_input_length = SEMVER_MAX_INPUT_LENGTH;

  // Compile-time library version integers; usable in static_assert and templates.
  inline constexpr std::uint32_t library_version_major = SEMVER_VERSION_MAJOR;
  inline constexpr std::uint32_t library_version_minor = SEMVER_VERSION_MINOR;
  inline constexpr std::uint32_t library_version_patch = SEMVER_VERSION_PATCH;

  namespace detail {

    template <typename Int>
    constexpr std::size_t length(Int n) noexcept {
      auto un = n;
      std::size_t digits = 0;
      do { ++digits; un /= 10; } while (un != 0);
      return digits;
    }

    template <typename OutputIt, typename Int>
    constexpr OutputIt uint_write_backward(OutputIt dest, Int n) noexcept {
      auto un = n;
      do {
        *(--dest) = static_cast<char>('0' + (un % 10));
        un /= 10;
      } while (un != 0);
      return dest;
    }

    // Returns true when `tag` is a valid §9 pre-release identifier:
    //   - non-empty dot-separated identifiers, chars [0-9A-Za-z-]
    //   - no leading zeros in purely-numeric identifiers
    constexpr bool validate_prerelease_tag(std::string_view tag) noexcept {
      if (tag.empty()) return true;
      std::size_t start = 0;
      for (;;) {
        const auto dot = tag.find('.', start);
        const auto len = (dot == std::string_view::npos) ? tag.size() - start : dot - start;
        if (len == 0) return false; // empty identifier (leading/trailing/double dot)
        const std::string_view id = tag.substr(start, len);
        bool numeric = true;
        for (char c : id) {
          const bool is_d = (c >= '0' && c <= '9');
          const bool is_l = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
          const bool is_h = (c == '-');
          if (!is_d && !is_l && !is_h) return false; // invalid char
          if (!is_d) numeric = false;
        }
        // Leading zero in a purely-numeric identifier is forbidden (semver §9).
        if (numeric && len > 1 && id[0] == '0') return false;
        if (dot == std::string_view::npos) break;
        start = dot + 1;
        if (start > tag.size()) return false; // trailing dot
      }
      return true;
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
    constexpr bool cmp_less_equal(T t, U u) noexcept {
      return !cmp_less(u, t);
    }

    template<class T, class U>
    constexpr bool cmp_greater_equal(T t, U u) noexcept {
      return !cmp_less(t, u);
    }

    template<typename R, typename T>
    constexpr bool number_in_range(T t) noexcept {
      return cmp_greater_equal(t, std::numeric_limits<R>::min()) && cmp_less_equal(t, std::numeric_limits<R>::max());
    }

    class version_parser;
  }

  template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
#if __cpp_concepts >= 201907L
    requires std::unsigned_integral<I1> && std::unsigned_integral<I2> && std::unsigned_integral<I3>
#endif
  class version {
    static_assert(std::is_unsigned_v<I1>, "semver: I1 must be an unsigned integral type");
    static_assert(std::is_unsigned_v<I2>, "semver: I2 must be an unsigned integral type");
    static_assert(std::is_unsigned_v<I3>, "semver: I3 must be an unsigned integral type");

    friend class detail::version_parser;

  public:
    version() = default; // default is 0.1.0, see semver.org FAQ §4
    version(const version&) = default;
    version(version&&) = default;
    ~version() = default;

    SEMVER_CONSTEXPR version(I1 major, I2 minor, I3 patch) noexcept : major_(major), minor_(minor), patch_(patch) {}

    // Precondition: prerelease must be a valid semver §9 identifier (e.g. "alpha.1", "0")
    // or empty. Passing an invalid string is a precondition violation (asserted in debug
    // builds); use try_parse()/from_string() for untrusted input.
    SEMVER_CONSTEXPR version(I1 major, I2 minor, I3 patch, std::string_view prerelease) : major_(major), minor_(minor), patch_(patch), prerelease_tag_(prerelease) {
      assert(detail::validate_prerelease_tag(prerelease) && "semver: invalid prerelease tag (bad chars, empty id, or leading zero in numeric id)");
    }

    version& operator=(const version&) = default;
    version& operator=(version&&) = default;

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

    // Returns (major+1).0.0; clears pre-release and build.
    [[nodiscard]] SEMVER_CONSTEXPR version<I1, I2, I3> bump_major() const noexcept {
      assert(major_ < std::numeric_limits<I1>::max() && "semver: bump_major overflow");
      return version<I1, I2, I3>{static_cast<I1>(major_ + I1{1}), I2{}, I3{}};
    }
    // Returns major.(minor+1).0; clears pre-release and build.
    [[nodiscard]] SEMVER_CONSTEXPR version<I1, I2, I3> bump_minor() const noexcept {
      assert(minor_ < std::numeric_limits<I2>::max() && "semver: bump_minor overflow");
      return version<I1, I2, I3>{major_, static_cast<I2>(minor_ + I2{1}), I3{}};
    }
    // Returns major.minor.(patch+1); clears pre-release and build.
    [[nodiscard]] SEMVER_CONSTEXPR version<I1, I2, I3> bump_patch() const noexcept {
      assert(patch_ < std::numeric_limits<I3>::max() && "semver: bump_patch overflow");
      return version<I1, I2, I3>{major_, minor_, static_cast<I3>(patch_ + I3{1})};
    }

    // Pre-release identifier (e.g. "alpha.1"). Empty if absent.
    [[nodiscard]] SEMVER_CONSTEXPR std::string_view prerelease_tag() const noexcept { return prerelease_tag_; }
    // Build metadata (e.g. "build.42"). Empty if absent. Excluded from comparisons and hash (spec §10).
    [[nodiscard]] SEMVER_CONSTEXPR std::string_view build_metadata() const noexcept { return build_metadata_; }

    [[nodiscard]] SEMVER_CONSTEXPR bool is_prerelease()      const noexcept { return !prerelease_tag_.empty(); }
    [[nodiscard]] SEMVER_CONSTEXPR bool has_build_metadata() const noexcept { return !build_metadata_.empty(); }

    // Serializes to "MAJOR.MINOR.PATCH[-prerelease][+build]".
    [[nodiscard]] SEMVER_CONSTEXPR std::string to_string() const {
      std::string result(length(), '\0');
      (void)to_chars(result.data(), result.data() + result.size(), *this);
      return result;
    }

  private:
    I1 major_ = 0;
    I2 minor_ = 1;
    I3 patch_ = 0;
    std::string prerelease_tag_;
    std::string build_metadata_;

    SEMVER_CONSTEXPR std::size_t length() const noexcept {
      return detail::length(major_) + detail::length(minor_) + detail::length(patch_) + 2
        + (prerelease_tag_.empty() ? 0 : prerelease_tag_.length() + 1)
        + (build_metadata_.empty() ? 0 : build_metadata_.length() + 1);
    }

    SEMVER_CONSTEXPR void reset() noexcept {
      major_ = 0;
      minor_ = 1;
      patch_ = 0;

      prerelease_tag_.clear();
      build_metadata_.clear();
    }
  };

  // Follows std::from_chars/std::to_chars conventions.
  // ptr: one past last char on success; first invalid char on failure.
  // ec:  std::errc{} on success; invalid_argument, result_out_of_range, or value_too_large on failure.
  struct from_chars_result {
    const char* ptr;
    std::errc ec;

    [[nodiscard]] explicit constexpr operator bool() const noexcept { return ec == std::errc{}; }
  };

  // Alias for from_chars_result; used as the return type of to_chars.
  using to_chars_result = from_chars_result;

  // Controls pre-release handling in range matching and ordering.
  enum class version_compare_option : std::uint8_t {
    exclude_prerelease, // pre-release only matches if a comparator explicitly targets the same M.m.p pre-release
    include_prerelease  // include pre-release versions in matching and ordering
  };

  // Short-name aliases for version_compare_option.
  inline constexpr auto include_prerelease = version_compare_option::include_prerelease;
  inline constexpr auto exclude_prerelease = version_compare_option::exclude_prerelease;

  // Returned by diff() — describes which component differs between two versions.
  enum class version_diff : std::uint8_t {
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

constexpr bool is_digit(char c) noexcept {
  return c >= '0' && c <= '9';
}

constexpr bool is_letter(char c) noexcept {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

constexpr std::uint8_t to_digit(char c) noexcept {
  return static_cast<std::uint8_t>(c - '0');
}

constexpr char to_char(std::uint8_t i) noexcept {
  return static_cast<char>('0' + i);
}

constexpr int compare_numerically(std::string_view lhs, std::string_view rhs) noexcept {
  // assume that strings don't have leading zeros (we've already checked it at parsing stage).
  if (lhs.size() != rhs.size()) {
    return lhs.size() < rhs.size() ? -1 : 1;
  }

  for (std::size_t i = 0; i < lhs.size(); ++i) {
    int a = lhs[i] - '0';
    int b = rhs[i] - '0';
    if (a != b) {
      return a < b ? -1 : 1;
    }
  }
  return 0;
}

enum class token_type : std::uint8_t {
  eol,
  space,
  dot,
  plus,
  hyphen,
  letter,
  digit,
  range_operator,
  logical_or,
  tilde,   // '~'
  caret,   // '^'
  wildcard // '*'  (x/X are lexed as letter but treated as wildcards in range context)
};

enum class range_operator : std::uint8_t {
  less,
  less_or_equal,
  greater,
  greater_or_equal,
  equal
};

struct token {
  token_type type{};
  union {
    std::uint8_t digit;    // token_type::digit
    char         letter;   // token_type::letter
    range_operator op;     // token_type::range_operator
  } value{};
  const char* lexeme = nullptr;
};

class token_stream {
public:
  SEMVER_CONSTEXPR explicit token_stream(std::size_t size_hint = 32) {
    tokens.reserve(size_hint);
  }

  SEMVER_CONSTEXPR void push(const token& token) {
    tokens.push_back(token);
  }

  SEMVER_CONSTEXPR token advance() noexcept {
    const token t = get(current);
    ++current;
    return t;
  }

  SEMVER_CONSTEXPR token peek(std::size_t k = 0) const noexcept {
    return get(current + k);
  }

  SEMVER_CONSTEXPR token previous() const noexcept {
    assert(current > 0 && "token_stream::previous() called before any advance()");
    return get(current - 1);
  }

  SEMVER_CONSTEXPR bool advance_if_match(token& token, token_type type) noexcept {
    if (get(current).type != type) {
      return false;
    }

    token = advance();
    return true;
  }

  SEMVER_CONSTEXPR bool advance_if_match(token_type type) noexcept {
    if (get(current).type != type) return false;
    ++current;
    return true;
  }

  SEMVER_CONSTEXPR bool check(token_type type) const noexcept {
    return peek().type == type;
  }

private:
  std::size_t current = 0;
  std::vector<token> tokens;

  SEMVER_CONSTEXPR token get(std::size_t i) const noexcept {
    assert(!tokens.empty() && "token_stream used before scan_tokens()");
    // tokens.back() is always token_type::eol after scan_tokens(); used as safe OOB sentinel.
    return i < tokens.size() ? tokens[i] : tokens.back();
  }
};

class lexer {
public:
  explicit SEMVER_CONSTEXPR lexer(std::string_view text) noexcept : text_{text}, current_pos_{0} {}

  SEMVER_CONSTEXPR from_chars_result scan_tokens(token_stream& token_stream) {
    from_chars_result result{ text_.data(), std::errc{} };

    while (!is_eol()) {
      result = scan_token(token_stream);
      if (!result) {
        // Always push eol sentinel to preserve OOB invariant, even on failure.
        token_stream.push({ token_type::eol, {}, result.ptr });
        return result;
      }
    }

    token_stream.push({ token_type::eol, {}, text_.data() + text_.size() });

    return result;
  }

private:
  std::string_view text_;
  std::size_t current_pos_;

  SEMVER_CONSTEXPR from_chars_result scan_token(token_stream& stream) {
    const char c = advance();

    switch (c) {
    case ' ':
      add_token(stream, token_type::space);
      break;
    case '.':
      add_token(stream, token_type::dot);
      break;
    case '-':
      add_token(stream, token_type::hyphen);
      break;
    case '+':
      add_token(stream, token_type::plus);
      break;
    case '|':
      if (advance_if_match('|')) {
        add_token(stream, token_type::logical_or);
        break;
      }
      return failure(get_prev_symbol());
    case '<':
      add_range_operator_token(stream, advance_if_match('=') ? range_operator::less_or_equal : range_operator::less);
      break;
    case '>':
      add_range_operator_token(stream, advance_if_match('=') ? range_operator::greater_or_equal : range_operator::greater);
      break;
    case '=':
      add_range_operator_token(stream, range_operator::equal);
      break;
    case '~':
      add_token(stream, token_type::tilde);
      break;
    case '^':
      add_token(stream, token_type::caret);
      break;
    case '*':
      add_token(stream, token_type::wildcard);
      break;
    default:
      if (is_digit(c)) {
        add_digit_token(stream, to_digit(c));
        break;
      }
      else if (is_letter(c)) {
        add_letter_token(stream, c);
        break;
      }
      return failure(get_prev_symbol());
    }

    return success(get_prev_symbol());
  }

  SEMVER_CONSTEXPR void add_token(token_stream& stream, token_type type) {
    stream.push({ type, {}, get_prev_symbol() });
  }

  SEMVER_CONSTEXPR void add_digit_token(token_stream& stream, std::uint8_t digit) {
    token t{};
    t.type        = token_type::digit;
    t.value.digit = digit;
    t.lexeme      = get_prev_symbol();
    stream.push(t);
  }

  SEMVER_CONSTEXPR void add_letter_token(token_stream& stream, char letter) {
    token t{};
    t.type         = token_type::letter;
    t.value.letter = letter;
    t.lexeme       = get_prev_symbol();
    stream.push(t);
  }

  SEMVER_CONSTEXPR void add_range_operator_token(token_stream& stream, range_operator op) {
    token t{};
    t.type     = token_type::range_operator;
    t.value.op = op;
    t.lexeme   = get_prev_symbol();
    stream.push(t);
  }

  SEMVER_CONSTEXPR char advance() noexcept {
    assert(!is_eol() && "lexer::advance() called past end of input");
    return text_[current_pos_++];
  }

  SEMVER_CONSTEXPR bool advance_if_match(char c) noexcept {
    if (is_eol() || text_[current_pos_] != c) return false;
    ++current_pos_;
    return true;
  }

  SEMVER_CONSTEXPR const char* get_prev_symbol() const noexcept {
    return text_.data() + current_pos_ - 1;
  }

  SEMVER_CONSTEXPR bool is_eol() const noexcept { return current_pos_ >= text_.size(); }
};

constexpr bool is_numeric_identifier(std::string_view id) noexcept {
  for (char c : id) {
    if (!is_digit(c)) return false;
  }
  return !id.empty();
}

constexpr std::string_view next_identifier(std::string_view& tag) noexcept {
  const auto dot = tag.find('.');
  std::string_view id;
  if (dot == std::string_view::npos) {
    id = tag;
    tag = std::string_view{};
  } else {
    id = tag.substr(0, dot);
    tag = tag.substr(dot + 1);
  }
  return id;
}

SEMVER_CONSTEXPR int compare_prerelease_tags(std::string_view lhs, std::string_view rhs) noexcept {
  while (!lhs.empty() && !rhs.empty()) {
    const auto lhs_id = next_identifier(lhs);
    const auto rhs_id = next_identifier(rhs);

    const bool lhs_numeric = is_numeric_identifier(lhs_id);
    const bool rhs_numeric = is_numeric_identifier(rhs_id);

    int cmp = 0;
    if (lhs_numeric && rhs_numeric) {
      cmp = compare_numerically(lhs_id, rhs_id);
    } else if (!lhs_numeric && !rhs_numeric) {
      cmp = lhs_id.compare(rhs_id);
    } else {
      return lhs_numeric ? -1 : 1;
    }

    if (cmp != 0) return cmp;
  }

  if (lhs.empty() && rhs.empty()) return 0;
  return lhs.empty() ? -1 : 1;
}

// Returns true for tokens that represent a wildcard version component: *, x, X.
constexpr bool is_wildcard_token(const token& t) noexcept {
  return t.type == token_type::wildcard ||
         (t.type == token_type::letter && (t.value.letter == 'x' || t.value.letter == 'X'));
}

// A version with optional/wildcard components, used for range desugaring (~, ^, x-ranges, hyphen).
// A nullopt component means "wildcard" — desugar functions fill it with 0 for lower bounds.
struct partial_version {
  std::optional<std::uint64_t> major, minor, patch;
  std::string prerelease; // empty if absent

  SEMVER_CONSTEXPR bool has_wildcard() const noexcept {
    return !major.has_value() || !minor.has_value() || !patch.has_value();
  }
};

class version_parser {
public:
  SEMVER_CONSTEXPR explicit version_parser(token_stream& stream) : stream{stream} {}

  template <typename I1, typename I2, typename I3>
  SEMVER_CONSTEXPR from_chars_result parse(version<I1, I2, I3>& out) {
    out.reset();

    from_chars_result result = parse_number(out.major_);
    if (!result) {
      return result;
    }

    if (!stream.advance_if_match(token_type::dot)) {
      return failure(stream.peek().lexeme);
    }

    result = parse_number(out.minor_);
    if (!result) {
      return result;
    }

    if (!stream.advance_if_match(token_type::dot)) {
      return failure(stream.peek().lexeme);
    }

    result = parse_number(out.patch_);
    if (!result) {
      return result;
    }

    if (stream.advance_if_match(token_type::hyphen)) {
      result = parse_prerelease_tag(out.prerelease_tag_);
      if (!result) {
        return result;
      }
    }

    if (stream.advance_if_match(token_type::plus)) {
      result = parse_build_metadata(out.build_metadata_);
      if (!result) {
        return result;
      }
    }

    return result;
  }

  // Parses M[.m[.p]][-prerelease][+build] where any of M/m/p may be a wildcard (*, x, X).
  // Used by range desugaring. Sets out.major/minor/patch to nullopt for wildcard components.
  SEMVER_CONSTEXPR from_chars_result parse_partial(partial_version& out) {
    out = {};

    // Full wildcard (* / x / X) — all components unconstrained.
    if (is_wildcard_token(stream.peek())) {
      stream.advance();
      return success(stream.peek().lexeme);
    }

    if (!stream.check(token_type::digit))
      return failure(stream.peek().lexeme);

    std::uint64_t n = 0;
    if (auto r = parse_number(n); !r) return r;
    out.major = n;

    if (!stream.advance_if_match(token_type::dot))
      return success(stream.peek().lexeme); // bare M

    if (is_wildcard_token(stream.peek())) {
      stream.advance();
      return success(stream.peek().lexeme); // M.*  M.x
    }
    if (!stream.check(token_type::digit))
      return success(stream.peek().lexeme);
    if (auto r = parse_number(n); !r) return r;
    out.minor = n;

    if (!stream.advance_if_match(token_type::dot))
      return success(stream.peek().lexeme); // M.m

    if (is_wildcard_token(stream.peek())) {
      stream.advance();
      return success(stream.peek().lexeme); // M.m.*  M.m.x
    }
    if (!stream.check(token_type::digit))
      return success(stream.peek().lexeme);
    if (auto r = parse_number(n); !r) return r;
    out.patch = n;

    // Optional prerelease (validated by parse_tag<true>).
    if (stream.advance_if_match(token_type::hyphen))
      parse_prerelease_tag(out.prerelease); // best-effort in range context

    // Build metadata is irrelevant for range matching — skip without allocation.
    if (stream.advance_if_match(token_type::plus)) {
      skip_tag();
    }

    return success(stream.peek().lexeme);
  }

private:
  token_stream& stream;

  template <typename Int>
  SEMVER_CONSTEXPR from_chars_result parse_number(Int& out) {
    token token = stream.advance();

    if (!is_digit(token)) {
      return failure(token.lexeme);
    }

    const auto first_digit = token.value.digit;
    std::uint64_t result = first_digit;

    if (first_digit == 0) {
      if (stream.check(token_type::digit)) {
        return failure(stream.peek().lexeme); // leading zero in numeric version component
      }
      out = static_cast<Int>(0);
      return success(stream.peek().lexeme);
    }

    while (stream.advance_if_match(token, token_type::digit)) {
      const auto d = token.value.digit;
      if (result > (std::numeric_limits<std::uint64_t>::max() - d) / 10) {
        return failure(token.lexeme, std::errc::result_out_of_range);
      }
      result = result * 10 + d;
    }

    if (detail::number_in_range<Int>(result)) {
      out = static_cast<Int>(result);
      return success(stream.peek().lexeme);
    }

    return failure(token.lexeme, std::errc::result_out_of_range);
  }

  // Dot-separated tag parser used by both parse_prerelease_tag and parse_build_metadata.
  // check_leading_zeros=true enforces spec §9 (no leading zeros in numeric identifiers).
  SEMVER_CONSTEXPR from_chars_result parse_tag(std::string& out, bool check_leading_zeros) {
    out.clear();
    do {
      if (!out.empty()) out.push_back('.');
      const auto id_start = out.size();
      if (const auto res = parse_identifier(out, check_leading_zeros); !res) {
        if (id_start > 0) out.resize(id_start - 1); // roll back '.'
        return res;
      }
    } while (stream.advance_if_match(token_type::dot));
    return success(stream.peek().lexeme);
  }

  SEMVER_CONSTEXPR from_chars_result parse_prerelease_tag(std::string& out) { return parse_tag(out, true); }
  SEMVER_CONSTEXPR from_chars_result parse_build_metadata(std::string& out) { return parse_tag(out, false); }

  // Skips a dot-separated tag (prerelease or build metadata) without building a string.
  // Used in parse_partial to discard build metadata with zero allocation.
  SEMVER_CONSTEXPR void skip_tag() noexcept {
    do {
      if (!is_alphanumeric(stream.peek())) break;
      while (is_alphanumeric(stream.peek())) stream.advance();
    } while (stream.advance_if_match(token_type::dot));
  }

  // Unified dot-separated identifier parser.
  // check_leading_zeros=true: prerelease identifiers obey spec §9 (no leading zeros in numeric IDs).
  // check_leading_zeros=false: build-metadata identifiers have no such restriction.
  SEMVER_CONSTEXPR from_chars_result parse_identifier(std::string& out, bool check_leading_zeros) {
    const auto start = out.size(); // append mode: track where this identifier begins
    // Peek before consuming: avoids advancing the stream on a non-starter token.
    if (!is_alphanumeric(stream.peek())) {
      return failure(stream.peek().lexeme);
    }
    token t = stream.advance();

    do {
      switch (t.type) {
      case token_type::hyphen:
        out.push_back('-');
        break;
      case token_type::letter:
        out.push_back(t.value.letter);
        break;
      case token_type::digit: {
        const auto digit = t.value.digit;
        if (check_leading_zeros) {
          // Purely numeric identifiers must not have leading zeros (spec §9).
          // "1.2.3-01.alpha" → invalid; "1.2.3-01b" → valid (alphanumeric).
          if (out.size() == start && is_leading_zero(digit)) {
            out.resize(start);
            return failure(t.lexeme);
          }
        }
        out.push_back(to_char(digit));
        break;
      }
      default:
        // All token_type values are handled above; this branch is unreachable.
        out.resize(start);
        return failure(t.lexeme);
      }
    } while (stream.advance_if_match(t, token_type::hyphen)
          || stream.advance_if_match(t, token_type::letter)
          || stream.advance_if_match(t, token_type::digit));

    return success(stream.peek().lexeme);
  }

  SEMVER_CONSTEXPR bool is_leading_zero(int digit) noexcept {
    if (digit != 0) return false;
    // Scan ahead: letter/hyphen after '0' → alphanumeric identifier → OK.
    // All digits → purely-numeric with leading zero → reject.
    for (std::size_t k = 0; ; ++k) {
      const token t = stream.peek(k);
      if (!is_alphanumeric(t)) break;        // end of identifier (eol sentinel guarantees exit)
      if (!is_digit(t))        return false;  // letter or hyphen → alphanumeric → OK
    }
    return is_digit(stream.peek(0)); // true iff '0' is followed by ≥1 digit
  }

  SEMVER_CONSTEXPR bool is_digit(const token& token) const noexcept {
    return token.type == token_type::digit;
  }

  SEMVER_CONSTEXPR bool is_alphanumeric(const token& token) const noexcept {
    return token.type == token_type::hyphen || token.type == token_type::letter || token.type == token_type::digit;
  }
};

template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
SEMVER_CONSTEXPR int compare_prerelease(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  const auto lhs_tag = lhs.prerelease_tag();
  const auto rhs_tag = rhs.prerelease_tag();
  if (lhs_tag.empty() != rhs_tag.empty()) {
    return lhs_tag.empty() ? 1 : -1;
  }
  if (lhs_tag.empty()) {
    return 0;
  }
  return compare_prerelease_tags(lhs_tag, rhs_tag);
}

template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
SEMVER_CONSTEXPR int compare_parsed(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs, version_compare_option compare_option) noexcept {
  if (detail::cmp_less(lhs.major(), rhs.major())) return -1;
  if (detail::cmp_less(rhs.major(), lhs.major())) return  1;

  if (detail::cmp_less(lhs.minor(), rhs.minor())) return -1;
  if (detail::cmp_less(rhs.minor(), lhs.minor())) return  1;

  if (detail::cmp_less(lhs.patch(), rhs.patch())) return -1;
  if (detail::cmp_less(rhs.patch(), lhs.patch())) return  1;

  if (compare_option == version_compare_option::include_prerelease) {
    return detail::compare_prerelease(lhs, rhs);
  }

  return 0;
}

template <typename I1, typename I2, typename I3>
[[nodiscard]] SEMVER_CONSTEXPR from_chars_result parse_version(std::string_view str, version<I1, I2, I3>& out) {
  if (str.size() > SEMVER_MAX_INPUT_LENGTH) {
    return failure(str.data(), std::errc::value_too_large);
  }

  token_stream ts{str.size()};
  from_chars_result result = lexer{ str }.scan_tokens(ts);
  if (!result) {
    return result;
  }

  version<I1, I2, I3> tmp;
  result = version_parser{ ts }.parse(tmp);
  if (!result) {
    return result;
  }

  if (!ts.advance_if_match(token_type::eol)) {
    return failure(ts.peek().lexeme);
  }

  out = std::move(tmp);
  return success(ts.previous().lexeme);
}

} // namespace semver::detail

// C++17: ==, !=, <, <=, >, >=. C++20: == and <=> (strong_ordering). Build metadata excluded (spec §10).
template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR bool operator==(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs, version_compare_option::include_prerelease) == 0;
}

#if __cpp_impl_three_way_comparison >= 201907L
template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR std::strong_ordering operator<=>(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  const int cmp = detail::compare_parsed(lhs, rhs, version_compare_option::include_prerelease);
  if (cmp == 0) return std::strong_ordering::equal;
  if (cmp > 0)  return std::strong_ordering::greater;
  return std::strong_ordering::less;
}
#else
template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR bool operator!=(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs, version_compare_option::include_prerelease) != 0;
}

template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR bool operator>(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs, version_compare_option::include_prerelease) > 0;
}

template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR bool operator>=(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs, version_compare_option::include_prerelease) >= 0;
}

template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR bool operator<(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs, version_compare_option::include_prerelease) < 0;
}

template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR bool operator<=(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs, version_compare_option::include_prerelease) <= 0;
}
#endif

// Full-string parse — fails if any trailing characters remain.
// On failure, output is left unchanged.
template <typename I1, typename I2, typename I3>
[[nodiscard]] SEMVER_CONSTEXPR from_chars_result parse(std::string_view str, version<I1, I2, I3>& output) {
  return detail::parse_version(str, output);
}

// Like std::from_chars — parses as far as possible, does not require full input consumption.
template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
[[nodiscard]] SEMVER_CONSTEXPR from_chars_result from_chars(const char* first, const char* last, version<I1, I2, I3>& v) {
  if (!first || !last || last < first)
    return detail::failure(first, std::errc::invalid_argument);

  const auto len = static_cast<std::size_t>(last - first);
  if (len > SEMVER_MAX_INPUT_LENGTH)
    return detail::failure(first, std::errc::value_too_large);

  detail::token_stream ts{len};
  // Ignore lexer errors: unrecognized chars act as stop markers.
  // scan_tokens() always pushes an eol sentinel, so the parser stops cleanly.
  detail::lexer{std::string_view{first, len}}.scan_tokens(ts);
  return detail::version_parser{ts}.parse(v);
}

// Returns true if str is a valid semver string.
template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
[[nodiscard]] SEMVER_CONSTEXPR bool valid(std::string_view str) {
  version<I1, I2, I3> v{};
  return static_cast<bool>(detail::parse_version(str, v));
}

// Zero-allocation serialization — follows std::to_chars semantics:
//   on success: ptr points one past the last written character
//   on failure (buffer too small): ptr == last, ec == std::errc::value_too_large
template <typename I1, typename I2, typename I3>
[[nodiscard]] constexpr to_chars_result to_chars(char* first, char* last, const version<I1, I2, I3>& v) noexcept {
  const std::size_t needed =
      detail::length(v.major()) + detail::length(v.minor()) + detail::length(v.patch()) + 2
      + (v.prerelease_tag().empty() ? std::size_t{0} : v.prerelease_tag().size() + 1)
      + (v.build_metadata().empty() ? std::size_t{0} : v.build_metadata().size() + 1);
  if (!first || !last || last < first)
    return detail::failure(last, std::errc::value_too_large);
  const auto avail = static_cast<std::size_t>(last - first);
  if (avail < needed)
    return detail::failure(last, std::errc::value_too_large);

  // Fills decimal digits forward using backward-fill helper.
  auto write_num = [](char* p, auto n) noexcept -> char* {
    const auto len = detail::length(n);
    detail::uint_write_backward(p + len, n); // fills p[0..len) backward from p+len
    return p + len;
  };

  char* p = first;
  p = write_num(p, v.major()); *p++ = '.';
  p = write_num(p, v.minor()); *p++ = '.';
  p = write_num(p, v.patch());
  if (!v.prerelease_tag().empty()) {
    *p++ = '-';
    for (char c : std::string_view{v.prerelease_tag()}) *p++ = c;
  }
  if (!v.build_metadata().empty()) {
    *p++ = '+';
    for (char c : std::string_view{v.build_metadata()}) *p++ = c;
  }
  return detail::success(p);
}

// Returns nullopt on failure.
template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
[[nodiscard]] SEMVER_CONSTEXPR std::optional<version<I1, I2, I3>> try_parse(std::string_view str) {
  version<I1, I2, I3> v;
  if (parse(str, v)) return v;
  return std::nullopt;
}

// Throws std::system_error on failure.
template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
[[nodiscard]] version<I1, I2, I3> from_string(std::string_view str) {
  version<I1, I2, I3> v;
  if (const auto res = parse(str, v); !res)
    throw std::system_error(std::make_error_code(res.ec), std::string{str});
  return v;
}

// Permissive parse: strips leading =, v/V, fills missing minor/patch with 0, strips leading zeros.
// "v1.2" → 1.2.0,  "=1.2.3" → 1.2.3,  "01.2.3" → 1.2.3,  "1.2.3.4" → 1.2.3,  "1.2.3garbage" → 1.2.3.
// Returns nullopt if no valid numeric prefix or a component overflows the target integer type.
// Not constexpr: uses std::to_string and string concatenation (C++17 limitations).
template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
[[nodiscard]] std::optional<version<I1, I2, I3>> coerce(std::string_view str) {
  while (!str.empty() && str.front() == ' ') str.remove_prefix(1);
  if (!str.empty() && str.front() == '=') {
    str.remove_prefix(1);
    while (!str.empty() && str.front() == ' ') str.remove_prefix(1);
  }
  if (!str.empty() && (str.front() == 'v' || str.front() == 'V')) {
    str.remove_prefix(1);
    while (!str.empty() && str.front() == ' ') str.remove_prefix(1);
  }
  if (str.empty() || !detail::is_digit(str.front())) return std::nullopt;

  const char* p   = str.data();
  const char* end = str.data() + str.size();

  // Parse a run of decimal digits at *p into out (uint64_t); advance p.
  // Naturally strips leading zeros. Returns false on uint64_t overflow.
  const auto parse_component = [&](std::uint64_t& out) noexcept -> bool {
    if (p >= end || !detail::is_digit(*p)) return false;
    out = 0;
    while (p < end && detail::is_digit(*p)) {
      const std::uint64_t d = static_cast<std::uint64_t>(static_cast<unsigned char>(*p) - '0');
      if (out > (std::numeric_limits<std::uint64_t>::max() - d) / 10) return false;
      out = out * 10 + d;
      ++p;
    }
    return true;
  };

  std::uint64_t maj = 0, min_v = 0, pat = 0;
  if (!parse_component(maj)) return std::nullopt;

  if (p < end && *p == '.') {
    const char* saved = p++;
    if (!parse_component(min_v)) { p = saved; min_v = 0; }
    else if (p < end && *p == '.') {
      const char* saved2 = p++;
      if (!parse_component(pat)) { p = saved2; pat = 0; }
    }
  }

  if (!detail::number_in_range<I1>(maj) ||
      !detail::number_in_range<I2>(min_v) ||
      !detail::number_in_range<I3>(pat)) return std::nullopt;

  version<I1, I2, I3> result{static_cast<I1>(maj), static_cast<I2>(min_v), static_cast<I3>(pat)};

  // Attach optional prerelease/build suffix: rebuild "M.m.p" + suffix and re-parse.
  if (p < end && (*p == '-' || *p == '+')) {
    std::string canonical;
    canonical.reserve(32 + static_cast<std::size_t>(end - p));
    canonical += std::to_string(maj);
    canonical += '.';
    canonical += std::to_string(min_v);
    canonical += '.';
    canonical += std::to_string(pat);
    canonical.append(p, static_cast<std::size_t>(end - p));
    if (auto r = try_parse<I1, I2, I3>(canonical)) return r;
    // Invalid suffix (e.g. leading-zero numeric identifier) — fall back to M.m.p.
  }

  return result;
}

// Writes "MAJOR.MINOR.PATCH[-prerelease][+build]" to os.
template <typename OStream, typename I1, typename I2, typename I3, typename = std::enable_if_t<std::is_base_of<std::ios_base, OStream>::value>>
OStream& operator<<(OStream& os, const version<I1, I2, I3>& v) {
  os << v.major() << '.' << v.minor() << '.' << v.patch();
  if (!v.prerelease_tag().empty()) os << '-' << v.prerelease_tag();
  if (!v.build_metadata().empty()) os << '+' << v.build_metadata();
  return os;
}

// Compares M.m.p, pre-release (§11), then build metadata (lexicographic).
// Non-standard: spec §10 says build metadata SHOULD be ignored in comparisons.
template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR int compare_with_build(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  const int c = detail::compare_parsed(lhs, rhs, version_compare_option::include_prerelease);
  if (c != 0) return c;
  const int b = lhs.build_metadata().compare(rhs.build_metadata());
  return b < 0 ? -1 : b > 0 ? 1 : 0;
}

// Returns which component differs between lhs and rhs.
template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR version_diff diff(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  if (lhs == rhs) return version_diff::none;

  const int cmp = detail::compare_parsed(lhs, rhs, version_compare_option::include_prerelease);
  // pre = true if the higher-precedence version has a pre-release tag.
  // Returns `prerelease` when only the tag differs (e.g. 1.0.0-alpha vs 1.0.0).
  const auto& newer = (cmp > 0) ? lhs : rhs;
  const bool  pre   = !newer.prerelease_tag().empty();

  const bool major_differs = detail::cmp_less(lhs.major(), rhs.major()) || detail::cmp_less(rhs.major(), lhs.major());
  if (major_differs) return pre ? version_diff::premajor : version_diff::major;

  const bool minor_differs = detail::cmp_less(lhs.minor(), rhs.minor()) || detail::cmp_less(rhs.minor(), lhs.minor());
  if (minor_differs) return pre ? version_diff::preminor : version_diff::minor;

  const bool patch_differs = detail::cmp_less(lhs.patch(), rhs.patch()) || detail::cmp_less(rhs.patch(), lhs.patch());
  if (patch_differs) return pre ? version_diff::prepatch : version_diff::patch;

  return version_diff::prerelease;
}

// Returns -1, 0, or 1. Build metadata excluded.
template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR int compare(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs, version_compare_option::include_prerelease);
}

// Reverse compare — returns 1, 0, or -1. Sorts descending when passed to std::sort.
template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR int rcompare(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  return compare(rhs, lhs);
}

namespace detail {
  template <typename I1, typename I2, typename I3>
  class range_comparator {
  public:
    SEMVER_CONSTEXPR range_comparator(const version<I1, I2, I3>& v, range_operator op) : v(v), op(op) {}

    SEMVER_CONSTEXPR bool contains(const version<I1, I2, I3>& other) const noexcept {
      switch (op) {
      case range_operator::less:
        return detail::compare_parsed(other, v, version_compare_option::include_prerelease) < 0;
      case range_operator::less_or_equal:
        return detail::compare_parsed(other, v, version_compare_option::include_prerelease) <= 0;
      case range_operator::greater:
        return detail::compare_parsed(other, v, version_compare_option::include_prerelease) > 0;
      case range_operator::greater_or_equal:
        return detail::compare_parsed(other, v, version_compare_option::include_prerelease) >= 0;
      case range_operator::equal:
        return detail::compare_parsed(other, v, version_compare_option::include_prerelease) == 0;
      default:
        return false; // unreachable: all range_operator values are handled above
      }
    }

    SEMVER_CONSTEXPR const version<I1, I2, I3>& get_version() const noexcept { return v; }

  private:
    version<I1, I2, I3> v;
    range_operator op;
  };

  // -----------------------------------------------------------------------
  // Range desugaring helpers
  // -----------------------------------------------------------------------

  // Exclusive upper bound for range desugaring: version M.m.p-0. Returns nullopt on overflow.
  template <typename I1, typename I2, typename I3>
  SEMVER_CONSTEXPR std::optional<version<I1, I2, I3>>
  make_upper_bound(std::uint64_t maj, std::uint64_t min_, std::uint64_t pat) {
    if (!number_in_range<I1>(maj) || !number_in_range<I2>(min_) || !number_in_range<I3>(pat))
      return std::nullopt;
    return version<I1, I2, I3>{
        static_cast<I1>(maj), static_cast<I2>(min_), static_cast<I3>(pat),
        std::string_view{"0"}};
  }

  // Pushes a >= comparator for the given partial version.
  template <typename I1, typename I2, typename I3>
  SEMVER_CONSTEXPR bool push_lower(
      const partial_version& pv,
      std::uint64_t maj, std::uint64_t min_, std::uint64_t pat,
      std::vector<range_comparator<I1, I2, I3>>& out) {
    if (!number_in_range<I1>(maj) || !number_in_range<I2>(min_) || !number_in_range<I3>(pat))
      return false;
    if (!pv.prerelease.empty()) {
      out.emplace_back(
          version<I1, I2, I3>{static_cast<I1>(maj), static_cast<I2>(min_), static_cast<I3>(pat),
                               std::string_view{pv.prerelease}},
          range_operator::greater_or_equal);
    } else {
      out.emplace_back(
          version<I1, I2, I3>{static_cast<I1>(maj), static_cast<I2>(min_), static_cast<I3>(pat)},
          range_operator::greater_or_equal);
    }
    return true;
  }

  // X-range: *  →  >=0.0.0
  //          M.* →  >=M.0.0 <(M+1).0.0-0
  //          M.m.* → >=M.m.0 <M.(m+1).0-0
  template <typename I1, typename I2, typename I3>
  SEMVER_CONSTEXPR bool desugar_xrange(
      const partial_version& pv,
      std::vector<range_comparator<I1, I2, I3>>& out) {
    const std::uint64_t maj  = pv.major.value_or(0);
    const std::uint64_t min_ = pv.minor.value_or(0);
    const std::uint64_t pat  = pv.patch.value_or(0);
    if (!push_lower(pv, maj, min_, pat, out)) return false;
    if (!pv.major.has_value()) return true; // * → >=0.0.0, no upper bound
    if (!pv.minor.has_value()) {
      if (maj == std::numeric_limits<std::uint64_t>::max()) return false;
      auto ub = make_upper_bound<I1, I2, I3>(maj + 1, 0, 0);
      if (!ub) return false;
      out.emplace_back(*ub, range_operator::less);
      return true;
    }
    // M.m.* or bare M.m — lock minor
    if (min_ == std::numeric_limits<std::uint64_t>::max()) return false;
    auto ub = make_upper_bound<I1, I2, I3>(maj, min_ + 1, 0);
    if (!ub) return false;
    out.emplace_back(*ub, range_operator::less);
    return true;
  }

  // Tilde: ~M.m[.p] → >=M.m.p  <M.(m+1).0-0
  //        ~M       → >=M.0.0   <(M+1).0.0-0
  template <typename I1, typename I2, typename I3>
  SEMVER_CONSTEXPR bool desugar_tilde(
      const partial_version& pv,
      std::vector<range_comparator<I1, I2, I3>>& out) {
    const std::uint64_t maj  = pv.major.value_or(0);
    const std::uint64_t min_ = pv.minor.value_or(0);
    const std::uint64_t pat  = pv.patch.value_or(0);
    if (!push_lower(pv, maj, min_, pat, out)) return false;
    if (!pv.major.has_value()) return true; // ~* — no upper bound
    if (pv.minor.has_value()) {
      // ~M.m[.p] — lock minor
      if (min_ == std::numeric_limits<std::uint64_t>::max()) return false;
      auto ub = make_upper_bound<I1, I2, I3>(maj, min_ + 1, 0);
      if (!ub) return false;
      out.emplace_back(*ub, range_operator::less);
    } else {
      // ~M — lock major
      if (maj == std::numeric_limits<std::uint64_t>::max()) return false;
      auto ub = make_upper_bound<I1, I2, I3>(maj + 1, 0, 0);
      if (!ub) return false;
      out.emplace_back(*ub, range_operator::less);
    }
    return true;
  }

  // Caret: locks the leftmost non-zero component (or follows the node-semver
  // rules for zero-prefixed and wildcard versions).
  //   ^M.m.p (M>0)            → >=M.m.p  <(M+1).0.0-0
  //   ^0.m.p (m>0)            → >=0.m.p  <0.(m+1).0-0
  //   ^0.0.p                  → >=0.0.p  <0.0.(p+1)-0
  //   ^M     / ^M.m (missing) → same as M>0 rule
  //   ^0.m   (missing patch)  → >=0.m.0  <0.(m+1).0-0
  //   ^0.0   (missing patch)  → >=0.0.0  <0.1.0-0
  template <typename I1, typename I2, typename I3>
  SEMVER_CONSTEXPR bool desugar_caret(
      const partial_version& pv,
      std::vector<range_comparator<I1, I2, I3>>& out) {
    const std::uint64_t maj  = pv.major.value_or(0);
    const std::uint64_t min_ = pv.minor.value_or(0);
    const std::uint64_t pat  = pv.patch.value_or(0);
    if (!push_lower(pv, maj, min_, pat, out)) return false;
    if (maj > 0 || !pv.minor.has_value()) {
      // Lock major: <(M+1).0.0-0
      if (maj == std::numeric_limits<std::uint64_t>::max()) return false;
      auto ub = make_upper_bound<I1, I2, I3>(maj + 1, 0, 0);
      if (!ub) return false;
      out.emplace_back(*ub, range_operator::less);
    } else if (!pv.patch.has_value() || min_ > 0) {
      // Lock minor: <0.(m+1).0-0
      if (min_ == std::numeric_limits<std::uint64_t>::max()) return false;
      auto ub = make_upper_bound<I1, I2, I3>(0, min_ + 1, 0);
      if (!ub) return false;
      out.emplace_back(*ub, range_operator::less);
    } else {
      // M=0, m=0, patch explicitly specified: lock patch <0.0.(p+1)-0
      if (pat == std::numeric_limits<std::uint64_t>::max()) return false;
      auto ub = make_upper_bound<I1, I2, I3>(0, 0, pat + 1);
      if (!ub) return false;
      out.emplace_back(*ub, range_operator::less);
    }
    return true;
  }

  // Hyphen range: lo - hi
  //   lower: >=lo (missing components filled with 0)
  //   upper: <=hi if hi.patch specified; <hi.M.(hi.m+1).0-0 if only M.m; <(hi.M+1).0.0-0 if only M
  template <typename I1, typename I2, typename I3>
  SEMVER_CONSTEXPR bool desugar_hyphen(
      const partial_version& lo, const partial_version& hi,
      std::vector<range_comparator<I1, I2, I3>>& out) {
    const std::uint64_t lo_maj = lo.major.value_or(0);
    const std::uint64_t lo_min = lo.minor.value_or(0);
    const std::uint64_t lo_pat = lo.patch.value_or(0);
    if (!push_lower(lo, lo_maj, lo_min, lo_pat, out)) return false;

    const std::uint64_t hi_maj = hi.major.value_or(0);
    const std::uint64_t hi_min = hi.minor.value_or(0);
    const std::uint64_t hi_pat = hi.patch.value_or(0);
    if (!number_in_range<I1>(hi_maj) || !number_in_range<I2>(hi_min) || !number_in_range<I3>(hi_pat))
      return false;

    if (hi.patch.has_value()) {
      // Full upper: <=hi
      if (!hi.prerelease.empty()) {
        out.emplace_back(
            version<I1, I2, I3>{static_cast<I1>(hi_maj), static_cast<I2>(hi_min), static_cast<I3>(hi_pat),
                                 std::string_view{hi.prerelease}},
            range_operator::less_or_equal);
      } else {
        out.emplace_back(
            version<I1, I2, I3>{static_cast<I1>(hi_maj), static_cast<I2>(hi_min), static_cast<I3>(hi_pat)},
            range_operator::less_or_equal);
      }
    } else if (hi.minor.has_value()) {
      // M.m: <M.(m+1).0-0
      if (hi_min == std::numeric_limits<std::uint64_t>::max()) return false;
      auto ub = make_upper_bound<I1, I2, I3>(hi_maj, hi_min + 1, 0);
      if (!ub) return false;
      out.emplace_back(*ub, range_operator::less);
    } else {
      // M: <(M+1).0.0-0
      if (hi_maj == std::numeric_limits<std::uint64_t>::max()) return false;
      auto ub = make_upper_bound<I1, I2, I3>(hi_maj + 1, 0, 0);
      if (!ub) return false;
      out.emplace_back(*ub, range_operator::less);
    }
    return true;
  }

  class range_parser;

  template <typename I1, typename I2, typename I3>
  class range {
  public:
    friend class detail::range_parser;

    SEMVER_CONSTEXPR bool contains(const version<I1, I2, I3>& v, version_compare_option option) const noexcept {
      if (option == version_compare_option::exclude_prerelease) {
        if (!match_at_least_one_comparator_with_prerelease(v)) {
          return false;
        }
      }

      for (const auto& rc : ranges_comparators) {
        if (!rc.contains(v)) return false;
      }
      return true;
    }
  private:
    std::vector<range_comparator<I1, I2, I3>> ranges_comparators;

    SEMVER_CONSTEXPR bool match_at_least_one_comparator_with_prerelease(const version<I1, I2, I3>& v) const noexcept {
      if (v.prerelease_tag().empty()) {
        return true;
      }

      for (const auto& rc : ranges_comparators) {
        const bool has_prerelease = !rc.get_version().prerelease_tag().empty();
        const bool equal_without_prerelease = detail::compare_parsed(v, rc.get_version(), version_compare_option::exclude_prerelease) == 0;
        if (has_prerelease && equal_without_prerelease) return true;
      }
      return false;
    }
  };
}

template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
#if __cpp_concepts >= 201907L
  requires std::unsigned_integral<I1> && std::unsigned_integral<I2> && std::unsigned_integral<I3>
#endif
class range_set {
  static_assert(std::is_unsigned_v<I1>, "semver: I1 must be an unsigned integral type");
  static_assert(std::is_unsigned_v<I2>, "semver: I2 must be an unsigned integral type");
  static_assert(std::is_unsigned_v<I3>, "semver: I3 must be an unsigned integral type");

public:
  friend class detail::range_parser;

  // Returns true if v satisfies at least one OR-separated comparator set.
  // With exclude_prerelease (default): a pre-release v only matches if a comparator
  // in the set explicitly targets the same [major.minor.patch] with a pre-release tag.
  [[nodiscard]] SEMVER_CONSTEXPR bool contains(const version<I1, I2, I3>& v, version_compare_option option = version_compare_option::exclude_prerelease) const noexcept {
    for (const auto& range : ranges) {
      if (range.contains(v, option)) return true;
    }
    return false;
  }

private:
  std::vector<detail::range<I1, I2, I3>> ranges;
};

namespace detail {
  class range_parser {
  public:
    SEMVER_CONSTEXPR explicit range_parser(token_stream& ts) noexcept : stream(ts) {}

    template <typename I1, typename I2, typename I3>
    SEMVER_CONSTEXPR from_chars_result parse(range_set<I1, I2, I3>& out) {
      out.ranges.clear();

      do {
        detail::range<I1, I2, I3> rng;
        if (const auto res = parse_range(rng); !res) {
          return res;
        }

        out.ranges.push_back(std::move(rng));
        skip_whitespaces();

      } while (stream.advance_if_match(token_type::logical_or));

      return success(stream.peek().lexeme);
    }
    
  private:
    token_stream& stream;

    // Returns true if token t can begin a new comparator (operator, digit, *, ~, ^, x, X).
    SEMVER_CONSTEXPR bool can_start_comparator(const token& t) const noexcept {
      return t.type == token_type::range_operator
          || t.type == token_type::digit
          || t.type == token_type::tilde
          || t.type == token_type::caret
          || detail::is_wildcard_token(t); // covers *, x, X
    }

    template <typename I1, typename I2, typename I3>
    SEMVER_CONSTEXPR from_chars_result parse_range(detail::range<I1, I2, I3>& out) {
      do {
        skip_whitespaces();
        const token first = stream.peek();

        if (first.type == token_type::tilde) {
          stream.advance();
          skip_whitespaces();
          detail::partial_version pv;
          version_parser vp{stream};
          if (auto r = vp.parse_partial(pv); !r) return r;
          if (!detail::desugar_tilde<I1, I2, I3>(pv, out.ranges_comparators))
            return failure(first.lexeme, std::errc::result_out_of_range);
        }
        else if (first.type == token_type::caret) {
          stream.advance();
          skip_whitespaces();
          detail::partial_version pv;
          version_parser vp{stream};
          if (auto r = vp.parse_partial(pv); !r) return r;
          if (!detail::desugar_caret<I1, I2, I3>(pv, out.ranges_comparators))
            return failure(first.lexeme, std::errc::result_out_of_range);
        }
        else if (first.type == token_type::range_operator) {
          // Explicit operator (>=, <=, <, >, =): existing strict-version logic.
          if (auto res = parse_range_comparator(out.ranges_comparators); !res) return res;
        }
        else {
          // No operator: bare version, X-range, or the left side of a hyphen range.
          detail::partial_version pv;
          version_parser vp{stream};
          if (auto r = vp.parse_partial(pv); !r) return r;

          skip_whitespaces(); // consume space before potential hyphen

          // Hyphen range: space(s) before '-' consumed by skip_whitespaces above.
          // Scan past '-' and any number of trailing spaces to find digit/wildcard.
          // Requires at least one space on each side (i.e. "1.2.3 - 2.0.0").
          const auto is_hyphen_range = [&]() noexcept -> bool {
            if (stream.peek(0).type != token_type::hyphen) return false;
            std::size_t k = 1;
            while (stream.peek(k).type == token_type::space) ++k;
            return k > 1 // at least one space after hyphen
                   && (stream.peek(k).type == token_type::digit
                       || detail::is_wildcard_token(stream.peek(k)));
          };
          if (is_hyphen_range()) {
            stream.advance(); // hyphen
            skip_whitespaces(); // consume all spaces after hyphen (≥1)
            detail::partial_version pv2;
            version_parser vp2{stream};
            if (auto r = vp2.parse_partial(pv2); !r) return r;
            if (!detail::desugar_hyphen<I1, I2, I3>(pv, pv2, out.ranges_comparators))
              return failure(first.lexeme, std::errc::result_out_of_range);
          }
          else if (pv.has_wildcard()) {
            // X-range (any component is wildcard / unspecified).
            if (!detail::desugar_xrange<I1, I2, I3>(pv, out.ranges_comparators))
              return failure(first.lexeme, std::errc::result_out_of_range);
          }
          else {
            // Plain bare version with all three components — treat as implicit '='.
            if (!pv.major.has_value())
              return failure(first.lexeme);
            const std::uint64_t maj  = *pv.major;
            const std::uint64_t min_ = pv.minor.value_or(0);
            const std::uint64_t pat  = pv.patch.value_or(0);
            if (!detail::number_in_range<I1>(maj) ||
                !detail::number_in_range<I2>(min_) ||
                !detail::number_in_range<I3>(pat))
              return failure(first.lexeme, std::errc::result_out_of_range);
            if (!pv.prerelease.empty()) {
              out.ranges_comparators.emplace_back(
                  version<I1, I2, I3>{static_cast<I1>(maj), static_cast<I2>(min_), static_cast<I3>(pat),
                                      std::string_view{pv.prerelease}},
                  range_operator::equal);
            } else {
              out.ranges_comparators.emplace_back(
                  version<I1, I2, I3>{static_cast<I1>(maj), static_cast<I2>(min_), static_cast<I3>(pat)},
                  range_operator::equal);
            }
          }
        }

        skip_whitespaces();
      } while (can_start_comparator(stream.peek()));

      return success(stream.peek().lexeme);
    }

    // Explicit-operator comparator (>=, <=, <, >, =) + full M.m.p version.
    // Note: partial/wildcard with explicit operators (e.g. ">=1.x") are unsupported — use ~, ^ or X-range syntax.
    template <typename I1, typename I2, typename I3>
    SEMVER_CONSTEXPR from_chars_result parse_range_comparator(std::vector<detail::range_comparator<I1, I2, I3>>& out) {
      range_operator op = range_operator::equal;
      token token;
      if (stream.advance_if_match(token, token_type::range_operator)) {
        op = token.value.op;
      }

      skip_whitespaces();

      version<I1, I2, I3> ver;
      version_parser parser{ stream };
      if (const auto res = parser.parse(ver); !res) {
        return res;
      }

      out.emplace_back(std::move(ver), op);
      return success(stream.peek().lexeme);
    }

    SEMVER_CONSTEXPR void skip_whitespaces() noexcept {
      while (stream.advance_if_match(token_type::space)) {
        ;
      }
    }
  };
} // namespace semver::detail

template <typename I1, typename I2, typename I3>
[[nodiscard]] SEMVER_CONSTEXPR from_chars_result parse(std::string_view str, range_set<I1, I2, I3>& out) {
  if (str.size() > SEMVER_MAX_INPUT_LENGTH) {
    return detail::failure(str.data(), std::errc::value_too_large);
  }

  if (!str.empty() && str.front() == ' ') {
    return detail::failure(str.data());
  }

  if (!str.empty() && str.back() == ' ') {
    return detail::failure(str.data() + str.size() - 1);
  }

  detail::token_stream ts{str.size()};
  const from_chars_result result = detail::lexer{str}.scan_tokens(ts);
  if (!result) {
    return result;
  }

  range_set<I1, I2, I3> tmp;
  const from_chars_result parse_result = detail::range_parser{ ts }.parse(tmp);
  if (!parse_result) {
    return parse_result;
  }

  if (!ts.advance_if_match(detail::token_type::eol)) {
    return detail::failure(ts.peek().lexeme);
  }

  out = std::move(tmp);
  return detail::success(ts.previous().lexeme);
}

// Satisfies check with a pre-parsed range_set.
template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
[[nodiscard]] SEMVER_CONSTEXPR bool satisfies(
    const version<I1, I2, I3>& v,
    const range_set<I1, I2, I3>& rs,
    version_compare_option option = exclude_prerelease) noexcept {
  return rs.contains(v, option);
}

// Parses range_str then checks containment. Re-parses on every call — pre-parse for hot paths.
template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
[[nodiscard]] SEMVER_CONSTEXPR bool satisfies(
    const version<I1, I2, I3>& v,
    std::string_view range_str,
    version_compare_option option = exclude_prerelease) {
  range_set<I1, I2, I3> rs;
  if (!parse(range_str, rs)) return false;
  return rs.contains(v, option);
}

// Bumps v by kind with optional pre-release tag. Returns nullopt if kind == none.
// For pre-* variants, pre defaults to "0". For prerelease without explicit pre:
// increments the last numeric identifier in the tag, or appends ".0" if non-numeric.
// Not constexpr: uses std::from_chars, std::to_string (C++17 limitations).
template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
[[nodiscard]] std::optional<version<I1, I2, I3>> inc(const version<I1, I2, I3>& v, version_diff kind, std::string_view pre = {}) {
  if (kind == version_diff::none) return std::nullopt;
  if (kind == version_diff::major) {
    if (v.major() == std::numeric_limits<I1>::max()) return std::nullopt;
    return v.bump_major();
  }
  if (kind == version_diff::minor) {
    if (v.minor() == std::numeric_limits<I2>::max()) return std::nullopt;
    return v.bump_minor();
  }
  if (kind == version_diff::patch) {
    if (v.patch() == std::numeric_limits<I3>::max()) return std::nullopt;
    return v.bump_patch();
  }

  version<I1, I2, I3> base;
  switch (kind) {
    case version_diff::premajor:
      if (v.major() == std::numeric_limits<I1>::max()) return std::nullopt;
      base = v.bump_major(); break;
    case version_diff::preminor:
      if (v.minor() == std::numeric_limits<I2>::max()) return std::nullopt;
      base = v.bump_minor(); break;
    case version_diff::prepatch:
      if (v.patch() == std::numeric_limits<I3>::max()) return std::nullopt;
      base = v.bump_patch(); break;
    case version_diff::prerelease:
      if (v.prerelease_tag().empty()) {
        if (v.patch() == std::numeric_limits<I3>::max()) return std::nullopt;
        base = v.bump_patch();
      } else {
        base = version<I1, I2, I3>{v.major(), v.minor(), v.patch()};
      }
      break;
    default: return std::nullopt;
  }

  std::string tag;
  if (!pre.empty()) {
    tag = std::string{pre};
  } else if (kind == version_diff::prerelease && !v.prerelease_tag().empty()) {
    // Increment the last numeric identifier in the existing pre-release tag.
    tag = std::string{v.prerelease_tag()};
    const auto dot_pos = tag.rfind('.');
    const std::size_t last_start = (dot_pos == std::string::npos) ? 0 : dot_pos + 1;
    const std::string last_id = tag.substr(last_start);
    const bool is_numeric = detail::is_numeric_identifier(last_id);
    if (is_numeric) {
      // Increment using integer arithmetic; nullopt on overflow.
      std::uint64_t num = 0;
      (void)std::from_chars(last_id.data(), last_id.data() + last_id.size(), num); // all-digit string, never fails
      if (num == std::numeric_limits<std::uint64_t>::max()) return std::nullopt;
      const std::string incremented = std::to_string(num + 1);
      tag = (dot_pos == std::string::npos) ? incremented
                                           : tag.substr(0, dot_pos + 1) + incremented;
    } else {
      tag += ".0";
    }
  } else {
    tag = "0";
  }

  // Direct construction; validate_prerelease_tag guards the 4-arg constructor precondition.
  if (!detail::validate_prerelease_tag(tag)) return std::nullopt;
  return version<I1, I2, I3>{base.major(), base.minor(), base.patch(), std::string_view{tag}};
}

// Returns iterator to the highest version in [first, last) satisfying rs, or last if none.
template <typename ForwardIt, typename I1, typename I2, typename I3>
[[nodiscard]] SEMVER_CONSTEXPR ForwardIt max_satisfying(ForwardIt first, ForwardIt last, const range_set<I1, I2, I3>& rs, version_compare_option opt = version_compare_option::exclude_prerelease) noexcept {
  ForwardIt result = last;
  for (auto it = first; it != last; ++it) {
    if (rs.contains(*it, opt) && (result == last || *result < *it))
      result = it;
  }
  return result;
}

// Returns iterator to the lowest version in [first, last) satisfying rs, or last if none.
template <typename ForwardIt, typename I1, typename I2, typename I3>
[[nodiscard]] SEMVER_CONSTEXPR ForwardIt min_satisfying(ForwardIt first, ForwardIt last, const range_set<I1, I2, I3>& rs, version_compare_option opt = version_compare_option::exclude_prerelease) noexcept {
  ForwardIt result = last;
  for (auto it = first; it != last; ++it) {
    if (rs.contains(*it, opt) && (result == last || *it < *result))
      result = it;
  }
  return result;
}

// Strips leading/trailing whitespace and optional leading '='/'v'/'V', then strict-parses.
// Unlike coerce(), does not fill missing components.
// clean("  =v1.2.3  ") → 1.2.3,  clean("v1.2.3-alpha") → 1.2.3-alpha,
// clean("~1.2.3") → nullopt,  clean("1.2") → nullopt.
template <typename I1 = std::uint32_t, typename I2 = I1, typename I3 = I1>
[[nodiscard]] SEMVER_CONSTEXPR std::optional<version<I1, I2, I3>> clean(std::string_view str) {
  while (!str.empty() && str.front() == ' ') str.remove_prefix(1);
  if (!str.empty() && str.front() == '=')                              str.remove_prefix(1);
  if (!str.empty() && (str.front() == 'v' || str.front() == 'V'))     str.remove_prefix(1);
  while (!str.empty() && str.front() == ' ') str.remove_prefix(1);
  while (!str.empty() && str.back()  == ' ') str.remove_suffix(1);
  return try_parse<I1, I2, I3>(str);
}

inline const version<> library_version{SEMVER_VERSION_MAJOR, SEMVER_VERSION_MINOR, SEMVER_VERSION_PATCH};

#if SEMVER_HAS_CONSTEVAL_LITERAL
namespace literals {
  // Compile-time version literal. Requires SEMVER_HAS_CONSTEVAL_LITERAL == 1.
  consteval version<> operator""_semver(const char* str, std::size_t len) {
    version<> v;
    const auto result = detail::parse_version(std::string_view{str, len}, v);
    if (!result) {
      throw "invalid semver literal";
    }
    return v;
  }
} // namespace literals
#endif

} // namespace semver

// SEMVER_CONSTEXPR is internal; undef'd after use.
// SEMVER_HAS_CONSTEXPR and SEMVER_HAS_CONSTEVAL_LITERAL are user-queryable feature flags.
#undef SEMVER_CONSTEXPR

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

namespace std {
  template <typename I1, typename I2, typename I3>
  struct hash<semver::version<I1, I2, I3>> {
    std::size_t operator()(const semver::version<I1, I2, I3>& v) const noexcept {
      // build_metadata excluded per §10.
      // Fibonacci-derived mixing constant, width-appropriate.
#if SIZE_MAX > 0xFFFFFFFFU
      static constexpr std::size_t kPhiHash = std::size_t{0x9e3779b97f4a7c15ULL};
#else
      static constexpr std::size_t kPhiHash = std::size_t{0x9e3779b9U};
#endif
      static constexpr auto hash_combine = [](std::size_t seed, std::size_t value) noexcept -> std::size_t {
        return seed ^ (value + kPhiHash + (seed << 6) + (seed >> 2));
      };
      std::size_t h = std::hash<I1>{}(v.major());
      h = hash_combine(h, std::hash<I2>{}(v.minor()));
      h = hash_combine(h, std::hash<I3>{}(v.patch()));
      h = hash_combine(h, std::hash<std::string_view>{}(v.prerelease_tag()));
      return h;
    }
  };
} // namespace std

#if __cpp_lib_format >= 202110L
#include <format>
namespace std {
  template <typename I1, typename I2, typename I3>
  struct formatter<semver::version<I1, I2, I3>> {
    constexpr auto parse(format_parse_context& ctx) {
      auto it = ctx.begin();
      if (it != ctx.end() && *it != '}')
        throw format_error("semver::version does not support format specs");
      return it;
    }

    template <typename FormatContext>
    auto format(const semver::version<I1, I2, I3>& v, FormatContext& ctx) const {
      const auto s = v.to_string();
      return std::copy(s.begin(), s.end(), ctx.out());
    }
  };
} // namespace std
#endif

#endif // NEARGYE_SEMANTIC_VERSIONING_HPP
