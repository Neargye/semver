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
// version 0.4.0                              |___/
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2022 Daniil Goncharov <neargye@gmail.com>.
// Copyright (c) 2020 - 2022 Alexander Gorbunov <naratzul@gmail.com>.
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

#include <algorithm>
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

#if defined(SEMVER_CONFIG_FILE)
#include SEMVER_CONFIG_FILE
#endif

#if defined(SEMVER_THROW)
// define SEMVER_THROW(msg) to override semver throw behavior.
#elif defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
#  include <stdexcept>
#  define SEMVER_THROW(msg) (throw std::invalid_argument{msg})
#else
#  include <cassert>
#  include <cstdlib>
#  define SEMVER_THROW(msg) (assert(!msg), std::abort())
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

namespace detail {

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

template<typename Int>
constexpr std::size_t length(Int x) noexcept {
  std::size_t number_of_digits = 0;
  do {
    ++number_of_digits;
    x /= 10;
  } while (x != 0);

  return number_of_digits;
}

template<typename Int>
constexpr char* to_chars(char* str, Int x) noexcept {
  do {
    *(--str) = static_cast<char>('0' + (x % 10));
    x /= 10;
  } while (x != 0);

  return str;
}

constexpr char* to_chars(char* str, std::string_view s) noexcept {
  for (std::size_t i = s.size(); i > 0; --i) {
    *(--str) = s[i - 1];
  }

  return str;
}

template<typename Int>
constexpr bool parse_int(std::string_view s, Int& d) noexcept {
  int i = 0;
  Int t = 0;
  for (char c : s) {
    t = t * 10 + to_digit(c);
    ++i;
  }
  if (i <= std::numeric_limits<Int>::digits10) {
    d = t;
    return true;
  }

  return false;
}

// std implementation can throw
constexpr bool substr(std::string_view str, std::size_t pos, std::size_t len, std::string_view& result) noexcept {
  if (pos > str.size()) {
    return false;
  }

  result = std::string_view{str.data() + pos, len};
  return result.length() == len;
}

constexpr int compare(std::string_view lhs, std::string_view rhs) {
#if defined(_MSC_VER) && _MSC_VER < 1920 && !defined(__clang__)
  // https://developercommunity.visualstudio.com/content/problem/360432/vs20178-regression-c-failed-in-test.html
  // https://developercommunity.visualstudio.com/content/problem/232218/c-constexpr-string-view.html
  constexpr bool workaround = true;
#else
  constexpr bool workaround = false;
#endif

  if constexpr (workaround) {
    const auto size = std::min(lhs.size(), rhs.size());
    for (std::size_t i = 0; i < size; ++i) {
      if (lhs[i] < rhs[i]) {
        return -1;
      } else if (lhs[i] > rhs[i]) {
        return 1;
      }
    }

    return static_cast<int>(lhs.size() - rhs.size());
  } else {
    return lhs.compare(rhs);
  }
}

constexpr bool compare_equal(std::string_view lhs, std::string_view rhs) {
  return compare(lhs, rhs) == 0;
}

constexpr int compare_numerically(std::string_view lhs, std::string_view rhs) {
  // assume that strings don't have leading zeros (we've already checked it at parsing stage).

  if (lhs.size() != rhs.size()) {
    return static_cast<int>(lhs.size() - rhs.size());
  }

  for (std::size_t i = 0; i < lhs.size(); ++i) {
    int a = lhs[i] - '0';
    int b = rhs[i] - '0';
    if (a != b) {
      return a - b;
    }
  }

  return 0;
}

enum class token_type : std::uint8_t {
  none,
  eol,
  space,
  dot,
  plus,
  hyphen,
  letter,
  range_operator,
  logical_or,
  numeric_identifier,
  non_numeric_identifier,
  unexpected
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
};

class lexer {
 public:
  explicit constexpr lexer(std::string_view text) noexcept : text_{text}, current_pos_{0} {}

