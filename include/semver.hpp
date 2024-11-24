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
// version 0.3.1                              |___/
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2024 Daniil Goncharov <neargye@gmail.com>.
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
#define SEMVER_VERSION_PATCH 1

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#if __has_include(<charconv>)
#include <charconv>
#else
#include <system_error>
#endif

#if defined(SEMVER_CONFIG_FILE)
#include SEMVER_CONFIG_FILE
#endif

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wmissing-braces" // Ignore warning: suggest braces around initialization of subobject 'return {first, std::errc::invalid_argument};'.
#endif

#if __cpp_impl_three_way_comparison >= 201907L
#include <compare>
#endif

namespace semver {

  namespace detail {
    enum struct prerelease_identifier_type {
      numeric,
      alphanumeric
    };

    struct prerelease_identifier {
      prerelease_identifier_type type;
      std::string identifier;
    };

    class version_parser;
    class prerelease_comparator;
  }

  template <typename I1 = int, typename I2 = int, typename I3 = int>
  class version {
  public:
    friend class detail::version_parser;
    friend class detail::prerelease_comparator;

    constexpr version() = default; // https://semver.org/#how-should-i-deal-with-revisions-in-the-0yz-initial-development-phase
    constexpr version(const version&) = default;
    constexpr version(version&&) = default;
    ~version() = default;

    version& operator=(const version&) = default;
    version& operator=(version&&) = default;

    constexpr I1 major() const noexcept { return major_; }
    constexpr I2 minor() const noexcept { return minor_; }
    constexpr I3 patch() const noexcept { return patch_; }

    constexpr const std::string& prerelease_tag() const { return prerelease_tag_; }
    constexpr const std::string& build_metadata() const { return build_metadata_; }

  private:
    I1 major_ = 0;
    I2 minor_ = 1;
    I3 patch_ = 0;
    std::string prerelease_tag_;
    std::string build_metadata_;

    std::vector<detail::prerelease_identifier> prerelease_identifiers;
  };

#if __has_include(<charconv>)
  struct from_chars_result : std::from_chars_result {
    [[nodiscard]] constexpr operator bool() const noexcept { return ec == std::errc{}; }
  };
#else
  struct from_chars_result {
    const char* ptr;
    std::errc ec;

    [[nodiscard]] constexpr operator bool() const noexcept { return ec == std::errc{}; }
  };
#endif

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

constexpr char to_char(int i) noexcept {
  return '0' + (char)i;
}

template<typename R, typename T>
constexpr bool number_in_range(T t) noexcept {
  return std::cmp_greater_equal(t, std::numeric_limits<R>::min()) && 
         std::cmp_less_equal(t, std::numeric_limits<R>::max());
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
  eol,
  space,
  dot,
  plus,
  hyphen,
  letter,
  digit,
  range_operator,
  logical_or,
};

enum class range_operator : std::uint8_t {
  less,
  less_or_equal,
  greater,
  greater_or_equal,
  equal
};

struct token {
  token_type type;
  std::variant<std::monostate, std::uint8_t, char, range_operator> value;
};

class lexer {
 public:
  explicit constexpr lexer(std::string_view text) noexcept : text_{text}, current_pos_{0} {}

  constexpr std::errc advance(token& token) noexcept {
    if (eol(current_pos_)) {
      token = { token_type::eol, {} };
      return std::errc{};
    }

    const char c = advance();
    return get_token(c, token);
  }

  constexpr std::errc peek(token& token, int k = 0) noexcept {
    if (eol(current_pos_ + k)) {
      token = { token_type::eol, {} };
      return std::errc{};
    }

    return get_token(text_[current_pos_ + k], token);
  }

  constexpr const char* get_current_symbol() const noexcept {
    return text_.data() + current_pos_;
  }

  constexpr const char* get_next_symbol() const noexcept {
    return text_.data() + (current_pos_ + 1);
  }

 private:
  std::string_view text_;
  std::size_t current_pos_;

  constexpr std::errc get_token(char c, token& token) noexcept {
    switch (c) {
    case ' ':
      token = { token_type::space, {} };
      break;
    case '.':
      token = { token_type::dot, {} };
      break;
    case '-':
      token = { token_type::hyphen, {} };
      break;
    case '+':
      token = { token_type::plus, {} };
      break;
    case '|':
      return logical_or(token);
    case '<':
      token = { token_type::range_operator, advanceIfMatch('=') ? range_operator::less_or_equal : range_operator::less };
      break;
    case '>':
      token = { token_type::range_operator, advanceIfMatch('=') ? range_operator::greater_or_equal : range_operator::greater };
      break;
    default:
      if (is_digit(c)) {
        token = { token_type::digit, to_digit(c) };
        break;
      }
      else if (is_letter(c)) {
        token = { token_type::letter, c };
        break;
      }
      return std::errc::invalid_argument;
    }

    return std::errc{};
  }

