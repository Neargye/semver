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
// version 0.3.0                              |___/
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2021 Daniil Goncharov <neargye@gmail.com>.
// Copyright (c) 2020 - 2021 Alexander Gorbunov <naratzul@gmail.com>.
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

#define SEMVER_VERSION_MAJOR 0
#define SEMVER_VERSION_MINOR 3
#define SEMVER_VERSION_PATCH 0

#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#if __has_include(<charconv>)
#include <charconv>
#else
#include <system_error>
#endif

// Allow to disable exceptions.
#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)) && !defined(SEMVER_NOEXCEPTION)
#  include <stdexcept>
#  define NEARGYE_THROW(exception) throw exception
#else
#  include <cstdlib>
#  define NEARGYE_THROW(exception) std::abort()
#endif

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wmissing-braces" // Ignore warning: suggest braces around initialization of subobject 'return {first, std::errc::invalid_argument};'.
#endif

namespace semver {

#if __has_include(<charconv>)
struct from_chars_result : std::from_chars_result {
  [[nodiscard]] constexpr operator bool() const noexcept { return ec == std::errc{}; }
};

struct to_chars_result : std::to_chars_result {
  [[nodiscard]] constexpr operator bool() const noexcept { return ec == std::errc{}; }
};
#else
struct from_chars_result {
  const char* ptr;
  std::errc ec;

  [[nodiscard]] constexpr operator bool() const noexcept { return ec == std::errc{}; }
};

struct to_chars_result {
  char* ptr;
  std::errc ec;