  constexpr token get_next_token() noexcept {
    if (eol()) {
      return {token_type::eol, {}};
    }

    const auto pos = current_pos_;

    if (is_space(text_[pos])) {
      advance(1);
      return {token_type::space, {pos, 1}};
    }

    if (is_digit(text_[pos])) {
      return {token_type::numeric_identifier, get_int_range()};
    }

    if (is_dot(text_[pos])) {
      advance(1);
      return {token_type::dot, {pos, 1}};
    }

    if (is_hyphen(text_[pos])) {
      advance(1);
      return {token_type::hyphen, {pos, 1}};
    }

    if (is_plus(text_[pos])) {
      advance(1);
      return {token_type::plus, {pos, 1}};
    }

    if (is_letter(text_[pos])) {
      advance(1);
      return {token_type::letter, {pos, 1}};
    }

    if (is_logical_or(text_[pos])) {
      advance(2);
      return {token_type::logical_or, {pos, 2}};
    }

    if (is_operator(text_[pos])) {
      return {token_type::range_operator, get_operator_range()};
    }

    return {token_type::unexpected, {}};
  }

  constexpr bool get_string(std::size_t pos, std::size_t len, std::string_view& result) const noexcept {
    return substr(text_, pos, len, result);
  }

  constexpr char get_letter_at(std::size_t pos) const noexcept {
    return text_[pos];
  }

  constexpr bool get_operator(std::size_t pos, std::size_t len, range_operator& result) const noexcept {
    std::string_view str;
    if (!substr(text_, pos, len, str)) {
      return false;
    }

    if (compare_equal(str, "=")) {
      result = range_operator::equal;
    } else if (compare_equal(str, "<")) {
      result = range_operator::less;
    } else if (compare_equal(str, "<=")) {
      result = range_operator::less_or_equal;
    } else if (compare_equal(str, ">")) {
      result = range_operator::greater;
    } else if (compare_equal(str, ">=")) {
      result = range_operator::greater_or_equal;
    } else {
      return false;
    }

    return true;
  }

 private:
  std::string_view text_;
  std::size_t current_pos_;

  constexpr bool eol() const noexcept { return current_pos_ >= text_.size(); }

  constexpr token_range get_int_range() noexcept {
    std::size_t pos = current_pos_;
    std::size_t len = 0;

    while (!eol() && is_digit(text_[current_pos_])) {
      len++;
      advance(1);
    }

    return {pos, len};
  }

  constexpr token_range get_operator_range() noexcept {
    std::size_t pos = current_pos_;
    std::size_t len = 0;

    for (std::size_t i = 0; i < 2; ++i) {
      if (!eol() && is_operator(text_[current_pos_])) {
        len++;
        advance(1);
      }
    }

    return {pos, len};
  }

  constexpr void advance(std::size_t step) noexcept { current_pos_ += step; }
};

class prerelease_identifier_parser {
public:
  explicit constexpr prerelease_identifier_parser(const lexer& lexer) noexcept : lexer_{lexer}, token_{token_type::none, {}} {
    advance();
  }

  constexpr token get_next_identifier() noexcept {
    if (is_dot()) {
      advance();
    }

    const auto pos = token_.range.pos;
    std::size_t len = 0;

    bool non_numeric = false;

    // identifiers are dot separated.
    while (!is_dot() && !is_eol()) {
      if (is_letter() || is_hyphen()) {
        non_numeric = true;
      }
      len += token_.range.len;
      advance();
    }

    if (len == 0) {
      return {token_type::eol, {}};
    }

    if (non_numeric) {
      return {token_type::non_numeric_identifier, {pos, len}};
    }

    return {token_type::numeric_identifier, {pos, len}};
  }

private:
  lexer lexer_;
  token token_;

  constexpr void advance() { token_ = lexer_.get_next_token(); }

  constexpr bool is_eol() const { return token_.type == token_type::eol; }

  constexpr bool is_dot() const { return token_.type == token_type::dot; }

  constexpr bool is_letter() const { return token_.type == token_type::letter; }

  constexpr bool is_hyphen() const { return token_.type == token_type::hyphen; }

};

class prerelease_comparator {
public:
  constexpr prerelease_comparator(std::string_view lhs, std::string_view rhs) noexcept : lhs{lhs}, rhs{rhs} {}

  [[nodiscard]] constexpr int compare() const noexcept {
    prerelease_identifier_parser lhs_parser{lhs}, rhs_parser{rhs};
    token lhs_identifier = lhs_parser.get_next_identifier();
    token rhs_identifier = rhs_parser.get_next_identifier();

    while (lhs_identifier.type != token_type::eol && rhs_identifier.type != token_type::eol) {
      const int compare_result = compare_identifier(lhs_identifier, rhs_identifier);
      if (compare_result != 0) {
        return compare_result;
      }
      lhs_identifier = lhs_parser.get_next_identifier();
      rhs_identifier = rhs_parser.get_next_identifier();
    }

    if (lhs_identifier.type != token_type::eol && rhs_identifier.type == token_type::eol) {
      return 1;
    } else if (lhs_identifier.type == token_type::eol && rhs_identifier.type != token_type::eol) {
      return -1;
    }

    return 0;
  }

private:
  lexer lhs, rhs;