  constexpr std::errc logical_or(token& token) noexcept {
    if (eol(current_pos_)) {
      token = { token_type::eol, {} };
      return std::errc{};
    }

    const char c = advance();
    if (c == '|') {
      token = { token_type::logical_or, {} };
      return std::errc{};
    }

    return std::errc::invalid_argument;
  }

  constexpr bool eol(std::size_t pos) const noexcept { return pos >= text_.size(); }

  constexpr char advance() noexcept { 
    char c = text_[current_pos_]; 
    current_pos_ += 1;
    return c;
  }

  constexpr bool advanceIfMatch(char c) noexcept {
    if (eol(current_pos_)) {
      return false;
    }

    if (text_[current_pos_] != c) {
      return false;
    }

    current_pos_ += 1;

    return true;
  }
};

class prerelease_comparator {
public:
  template <typename I1, typename I2, typename I3>
  [[nodiscard]] constexpr int compare(const version<I1, I2, I3>& lhs, const version<I1, I2, I3>& rhs) const noexcept {
    if (lhs.prerelease_identifiers.empty() != rhs.prerelease_identifiers.empty()) {
      return static_cast<int>(rhs.prerelease_identifiers.size()) - static_cast<int>(lhs.prerelease_identifiers.size());
    }

    const std::size_t count = std::min(lhs.prerelease_identifiers.size(), rhs.prerelease_identifiers.size());

    for (std::size_t i = 0; i < count; ++i) {
      const int compare_result = compare_identifier(lhs.prerelease_identifiers[i], rhs.prerelease_identifiers[i]);
      if (compare_result != 0) {
        return compare_result;
      }
    }

    return static_cast<int>(lhs.prerelease_identifiers.size()) - static_cast<int>(rhs.prerelease_identifiers.size());
  }

private:
  [[nodiscard]] constexpr int compare_identifier(const prerelease_identifier& lhs, const prerelease_identifier& rhs) const noexcept {
    if (lhs.type == prerelease_identifier_type::numeric && rhs.type == prerelease_identifier_type::numeric) {
      return compare_numerically(lhs.identifier, rhs.identifier);
    } else if (lhs.type == prerelease_identifier_type::alphanumeric && rhs.type == prerelease_identifier_type::alphanumeric) {
      return detail::compare(lhs.identifier, rhs.identifier);
    }

    return lhs.type == prerelease_identifier_type::alphanumeric ? 1 : -1;
  }
};

class version_parser {
 public:
  constexpr explicit version_parser(lexer& lexer) : lexer_{lexer} {
  }

  template <typename I1, typename I2, typename I3>
  constexpr from_chars_result parse(version<I1, I2, I3>& out) noexcept {
    from_chars_result result;

    if (result = parse_number(out.major_); !result) {
      return result;
    }

    if (!consume(token_type::dot)) {
      return failure(lexer_.get_current_symbol());
    }

    if (result = parse_number(out.minor_); !result) {
      return result;
    }

    if (!consume(token_type::dot)) {
      return failure(lexer_.get_current_symbol());
    }

    if (result = parse_number(out.patch_); !result) {
      return result;
    }

    if (advanceIfMatch(token_type::hyphen)) {
      if (result = parse_prerelease_tag(out.prerelease_tag_, out.prerelease_identifiers); !result) {
        return failure(lexer_.get_current_symbol());
      }
    }

    if (advanceIfMatch(token_type::plus)) {
      if (result = parse_build_metadata(out.build_metadata_); !result) {
        return failure(lexer_.get_current_symbol());
      }
    }

    if (!advanceIfMatch(token_type::eol)) {
      return failure(lexer_.get_current_symbol());
    }

    return success(lexer_.get_next_symbol());
  }


 private:
  lexer& lexer_;
  token token_;

  template <typename Int>
  constexpr from_chars_result parse_number(Int& out) {
    if (const auto ret = advance(); ret != std::errc{}) {
      return failure(lexer_.get_current_symbol(), ret);
    }

    if (!is_digit(token_)) {
      return failure(lexer_.get_current_symbol());
    }

    const auto first_digit = std::get<std::uint8_t>(token_.value);
    std::uint64_t result = first_digit;

    if (first_digit == 0) {
      out = static_cast<Int>(result);
      return success(lexer_.get_next_symbol());
    }

    token next;
    while (advanceIfMatch(token_type::digit)) {
      result = result * 10 + std::get<std::uint8_t>(token_.value);
    }

    if (detail::number_in_range<Int>(result)) {
      out = static_cast<Int>(result);
      return success(lexer_.get_next_symbol());
    }

    return failure(lexer_.get_current_symbol(), std::errc::result_out_of_range);
  }