  [[nodiscard]] constexpr operator bool() const noexcept { return ec == std::errc{}; }
};
#endif

// Max version string length = 3(<major>) + 1(.) + 3(<minor>) + 1(.) + 3(<patch>) + 1(-) + 5(<prerelease>) + 1(.) + 3(<prereleaseversion>) = 21.
inline constexpr auto max_version_string_length = std::size_t{21};

namespace detail {

// Min version string length = 1(<major>) + 1(.) + 1(<minor>) + 1(.) + 1(<patch>) = 5.
inline constexpr auto min_version_string_length = 5;

constexpr char to_lower(char c) noexcept {
  return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
}

constexpr bool is_digit(char c) noexcept {
  return c >= '0' && c <= '9';
}

constexpr bool is_space(char c) noexcept {
  return c == ' ';
}

constexpr bool is_operator(char c) noexcept {
  return c == '<' || c == '>' || c == '=';
}

constexpr bool is_dot(char c) noexcept {
  return c == '.';
}

constexpr bool is_logical_or(char c) noexcept {
  return c == '|';
}

constexpr bool is_hyphen(char c) noexcept {
  return c == '-';
}

constexpr bool is_plus(char c) noexcept {
  return c == '+';
}

constexpr bool is_letter(char c) noexcept {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

constexpr std::uint8_t to_digit(char c) noexcept {
  return static_cast<std::uint8_t>(c - '0');
}

constexpr std::uint8_t length(std::uint8_t x) noexcept {
  return x < 10 ? 1 : (x < 100 ? 2 : 3);
}

constexpr bool equals(const char* first, const char* last, std::string_view str) noexcept {
  for (std::size_t i = 0; first != last && i < str.length(); ++i, ++first) {
    if (to_lower(*first) != to_lower(str[i])) {
      return false;
    }
  }

  return true;
}

constexpr char* to_chars(char* str, std::uint8_t x, bool dot = true) noexcept {
  do {
    *(--str) = static_cast<char>('0' + (x % 10));
    x /= 10;
  } while (x != 0);

  if (dot) {
    *(--str) = '.';
  }

  return str;
}

constexpr const char* from_chars(const char* first, const char* last, std::uint8_t& d) noexcept {
  if (first != last && is_digit(*first)) {
    std::int32_t t = 0;
    for (; first != last && is_digit(*first); ++first) {
      t = t * 10 + to_digit(*first);
    }
    if (t <= (std::numeric_limits<std::uint8_t>::max)()) {
      d = static_cast<std::uint8_t>(t);
      return first;
    }
  }

  return nullptr;
}

enum class token_type : std::uint8_t {
  none,
  eol,
  space,
  dot,
  integer,
  plus,
  hyphen,
  letter,
  range_operator,
  logical_or
};

enum class range_operator : std::uint8_t {
  less,
  less_or_equal,
  greater,
  greater_or_equal,
  equal
};

struct token_range {
  std::size_t pos;
  std::size_t len;
};

struct token {
  token_type type;
  token_range range;

  constexpr token(token_type type, const token_range& range) : type(type), range(range) { }

  constexpr token(const token&) = default;

  constexpr token(token&&) = default;

  constexpr token& operator=(const token&) = default;

  constexpr token& operator=(token&&) = default;

  ~token() = default;
};

class lexer {

public:
  constexpr explicit lexer(std::string_view text) : text(text), pos(0) { }

  constexpr token get_next_token() {
    const auto pos = this->pos;

    if (is_space(text[pos])) {
      advance(1);
      return { token_type::space, { pos, 1 } };
    }

    if (is_digit(text[pos])) {
      return { token_type::integer, get_int_range() };
    }

    if (is_dot(text[pos])) {
      advance(1);
      return { token_type::dot, { pos, 1 } };
    }

    if (is_hyphen(text[pos])) {
      advance(1);
      return { token_type::hyphen, { pos, 1 } };
    }

    if (is_plus(text[pos])) {
      advance(1);
      return { token_type::plus, { pos, 1 } };
    }

    if (is_letter(text[pos])) {
      advance(1);
      return { token_type::letter, { pos, 1 } };
    }

    if(is_logical_or(text[pos])) {
      advance(2);
      return { token_type::logical_or, { pos, 2 } };
    }

    if (is_operator(text[pos])) {
      return { token_type::range_operator, get_operator_range() };
    }

    if (eol())
      return { token_type::eol, {} };

    // TODO: throw unexpected symbol
    return { token_type::eol, {} };
  }

  constexpr std::uint8_t get_int(std::size_t pos, std::size_t len) const {
    std::uint8_t value = 0;
    
    const auto result = from_chars(text.data() + pos, text.data() + pos + len, value);
    if (!result) {
      // TODO: throw invalid agrument or out of range
      return 0;
    }

    return value;
  }

  constexpr std::string_view get_string(std::size_t pos, std::size_t len) const {
    return text.substr(pos, len);
  }

  constexpr range_operator get_operator(std::size_t pos, std::size_t len) const {
    const auto str = text.substr(pos, len);

    if (str == "<") {
      return range_operator::less;
    } else if (str == "<=") {
      return range_operator::less_or_equal;
    } else if (str == ">") {
      return range_operator::greater;
    } else if (str == ">=") {
      return range_operator::greater_or_equal;
    }

    return range_operator::equal;
  }

private:
  std::string_view text;
  std::size_t pos;

  constexpr bool eol() const noexcept { return pos >= text.size(); }

  constexpr token_range get_int_range() noexcept {
    std::size_t pos = this->pos;
    std::size_t len = 0;

    while (!eol() && is_digit(text[this->pos])) {
      len++;
      advance(1);
    }

    return { pos, len };
  }

  constexpr token_range get_operator_range() noexcept {
    std::size_t pos = this->pos;
    std::size_t len = 0;

    for (std::size_t i = 0; i < 2; ++i) {
      if (!eol() && is_operator(text[pos])) {
        len++;
        advance(1);
      }
    }

    return { pos, len };
  }

  constexpr void advance(std::size_t step) noexcept { pos += step; }
};

class version_parser {

public:
  constexpr explicit version_parser(const lexer& lexer) : lexer(lexer), token(token_type::none, {}) {
    advance(token_type::none);
    skip_whitespaces();
  }

  constexpr std::array<std::uint8_t, 3> parse_core() {
    auto major = lexer.get_int(token.range.pos, token.range.len);
    advance(token_type::integer);
    advance(token_type::dot);

    auto minor = lexer.get_int(token.range.pos, token.range.len);
    advance(token_type::integer);
    advance(token_type::dot);

    auto patch = lexer.get_int(token.range.pos, token.range.len);
    advance(token_type::integer);

    return { major, minor, patch };
  }

  constexpr std::string_view parse_prerelease_tag() {
    if (token.type != token_type::hyphen)
      return {};

    advance(token_type::hyphen);

    const auto pos = token.range.pos;
    std::size_t len = 0;

    while (!is_eol()) {
      if (is_plus())
        break;

      if (!is_alphanumeric() && !is_integer()) {
        // TODO: throw invalid version syntax
        break;
      }

      len += token.range.len;
      advance(token.type);
    }

    return lexer.get_string(pos, len);
  }

  constexpr std::string_view parse_build() {
    if (token.type != token_type::plus)
      return {};

    advance(token_type::plus);

    const auto pos = token.range.pos;
    std::size_t len = 0;

    while (!is_eol()) {
      if (!is_alphanumeric() && !is_integer()) {
        // TODO: throw invalid version syntax
        break;
      }

      len += token.range.len;
      advance(token.type);
    }

    return lexer.get_string(pos, len);
  }

private:
  lexer lexer;
  token token;

  constexpr void skip_whitespaces() {
    while (token.type == token_type::space) {
        advance(token_type::space);
    }
  }

  constexpr void advance(token_type expected_token) {
    if (token.type != expected_token) {
      // TODO: throw unexpected token
    }
    token = lexer.get_next_token();
  }

  constexpr bool is_eol() const {
    return token.type == token_type::eol;
  }

  constexpr bool is_plus() const {
    return token.type == token_type::plus;
  }

  constexpr bool is_alphanumeric() const {
    return token.type == token_type::letter || token.type == token_type::dot || token.type == token_type::hyphen;
  }

  constexpr bool is_integer() const {
    return token.type == token_type::integer;
  }
};

} // namespace semver::detail

class version {
  std::uint8_t major             = 0;
  std::uint8_t minor             = 1;
  std::uint8_t patch             = 0;
  std::string_view prerelease_tag = {};
  std::string_view build_metadata = {};

public:
  constexpr version(std::uint8_t major,
                    std::uint8_t minor,
                    std::uint8_t patch,
                    std::string_view prerelease_tag = {},
                    std::string_view build_metadata = {}) noexcept
        : major(major),
          minor(minor),
          patch(patch),
          prerelease_tag(prerelease_tag),
          build_metadata(build_metadata) {
  }