  [[nodiscard]] constexpr int compare_identifier(const token& lhs_id, const token& rhs_id) const noexcept {
    if (lhs_id.type == token_type::numeric_identifier && rhs_id.type == token_type::numeric_identifier) {
      return compare_numeric(lhs_id, rhs_id);
    } else if (lhs_id.type == token_type::non_numeric_identifier && rhs_id.type == token_type::non_numeric_identifier) {
      return compare_non_numeric(lhs_id, rhs_id);
    }

    return lhs_id.type == token_type::non_numeric_identifier ? 1 : -1;
  }

  [[nodiscard]] constexpr int compare_numeric(const token& lhs_identifier, const token& rhs_identifier) const noexcept {
    std::string_view lhs_sv, rhs_sv;
    lhs.get_string(lhs_identifier.range.pos, lhs_identifier.range.len, lhs_sv);
    rhs.get_string(rhs_identifier.range.pos, rhs_identifier.range.len, rhs_sv);

    return detail::compare_numerically(lhs_sv, rhs_sv);
  }

  [[nodiscard]] constexpr int compare_non_numeric(const token& lhs_identifier, const token& rhs_identifier) const noexcept {
    std::string_view lhs_sv, rhs_sv;
    lhs.get_string(lhs_identifier.range.pos, lhs_identifier.range.len, lhs_sv);
    rhs.get_string(rhs_identifier.range.pos, rhs_identifier.range.len, rhs_sv);

    return detail::compare(lhs_sv, rhs_sv);
  }
};

class version_parser {
 public:
  constexpr version_parser(lexer& lexer, token& token) : lexer_{lexer}, token_{token} {
  }

  constexpr bool init() {
    if (token_.type == token_type::none) {
      return advance(token_type::none);
    }
    return true;
  }

  constexpr bool parse_major(std::string_view& major) {
    return parse_number(major) &&
           advance(token_type::numeric_identifier) &&
           advance(token_type::dot);
  }

  constexpr bool parse_minor(std::string_view& minor) {
    return parse_number(minor) &&
           advance(token_type::numeric_identifier) &&
           advance(token_type::dot);
  }

  constexpr bool parse_patch(std::string_view& patch) {
    return parse_number(patch) &&
           advance(token_type::numeric_identifier);
  }

  constexpr bool parse_prerelease(std::string_view& prerelease) {
    if (token_.type != token_type::hyphen) {
      prerelease = {};
      return true;
    }

    if (!advance(token_type::hyphen)) {
      return false;
    }

    const auto pos = token_.range.pos;
    std::size_t len = 0;

    bool identifier_expected = true;

    while (identifier_expected || !is_eol()) {
      if (token_.range.len == 0) {
       return false;
      }

      if (is_plus() || is_space()) {
        break;
      }

      if (is_dot()) {
        if (identifier_expected) {
          return false;
        }
        identifier_expected = true;
      } else {
        if (!is_letter() && !is_hyphen() && !is_integer()) {
          return false;
        }
        identifier_expected = false;
      }

      // numeric identifiers with leading zeros are invalid in prerelease tag.
      if (is_integer() && has_leading_zero()) {
        return false;
      }

      len += token_.range.len;
      if (!advance(token_.type)) {
        return false;
      }
    }

    return lexer_.get_string(pos, len, prerelease);
  }

  constexpr bool parse_build(std::string_view& build_metadata) {
    if (token_.type != token_type::plus) {
      build_metadata = {};
      return is_eol();
    }

    if (!advance(token_type::plus)) {
      return false;
    }

    const auto pos = token_.range.pos;
    std::size_t len = 0;

    bool identifier_expected = true;

    while (identifier_expected || !is_eol()) {
      if (token_.range.len == 0) {
        return false;
      }

      if (is_dot()) {
        if (identifier_expected) {
          return false;
        }
        identifier_expected = true;
      } else {
        if (!is_letter() && !is_hyphen() && !is_integer()) {
          return false;
        }
        identifier_expected = false;
      }

      len += token_.range.len;

      if (!advance(token_.type)) {
        return false;
      }
    }

    return lexer_.get_string(pos, len, build_metadata);
  }