  constexpr from_chars_result parse_prerelease_tag(std::string& out, std::vector<detail::prerelease_identifier>& out_identifiers) {
    std::string result;

    do {
      if (!result.empty()) {
        result.push_back('.');
      }

      if (const auto res = advance(); res != std::errc{}) {
        return failure(lexer_.get_current_symbol(), res);
      }

      std::string identifier;
      if (const auto res = parse_prerelease_identifier(identifier); !res) {
        return res;
      }

      result.append(identifier);
      out_identifiers.push_back(make_prerelease_identifier(identifier));

    } while (advanceIfMatch(token_type::dot));

    out = result;
    return success(lexer_.get_next_symbol());
  }

  constexpr from_chars_result parse_build_metadata(std::string& out) {
    std::string result;

    do {
      if (!result.empty()) {
        result.push_back('.');
      }

      if (const auto res = advance(); res != std::errc{}) {
        return failure(lexer_.get_current_symbol(), res);
      }

      std::string identifier;
      if (const auto res = parse_build_identifier(identifier); !res) {
        return res;
      }

      result.append(identifier);
    } while (advanceIfMatch(token_type::dot));

    out = result;
    return success(lexer_.get_next_symbol());
  }

  constexpr from_chars_result parse_prerelease_identifier(std::string& out) {
    std::string result;

    do {
      switch (token_.type) {
      case token_type::hyphen:
        result.push_back('-');
        break;
      case token_type::letter:
        result.push_back(std::get<char>(token_.value));
        break;
      case token_type::digit:
      {
        const auto digit = std::get<std::uint8_t>(token_.value);

        // numerical prerelease identifier doesn't allow leading zero 
        // 1.2.3-1.alpha is valid,
        // 1.2.3-01b is valid as well, but
        // 1.2.3-01.alpha is not valid

        if (is_leading_zero(digit)) {
          return failure(lexer_.get_current_symbol());
        }

        result.push_back(to_char(digit));
        break;
      }
      default:
        return failure(lexer_.get_current_symbol());
      }
    } while (advanceIfMatch(token_type::hyphen) || advanceIfMatch(token_type::letter) || advanceIfMatch(token_type::digit));

    out = result;
    return success(lexer_.get_next_symbol());
  }

  constexpr detail::prerelease_identifier make_prerelease_identifier(const std::string& identifier) {
    auto type = detail::prerelease_identifier_type::numeric;
    for (char c : identifier) {
      if (c == '-' || detail::is_letter(c)) {
        type = detail::prerelease_identifier_type::alphanumeric;
        break;
      }
    }
    return detail::prerelease_identifier{ type, identifier };
  }

  constexpr from_chars_result parse_build_identifier(std::string& out) {
    std::string result;

    do {
      switch (token_.type) {
      case token_type::hyphen:
        result.push_back('-');
        break;
      case token_type::letter:
        result.push_back(std::get<char>(token_.value));
        break;
      case token_type::digit:
      {
        const auto digit = std::get<std::uint8_t>(token_.value);
        result.push_back(to_char(digit));
        break;
      }
      default:
        return failure(lexer_.get_current_symbol());
      }
    } while (advanceIfMatch(token_type::hyphen) || advanceIfMatch(token_type::letter) || advanceIfMatch(token_type::digit));

    out = result;
    return success(lexer_.get_next_symbol());
  }

  constexpr bool is_leading_zero(int digit) noexcept {
    if (digit != 0) {
      return false;
    }

    token next;
    int k = 0;
    int alpha_numerics = 0;
    int digits = 0;

    while (true) {
      if (lexer_.peek(next, k) != std::errc{}) {
        break;
      }

      if (!is_alphanumeric(next)) {
        break;
      }

      ++alpha_numerics;

      if (is_digit(next)) {
        ++digits;
      }

      ++k;
    }

    return digits > 0 && digits == alpha_numerics;
  }

  constexpr std::errc advance() noexcept {
    return lexer_.advance(token_);
  }

  constexpr bool advanceIfMatch(token_type token_type) {
    token next;
    if (lexer_.peek(next) == std::errc{} && next.type == token_type) {
      lexer_.advance(token_);
      return true;
    }

    return false;
  }