  explicit constexpr version(std::string_view str) : version(0, 0, 0) {
    from_string(str);
  }

  constexpr version() = default; // https://semver.org/#how-should-i-deal-with-revisions-in-the-0yz-initial-development-phase

  constexpr version(const version&) = default;

  constexpr version(version&&) = default;

  ~version() = default;

  version& operator=(const version&) = default;

  version& operator=(version&&) = default;

  [[nodiscard]] constexpr from_chars_result from_chars(const char* first, const char* last) noexcept {
    if (first == nullptr || last == nullptr || (last - first) < detail::min_version_string_length) {
      return {first, std::errc::invalid_argument};
    }

    detail::lexer lexer(std::string_view(first, last - first));
    detail::version_parser parser(lexer);

    const auto& core = parser.parse_core();
    major = core[0];
    minor = core[1];
    patch = core[2];

    prerelease_tag = parser.parse_prerelease_tag();
    build_metadata = parser.parse_build();

    return {first, std::errc{}};
  }

  [[nodiscard]] constexpr to_chars_result to_chars(char* first, char* last) const noexcept {
    const auto length = string_length();
    if (first == nullptr || last == nullptr || (last - first) < length) {
      return {last, std::errc::value_too_large};
    }

    // auto next = first + length;
    // if (prerelease_type != prerelease::none) {
    //   if (prerelease_number != 0) {
    //     next = detail::to_chars(next, prerelease_number);
    //   }
    //   next = detail::to_chars(next, prerelease_type);
    // }
    // next = detail::to_chars(next, patch);
    // next = detail::to_chars(next, minor);
    // next = detail::to_chars(next, major, false);

    return {first + length, std::errc{}};
  }

  [[nodiscard]] constexpr bool from_string_noexcept(std::string_view str) noexcept {
    return from_chars(str.data(), str.data() + str.length());
  }

  constexpr version& from_string(std::string_view str) {
    if (!from_string_noexcept(str)) {
      NEARGYE_THROW(std::invalid_argument{"semver::version::from_string invalid version."});
    }

    return *this;
  }

  [[nodiscard]] std::string to_string() const {
    auto str = std::string(string_length(), '\0');
    if (!to_chars(str.data(), str.data() + str.length())) {
      NEARGYE_THROW(std::invalid_argument{"semver::version::to_string invalid version."});
    }

    return str;
  }

  [[nodiscard]] constexpr std::uint8_t string_length() const noexcept {
    // (<major>) + 1(.) + (<minor>) + 1(.) + (<patch>)
    auto length = detail::length(major) + detail::length(minor) + detail::length(patch) + 2;
    // if (prerelease_type != prerelease::none) {
    //   // + 1(-) + (<prerelease>)
    //   length += detail::length(prerelease_type) + 1;
    //   if (prerelease_number != 0) {
    //     // + 1(.) + (<prereleaseversion>)
    //     length += detail::length(prerelease_number) + 1;
    //   }
    // }

    return static_cast<std::uint8_t>(length);
  }