 private:
  lexer& lexer_;
  token& token_;

  [[nodiscard]] constexpr bool advance(token_type expected_token) {
    if (token_.type != expected_token) {
      return false;
    }

    token_ = lexer_.get_next_token();
    return token_.type != token_type::unexpected;
  }

  constexpr bool parse_number(std::string_view& value) {
    // version with leading zeros in [major, minor, patch] is invalid.
    if (has_leading_zero())
      return false;

    return lexer_.get_string(token_.range.pos, token_.range.len, value);
  }

  constexpr bool has_leading_zero() const {
    return token_.range.len > 1 && lexer_.get_letter_at(token_.range.pos) == '0';
  }

  constexpr bool is_eol() const {
    return token_.type == token_type::eol;
  }

  constexpr bool is_plus() const {
    return token_.type == token_type::plus;
  }

  constexpr bool is_space() const {
    return token_.type == token_type::space;
  }

  constexpr bool is_letter() const {
    return token_.type == token_type::letter;
  }

  constexpr bool is_hyphen() const {
    return token_.type == token_type::hyphen;
  }

  constexpr bool is_dot() const {
    return token_.type == token_type::dot;
  }

  constexpr bool is_integer() const {
    return token_.type == token_type::numeric_identifier;
  }
};

constexpr bool is_prerelease_valid(std::string_view pr) noexcept {
  lexer lexer(pr);
  detail::token token{detail::token_type::hyphen, detail::token_range{0, 1}};
  version_parser parser(lexer, token);
  std::string_view prerelease;
  return parser.init() && parser.parse_prerelease(prerelease);
}

constexpr bool is_build_metadata_valid(std::string_view bm) noexcept {
  lexer lexer(bm);
  detail::token token{detail::token_type::plus, detail::token_range{0, 1}};
  version_parser parser(lexer, token);
  std::string_view build_metadata;
  return parser.init() && parser.parse_build(build_metadata);
}

constexpr int compare_prerelease(std::string_view lhs, std::string_view rhs) noexcept {
  return prerelease_comparator{lhs, rhs}.compare();
}

struct version {
  std::string_view major;
  std::string_view minor;
  std::string_view patch;
  std::string_view prerelease;
  std::string_view build_metadata;
};

constexpr int compare(const version& lhs, const version& rhs) {
  if (int result = compare_numerically(lhs.major, rhs.major); result != 0) {
    return result;
  }

  if (int result = compare_numerically(lhs.minor, rhs.minor); result != 0) {
    return result;
  }

  if (int result = compare_numerically(lhs.patch, rhs.patch); result != 0) {
    return result;
  }

  if (lhs.prerelease.empty() != rhs.prerelease.empty()) {
    return static_cast<int>(rhs.prerelease.size() - lhs.prerelease.size());
  }

  return detail::compare_prerelease(lhs.prerelease, rhs.prerelease);
}

constexpr bool parse(std::string_view str, version& result) {
  lexer lexer{str};
  detail::token token{token_type::none, token_range{}};
  version_parser parser{lexer, token};
  return parser.init() && parser.parse_major(result.major) &&
         parser.parse_minor(result.minor) &&
         parser.parse_patch(result.patch) &&
         parser.parse_prerelease(result.prerelease) &&
         parser.parse_build(result.build_metadata);
}

constexpr int parse_and_compare(std::string_view lhs, std::string_view rhs) {
  // TODO: error handling
  version lhs_version, rhs_version;
  parse(lhs, lhs_version);
  parse(rhs, rhs_version);
  return compare(lhs_version, rhs_version);
}

} // namespace semver::detail

template<typename I1, typename I2, typename I3>
constexpr bool parse(std::string_view version, I1& major, I2& minor, I3& patch,
                     char* prerelease_begin = nullptr,
                     char* prerelease_end = nullptr,
                     char* build_begin = nullptr,
                     char* build_end = nullptr) {
  detail::version result;
  if (!detail::parse(version, result)) {
    return false;
  }

  if (!detail::parse_int(result.major, major)) {
    return false;
  }

  if (!detail::parse_int(result.minor, minor)) {
    return false;
  }

  if (!detail::parse_int(result.patch, patch)) {
    return false;
  }

  if (!result.prerelease.empty()) {
    if (prerelease_begin && prerelease_end) {
      auto length = static_cast<long>(result.prerelease.size());
      if (prerelease_end - prerelease_begin < length) {
        return false;
      }
      std::copy(result.prerelease.begin(), result.prerelease.end(),
                prerelease_begin);
    }
  }

  if (!result.build_metadata.empty()) {
    if (build_begin && build_end) {
      auto length = static_cast<long>(result.build_metadata.size());
      if (build_end - build_begin < length) {
        return false;
      }
      std::copy(result.build_metadata.begin(), result.build_metadata.end(),
                build_begin);
    }
  }

  return true;
}