  constexpr bool consume(token_type token_type) noexcept {
    return lexer_.advance(token_) == std::errc{} && token_.type == token_type;
  }

  constexpr bool is_digit(const token& token) const noexcept {
    return token.type == token_type::digit;
  }

  constexpr bool is_eol(const token& token) const noexcept {
    return token.type == token_type::eol;
  }

  constexpr bool is_alphanumeric(const token& token) const noexcept {
    return token.type == token_type::hyphen || token.type == token_type::letter || token.type == token_type::digit;
  }
};

template <typename I1, typename I2, typename I3>
constexpr int compare_prerelease(const version<I1, I2, I3>& lhs, const version<I1, I2, I3>& rhs) noexcept {
  return prerelease_comparator{}.compare(lhs, rhs);
}

template <typename I1, typename I2, typename I3>
constexpr int compare_parsed(const version<I1, I2, I3>& lhs, const version<I1, I2, I3>& rhs) {
  int result = lhs.major() - rhs.major();
  if (result != 0) {
    return result;
  }

  result = lhs.minor() - rhs.minor();
  if (result != 0) {
    return result;
  }

  result = lhs.patch() - rhs.patch();
  if (result != 0) {
    return result;
  }

  return detail::compare_prerelease(lhs, rhs);
}

template <typename I1, typename I2, typename I3>
constexpr from_chars_result parse(std::string_view str, version<I1, I2, I3>& result) {
  lexer lexer{ str };
  return version_parser{lexer}.parse(result);
}

} // namespace semver::detail

template <typename I1, typename I2, typename I3>
[[nodiscard]] constexpr bool operator==(const version<I1, I2, I3>& lhs, const version<I1, I2, I3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs) == 0;
}

template <typename I1, typename I2, typename I3>
[[nodiscard]] constexpr bool operator!=(const version<I1, I2, I3>& lhs, const version<I1, I2, I3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs) != 0;
}

template <typename I1, typename I2, typename I3>
[[nodiscard]] constexpr bool operator>(const version<I1, I2, I3>& lhs, const version<I1, I2, I3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs) > 0;
}

template <typename I1, typename I2, typename I3>
[[nodiscard]] constexpr bool operator>=(const version<I1, I2, I3>& lhs, const version<I1, I2, I3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs) >= 0;
}

template <typename I1, typename I2, typename I3>
[[nodiscard]] constexpr bool operator<(const version<I1, I2, I3>& lhs, const version<I1, I2, I3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs) < 0;
}

template <typename I1, typename I2, typename I3>
[[nodiscard]] constexpr bool operator<=(const version<I1, I2, I3>& lhs, const version<I1, I2, I3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs) <= 0;
}

#if __cpp_impl_three_way_comparison >= 201907L
template <typename I1, typename I2, typename I3>
[[nodiscard]] constexpr std::strong_ordering operator<=>(const version<I1, I2, I3>& lhs, const version<I1, I2, I3>& rhs) {
  int compare = detail::compare_parsed(lhs, rhs);
  if (compare == 0)
    return std::strong_ordering::equal;
  if (compare > 0)
    return std::strong_ordering::greater;
  return std::strong_ordering::less;
}
#endif

template<typename I1, typename I2, typename I3>
constexpr from_chars_result parse(std::string_view str, version<I1, I2, I3>& output) {
  return detail::parse(str, output);
}

constexpr bool valid(std::string_view str) {
  version v{};
  return detail::parse(str, v);
}

#ifdef RANGES

inline namespace comparators {

enum struct comparators_option : std::uint8_t {
  exclude_prerelease,
  include_prerelease
};

[[nodiscard]] constexpr int compare(const detail::version_view& lhs, const detail::version_view& rhs, comparators_option option = comparators_option::include_prerelease) noexcept {
  if (option == comparators_option::exclude_prerelease) {
    return detail::compare(detail::version_view{lhs.major, lhs.minor, lhs.patch, {}, {}},
                           detail::version_view{rhs.major, rhs.minor, rhs.patch, {}, {}});
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
    version_view version{};
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
    version_view ver;

    constexpr bool satisfies(const version_view& version) const {
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

    constexpr std::optional<version_view> parse_version() {
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
          v_parser.parse_prerelease(prerelease)) {
        return version_view{major, minor, patch, prerelease, {}};
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

#endif

// Version lib semver.
inline constexpr auto semver_version = "0.4.0";

} // namespace semver

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

#endif // NEARGYE_SEMANTIC_VERSIONING_HPP