  [[nodiscard]] constexpr int compare(const version& other) const noexcept {
    if (major != other.major) {
      return major - other.major;
    }

    if (minor != other.minor) {
      return minor - other.minor;
    }

    if (patch != other.patch) {
      return patch - other.patch;
    }

    int pre_release_compare = prerelease_tag.compare(other.prerelease_tag);
    if (pre_release_compare != 0) {
        return pre_release_compare;
    }

    int build_metadata_compare = build_metadata.compare(other.build_metadata);
    if (build_metadata_compare != 0) {
        return build_metadata_compare;
    }

    return 0;
  }

  constexpr std::uint8_t get_major() const noexcept { return major; }

  constexpr std::uint8_t get_minor() const noexcept { return minor; }

  constexpr std::uint8_t get_patch() const noexcept { return patch; }

  constexpr std::string_view get_prerelese_tag() const noexcept { return prerelease_tag; }

  constexpr std::string_view get_build_metadata() const noexcept { return build_metadata; }
};

[[nodiscard]] constexpr bool operator==(const version& lhs, const version& rhs) noexcept {
  return lhs.compare(rhs) == 0;
}

[[nodiscard]] constexpr bool operator!=(const version& lhs, const version& rhs) noexcept {
  return lhs.compare(rhs) != 0;
}

[[nodiscard]] constexpr bool operator>(const version& lhs, const version& rhs) noexcept {
  return lhs.compare(rhs) > 0;
}

[[nodiscard]] constexpr bool operator>=(const version& lhs, const version& rhs) noexcept {
  return lhs.compare(rhs) >= 0;
}

[[nodiscard]] constexpr bool operator<(const version& lhs, const version& rhs) noexcept {
  return lhs.compare(rhs) < 0;
}

[[nodiscard]] constexpr bool operator<=(const version& lhs, const version& rhs) noexcept {
  return lhs.compare(rhs) <= 0;
}

[[nodiscard]] constexpr version operator""_version(const char* str, std::size_t length) {
  return version{std::string_view{str, length}};
}

[[nodiscard]] constexpr bool valid(std::string_view str) noexcept {
  return version{}.from_string_noexcept(str);
}

[[nodiscard]] constexpr from_chars_result from_chars(const char* first, const char* last, version& v) noexcept {
  return v.from_chars(first, last);
}

[[nodiscard]] constexpr to_chars_result to_chars(char* first, char* last, const version& v) noexcept {
  return v.to_chars(first, last);
}

[[nodiscard]] constexpr std::optional<version> from_string_noexcept(std::string_view str) noexcept {
  if (version v{}; v.from_string_noexcept(str)) {
    return v;
  }

  return std::nullopt;
}

[[nodiscard]] constexpr version from_string(std::string_view str) {
  return version{str};
}

[[nodiscard]] inline std::string to_string(const version& v) {
  return v.to_string();
}

template <typename Char, typename Traits>
inline std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& os, const version& v) {
  for (const auto c : v.to_string()) {
    os.put(c);
  }

  return os;
}

inline namespace comparators {

enum struct comparators_option : std::uint8_t {
  exclude_prerelease,
  include_prerelease
};

[[nodiscard]] constexpr int compare(const version& lhs, const version& rhs, comparators_option option = comparators_option::include_prerelease) noexcept {
  if (option == comparators_option::exclude_prerelease) {
    return version{lhs.get_major(), lhs.get_minor(), lhs.get_patch()}.compare(version{rhs.get_major(), rhs.get_minor(), rhs.get_patch()});
  }
  return lhs.compare(rhs);
}

[[nodiscard]] constexpr bool equal_to(const version& lhs, const version& rhs, comparators_option option = comparators_option::include_prerelease) noexcept {
  return compare(lhs, rhs, option) == 0;
}

[[nodiscard]] constexpr bool not_equal_to(const version& lhs, const version& rhs, comparators_option option = comparators_option::include_prerelease) noexcept {
  return compare(lhs, rhs, option) != 0;
}

[[nodiscard]] constexpr bool greater(const version& lhs, const version& rhs, comparators_option option = comparators_option::include_prerelease) noexcept {
  return compare(lhs, rhs, option) > 0;
}

[[nodiscard]] constexpr bool greater_equal(const version& lhs, const version& rhs, comparators_option option = comparators_option::include_prerelease) noexcept {
  return compare(lhs, rhs, option) >= 0;
}

[[nodiscard]] constexpr bool less(const version& lhs, const version& rhs, comparators_option option = comparators_option::include_prerelease) noexcept {
  return compare(lhs, rhs, option) < 0;
}

[[nodiscard]] constexpr bool less_equal(const version& lhs, const version& rhs, comparators_option option = comparators_option::include_prerelease) noexcept {
  return compare(lhs, rhs, option) <= 0;
}

} // namespace semver::comparators

namespace range {

namespace detail {

using namespace semver::detail;

class range {
 public:
  constexpr explicit range(std::string_view str) noexcept : str_{str} {}