template <typename I>
constexpr std::optional<I> major(std::string_view version) {
  detail::version parsed_version;
  if (!detail::parse(version, parsed_version)) {
    return std::nullopt;
  }

  I result;
  if (!detail::parse_int(parsed_version.major, result)) {
    return false;
  }

  return result;
}

template <typename I>
constexpr std::optional<I> minor(std::string_view version) {
  detail::version parsed_version;
  if (!detail::parse(version, parsed_version)) {
    return std::nullopt;
  }

  I result;
  if (!detail::parse_int(parsed_version.minor, result)) {
    return false;
  }

  return result;
}

template <typename I>
constexpr std::optional<I> patch(std::string_view version) {
  detail::version parsed_version;
  if (!detail::parse(version, parsed_version)) {
    return std::nullopt;
  }

  I result;
  if (!detail::parse_int(parsed_version.patch, result)) {
    return false;
  }

  return result;
}

constexpr std::string_view prerelease(std::string_view version) {
  detail::version parsed_version;
  if (!detail::parse(version, parsed_version)) {
    return {};
  }

  return parsed_version.prerelease;
}

constexpr std::string_view build_metadata(std::string_view version) {
  detail::version parsed_version;
  if (!detail::parse(version, parsed_version)) {
    return {};
  }

  return parsed_version.build_metadata;
}

constexpr bool valid(std::string_view version) {
  detail::version v;
  return detail::parse(version, v);
}

constexpr bool equal(std::string_view lhs, std::string_view rhs) {
  return detail::parse_and_compare(lhs, rhs) == 0;
}

constexpr bool not_equal(std::string_view lhs, std::string_view rhs) {
  return detail::parse_and_compare(lhs, rhs) != 0;
}

constexpr bool greater(std::string_view lhs, std::string_view rhs) {
  return detail::parse_and_compare(lhs, rhs) > 0;
}

constexpr bool greater_equal(std::string_view lhs, std::string_view rhs) {
  return detail::parse_and_compare(lhs, rhs) >= 0;
}

constexpr bool less(std::string_view lhs, std::string_view rhs) {
  return detail::parse_and_compare(lhs, rhs) < 0;
}

constexpr bool less_equal(std::string_view lhs, std::string_view rhs) {
  return detail::parse_and_compare(lhs, rhs) <= 0;
}

inline namespace comparators {

enum struct comparators_option : std::uint8_t {
  exclude_prerelease,
  include_prerelease
};

[[nodiscard]] constexpr int compare(const detail::version& lhs, const detail::version& rhs, comparators_option option = comparators_option::include_prerelease) noexcept {
  if (option == comparators_option::exclude_prerelease) {
    return detail::compare(detail::version{lhs.major, lhs.minor, lhs.patch, {}, {}},
                           detail::version{rhs.major, rhs.minor, rhs.patch, {}, {}});
  }

  return detail::compare(lhs, rhs);
}

} // namespace semver::comparators

namespace range {

namespace detail {

using namespace semver::detail;

class range {
 public:
  explicit constexpr range(std::string_view str) noexcept : parser{str} {}

