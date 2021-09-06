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
#define SEMVER_VERSION_MINOR 4
#define SEMVER_VERSION_PATCH 0

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

using int_t = std::uint32_t;

namespace detail {

// Min version string length = 1(<major>) + 1(.) + 1(<minor>) + 1(.) + 1(<patch>) = 5.
inline constexpr auto min_version_string_length = 5;

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

constexpr std::size_t length(int_t x) noexcept {
  // TODO: more fast
  std::size_t number_of_digits = 0;
  do {
    ++number_of_digits;
    x /= 10;
  } while (x != 0);

  return number_of_digits;
}

constexpr char* to_chars(char* str, int_t x) noexcept {
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

constexpr const char* from_chars(const char* first, const char* last, int_t& d) noexcept {
  if (first != last && is_digit(*first)) {
    int i   = 0;
    int_t t = 0;
    for (; first != last && is_digit(*first); ++first, ++i) {
      t = t * 10 + to_digit(*first);
    }
    if (i <= std::numeric_limits<int_t>::digits10) {
      d = t;
      return first;
    }
  }

  return nullptr;
}

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

    if (lhs.size() < rhs.size()) {
      return -1;
    }

    if (lhs.size() > rhs.size()) {
      return 1;
    }

    return 0;
  } else {
    return lhs.compare(rhs);
  }
}

constexpr bool compare_equal(std::string_view lhs, std::string_view rhs) {
  return compare(lhs, rhs) == 0;
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
  logical_or,
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

  constexpr token(token_type type, const token_range& range) : type(type), range(range) { }

  constexpr token(const token&) = default;

  constexpr token(token&&) = default;

  constexpr token& operator=(const token&) = default;

  constexpr token& operator=(token&&) = default;

  ~token() = default;
};

class lexer {
 public:
  constexpr explicit lexer(std::string_view text) noexcept : text_(text), current_pos_(0) { }

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
      return {token_type::integer, get_int_range()};
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

  constexpr bool get_int(std::size_t pos, std::size_t len, int_t& result) const noexcept {
    return from_chars(text_.data() + pos, text_.data() + pos + len, result);
  }