  constexpr bool satisfies(const version& ver, bool include_prerelease) const {
    range_parser parser{str_};

    auto is_logical_or = [&parser]() constexpr noexcept -> bool { return parser.current_token.type == token_type::logical_or; };

    auto is_operator = [&parser]() constexpr noexcept -> bool { return parser.current_token.type == token_type::range_operator; };

    auto is_int = [&parser]() constexpr noexcept -> bool { return parser.current_token.type == token_type::integer; };

    const bool has_prerelease = !ver.get_prerelese_tag().empty();

    do {
      if (is_logical_or()) {
        parser.advance_token(token_type::logical_or);
      }

      bool contains = true;
      bool allow_compare = include_prerelease;

      while (is_operator() || is_int()) {
        const auto range = parser.parse_range();
        const bool equal_without_tags = equal_to(range.ver, ver, comparators_option::exclude_prerelease);

        if (has_prerelease && equal_without_tags) {
          allow_compare = true;
        }

        if (!range.satisfies(ver)) {
          contains = false;
          break;
        }
      }

      if (has_prerelease) {
        if (allow_compare && contains) {
          return true;
        }
      } else if (contains) {
        return true;
      }

    } while (is_logical_or());
    
    return false;
  }

private:
  struct range_comparator {
    range_operator op;
    version ver;

    constexpr bool satisfies(const version& version) const {
      switch (op) {
        case range_operator::equal:
          return version == ver;
        case range_operator::greater:
          return version > ver;
        case range_operator::greater_or_equal:
          return version >= ver;
        case range_operator::less:
          return version < ver;
        case range_operator::less_or_equal:
          return version <= ver;
        default:
          NEARGYE_THROW(std::invalid_argument{"semver::range unexpected operator."});
      }
    }
  };

  struct range_parser {
    lexer lexer;
    token current_token;

    constexpr explicit range_parser(std::string_view str) : lexer{str}, current_token{token_type::none, {}} {
      advance_token(token_type::none);
    }

    constexpr void advance_token(token_type token_type) {
      if (current_token.type != token_type) {
        NEARGYE_THROW(std::invalid_argument{"semver::range unexpected token."});
      }
      current_token = lexer.get_next_token();
    }

    constexpr range_comparator parse_range() {
      if (current_token.type == token_type::integer) {
        const auto version = parse_version();
        return {range_operator::equal, version};
      } else if (current_token.type == token_type::range_operator) {
        const auto range_operator = lexer.get_operator(current_token.range.pos, current_token.range.len);
        advance_token(token_type::range_operator);
        const auto version = parse_version();
        return {range_operator, version};
      }

      // TODO: unexpected token
      return {range_operator::equal, version{}};
    }

    constexpr version parse_version() {
      version_parser version_parser(lexer);

      const auto core = version_parser.parse_core();
      const auto prerelease_tag = version_parser.parse_prerelease_tag();
      const auto build_metadata = version_parser.parse_build();

      return {core[0], core[1], core[2], prerelease_tag, build_metadata};
    }
  };

  std::string_view str_;
};

} // namespace semver::range::detail

enum struct satisfies_option : std::uint8_t {
  exclude_prerelease,
  include_prerelease
};

constexpr bool satisfies(const version& ver, std::string_view str, satisfies_option option = satisfies_option::exclude_prerelease) {
  switch (option) {
  case satisfies_option::exclude_prerelease:
    return detail::range{str}.satisfies(ver, false);
  case satisfies_option::include_prerelease:
    return detail::range{str}.satisfies(ver, true);
  default:
    NEARGYE_THROW(std::invalid_argument{"semver::range unexpected satisfies_option."});
  }
}

} // namespace semver::range

// Version lib semver.
inline constexpr auto semver_version = version{SEMVER_VERSION_MAJOR, SEMVER_VERSION_MINOR, SEMVER_VERSION_PATCH};

} // namespace semver

#undef NEARGYE_THROW

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

#endif // NEARGYE_SEMANTIC_VERSIONING_HPP