  constexpr bool satisfies(std::string_view version_str, bool include_prerelease) {
    version version{};
    if (!parse(version_str, version)) {
      SEMVER_THROW("semver::range invalid version.");
    }

    if (!parser.init()) {
      SEMVER_THROW("semver::range invalid range.");
    }

    const bool has_prerelease = !version.prerelease.empty();

    do {
      if (is_logical_or_token()) {
        if (!parser.advance(token_type::logical_or) ||
            !parser.skip_whitespaces()) {
          SEMVER_THROW("semver::range invalid range.");
        }
      }

      bool contains = true;
      bool allow_compare = include_prerelease;

      while (is_operator_token() || is_number_token()) {
        const auto range_opt = parser.parse_range();
        if (!range_opt.has_value()) {
          SEMVER_THROW("semver::range invalid range.");
        }

        const auto range = *range_opt;
        const bool equal_without_tags = compare(range.ver, version, comparators_option::exclude_prerelease) == 0;

        if (has_prerelease && equal_without_tags) {
          allow_compare = true;
        }

        if (!range.satisfies(version)) {
          contains = false;
          break;
        }

        if (!parser.skip_whitespaces()) {
          SEMVER_THROW("semver::range invalid range.");
        }
      }

      if (has_prerelease) {
        if (allow_compare && contains) {
          return true;
        }
      } else if (contains) {
        return true;
      }

      if (!parser.skip_whitespaces()) {
        SEMVER_THROW("semver::range invalid range.");
      }
    } while (is_logical_or_token());
    
    return false;
  }

 private:
  struct range_comparator {
    range_operator op;
    version ver;

    constexpr bool satisfies(const version& version) const {
      switch (op) {
        case range_operator::equal:
          return detail::compare(version, ver) == 0;
        case range_operator::greater:
          return detail::compare(version, ver) > 0;
        case range_operator::greater_or_equal:
          return detail::compare(version, ver) >= 0;
        case range_operator::less:
          return detail::compare(version, ver) < 0;
        case range_operator::less_or_equal:
          return detail::compare(version, ver) <= 0;
        default:
          SEMVER_THROW("semver::range unexpected operator.");
      }
    }
  };

  struct range_parser {
    lexer lexer_;
    token token_;

    explicit constexpr range_parser(std::string_view str) : lexer_{str}, token_{token_type::none, token_range{}} {}

    constexpr bool init() {
      return advance(token_type::none);
    }

    constexpr bool advance(token_type token_type) {
      if (token_.type != token_type) {
        return false;
      }
      token_ = lexer_.get_next_token();
      return true;
    }

    constexpr bool skip_whitespaces() {
      while (token_.type == token_type::space) {
        if (!advance(token_type::space)) {
          return false;
        }
      }
      return true;
    }

    constexpr std::optional<range_comparator> parse_range() {
      if (token_.type == token_type::numeric_identifier) {
        if (const auto version = parse_version()) {
          return range_comparator{range_operator::equal, *version};
        }
      } else if (token_.type == token_type::range_operator) {
        auto range_operator = range_operator::equal;
        if (lexer_.get_operator(token_.range.pos, token_.range.len, range_operator) &&
            advance(token_type::range_operator)) {
          if (const auto version = parse_version()) {
            return range_comparator{range_operator, *version};
          }
        }
      }

      return std::nullopt;
    }

    constexpr std::optional<version> parse_version() {
      if (!skip_whitespaces()) {
        return std::nullopt;
      }

      version_parser v_parser(lexer_, token_);

      std::string_view major, minor, patch;
      std::string_view prerelease;

      if (v_parser.init() &&
          v_parser.parse_major(major) &&
          v_parser.parse_minor(minor) &&
          v_parser.parse_patch(patch) &&
          v_parser.parse_prerelease(prerelease)) { // TODO: parse build?
        return version{major, minor, patch, prerelease, {}};
      }

      return std::nullopt;
    }
  };

  [[nodiscard]] constexpr bool is_logical_or_token() const noexcept {
    return parser.token_.type == token_type::logical_or;
  }
  [[nodiscard]] constexpr bool is_operator_token() const noexcept {
    return parser.token_.type == token_type::range_operator;
  }

  [[nodiscard]] constexpr bool is_number_token() const noexcept {
    return parser.token_.type == token_type::numeric_identifier;
  }

  range_parser parser;
};

} // namespace semver::range::detail

enum struct satisfies_option : std::uint8_t {
  exclude_prerelease,
  include_prerelease
};

constexpr bool satisfies(std::string_view version, std::string_view range, satisfies_option option = satisfies_option::exclude_prerelease) {
  switch (option) {
  case satisfies_option::exclude_prerelease:
    return detail::range{range}.satisfies(version, false);
  case satisfies_option::include_prerelease:
    return detail::range{range}.satisfies(version, true);
  default:
    SEMVER_THROW("semver::range unexpected satisfies_option.");
  }
}

} // namespace semver::range

// Version lib semver.
inline constexpr auto semver_version = "0.4.0";

} // namespace semver

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

#endif // NEARGYE_SEMANTIC_VERSIONING_HPP