  constexpr bool get_string(std::size_t pos, std::size_t len, std::string_view& result) const noexcept {
    return substr(text_, pos, len, result);
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

class version_parser {
 public:
  constexpr explicit version_parser(const lexer& lexer, const token& token) : lexer_(lexer), token_(token), length_(0) {
    // TODO: err handle
    if (token.type == token_type::none) {
      [[maybe_unused]] bool success = advance(token_type::none);
    }
    [[maybe_unused]] bool success = skip_whitespaces();
  }

  constexpr bool parse_core(int_t &major, int_t &minor, int_t &patch) {
    return lexer_.get_int(token_.range.pos, token_.range.len, major) &&
           advance(token_type::integer) &&
           advance(token_type::dot) &&
           lexer_.get_int(token_.range.pos, token_.range.len, minor) &&
           advance(token_type::integer) &&
           advance(token_type::dot) &&
           lexer_.get_int(token_.range.pos, token_.range.len, patch) &&
           advance(token_type::integer);
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

    while (!is_eol()) {
      if (is_plus())
        break;

      if (!is_alphanumeric() && !is_integer()) {
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
      return true;
    }

    if (!advance(token_type::plus)) {
      return false;
    }

    const auto pos = token_.range.pos;
    std::size_t len = 0;

    while (!is_eol()) {
      if (!is_alphanumeric() && !is_integer()) {
        return false;
      }

      len += token_.range.len;
      if (!advance(token_.type)) {
        return false;
      }
    }

    return lexer_.get_string(pos, len, build_metadata);
  }

  constexpr token get_token() const noexcept { return token_; }

  constexpr lexer get_lexer() const noexcept { return lexer_; }

  constexpr std::size_t get_length() const noexcept { return length_; }

 private:
  lexer lexer_;
  token token_;
  std::size_t length_; // TODO: IMPL

  constexpr bool skip_whitespaces() {
    while (token_.type == token_type::space) {
      if (!advance(token_type::space)) {
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] constexpr bool advance(token_type expected_token) {
    if (token_.type != expected_token) {
      return false;
    }

    token_ = lexer_.get_next_token();
    return token_.type != token_type::unexpected;
  }

  constexpr bool is_eol() const {
    return token_.type == token_type::eol;
  }

  constexpr bool is_plus() const {
    return token_.type == token_type::plus;
  }

  constexpr bool is_alphanumeric() const {
    return token_.type == token_type::letter || token_.type == token_type::dot || token_.type == token_type::hyphen;
  }

  constexpr bool is_integer() const {
    return token_.type == token_type::integer;
  }
};

} // namespace semver::detail

class version {
  int_t major                     = 0;
  int_t minor                     = 1;
  int_t patch                     = 0;
  std::string_view prerelease     = {};
  std::string_view build_metadata = {};

 public:
  constexpr version(int_t mj,
                    int_t mn,
                    int_t pt,
                    std::string_view pr = {},
                    std::string_view bm = {}) noexcept
        : major(mj),
          minor(mn),
          patch(pt),
          prerelease(pr),
          build_metadata(bm) {
    // TODO: add validate prerelease
    // TODO: add validate build_metadata
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
    const auto length = (last - first);
    if (first == nullptr || last == nullptr || length < 0 || length < detail::min_version_string_length) {
      return {first, std::errc::invalid_argument};
    }

    detail::lexer lexer(std::string_view(first, static_cast<std::size_t>(length)));
    detail::version_parser parser(lexer, detail::token(detail::token_type::none, {0, 0}));

    if (parser.parse_core(major, minor, patch) && parser.parse_prerelease(prerelease) && parser.parse_build(build_metadata)) {
      return {first + parser.get_length(), std::errc{}};
    }

    // TODO: std::errc::result_out_of_range
    return {first, std::errc::invalid_argument};
  }

  [[nodiscard]] constexpr to_chars_result to_chars(char* first, char* last) const noexcept {
    const auto length = string_length();
    if (first == nullptr || last == nullptr || (last - first) < 0 || static_cast<std::size_t>(last - first) < length) {
      return {last, std::errc::value_too_large};
    }

    auto next = first + length;
    if (prerelease.length() > 0) {
      if (build_metadata.length() > 0) {
        next = detail::to_chars(next, build_metadata);
      }
      next = detail::to_chars(next, prerelease);
    }
    next = detail::to_chars(next, patch);
    *(--next) = '.';
    next = detail::to_chars(next, minor);
    *(--next) = '.';
    next = detail::to_chars(next, major);

    return {first + length, std::errc{}};
  }

  [[nodiscard]] constexpr bool from_string_noexcept(std::string_view str) noexcept {
    return from_chars(str.data(), str.data() + str.length());
  }

  constexpr version& from_string(std::string_view str) {
    if (!from_chars(str.data(), str.data() + str.length())) {
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

  // TODO: consider separator characters ('-' for prerelease, '+' for build metadata)
  [[nodiscard]] constexpr std::size_t string_length() const noexcept {
    // length(<major>) + length(.) + length(<minor>) + length(.) + length(<patch>) + length(<prerelease>) + length(<build_metadata>)
    return detail::length(major) + detail::length(minor) + detail::length(patch) + 2 + prerelease.length() + build_metadata.length();
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

    int pre_release_compare = 0;
    if (prerelease.empty() && !other.prerelease.empty()) {
      pre_release_compare = 1;
    } else if (!prerelease.empty() && other.prerelease.empty()) {
      pre_release_compare = -1;
    } else {
      pre_release_compare = detail::compare(prerelease, other.prerelease);
    }

    if (pre_release_compare != 0) {
      return pre_release_compare;
    }

    return 0;
  }

  [[nodiscard]] constexpr int_t get_major() const noexcept { return major; }

  constexpr version& set_major(int_t mj) noexcept {
    major = mj;

    return *this;
  }

  [[nodiscard]] constexpr int_t get_minor() const noexcept { return minor; }

  constexpr version& set_minor(int_t mn) noexcept {
    minor = mn;

    return *this;
  }

  [[nodiscard]] constexpr int_t get_patch() const noexcept { return patch; }

  constexpr version& set_patch(int_t pt) noexcept {
    patch = pt;

    return *this;
  }

  [[nodiscard]] constexpr std::string_view get_prerelease() const noexcept { return prerelease; }

  constexpr version& set_prerelease(std::string_view pr) {
    // TODO: add validate prerelease
    prerelease = pr;

    return *this;
  }

  [[nodiscard]] constexpr std::string_view get_build_metadata() const noexcept { return build_metadata; }

  constexpr version& set_build_metadata(std::string_view bm) {
    // TODO: add validate build_metadata
    build_metadata = bm;

    return *this;
  }
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
  if (version v; v.from_string_noexcept(str)) {
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

    const bool has_prerelease = !ver.get_prerelease().empty();

    do {
      if (is_logical_or()) {
        parser.advance_token(token_type::logical_or);
        parser.skip_whitespaces();
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

        parser.skip_whitespaces();
      }

      if (has_prerelease) {
        if (allow_compare && contains) {
          return true;
        }
      } else if (contains) {
        return true;
      }

      parser.skip_whitespaces();

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
    lexer lexer_;
    token current_token;

    constexpr explicit range_parser(std::string_view str) : lexer_{str}, current_token{token_type::none, {}} {
      advance_token(token_type::none);
    }

    constexpr void advance_token(token_type token_type) {
      if (current_token.type != token_type) {
        NEARGYE_THROW(std::invalid_argument{"semver::range unexpected token."});
      }
      current_token = lexer_.get_next_token();
    }

    constexpr void skip_whitespaces() {
      while (current_token.type == token_type::space) {
        advance_token(token_type::space);
      }
    }

    constexpr range_comparator parse_range() {
      if (current_token.type == token_type::integer) {
        const auto version = parse_version();
        return {range_operator::equal, version};
      } else if (current_token.type == token_type::range_operator) {
        auto range_operator = range_operator::equal;
        lexer_.get_operator(current_token.range.pos, current_token.range.len, range_operator);
        advance_token(token_type::range_operator);
        const auto version = parse_version();
        return {range_operator, version};
      }

      // TODO: unexpected token
      return {range_operator::equal, version{}};
    }

    constexpr version parse_version() {
      version_parser version_parser(lexer_, current_token);

      int_t major = 0;
      int_t minor = 0;
      int_t patch = 0;
      version_parser.parse_core(major, minor, patch);

      std::string_view prerelease;
      version_parser.parse_prerelease(prerelease);
      std::string_view build_metadata;
      version_parser.parse_build(build_metadata);

      current_token = version_parser.get_token();
      lexer_ = version_parser.get_lexer();

      return {major, minor, patch, prerelease, build_metadata};
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
