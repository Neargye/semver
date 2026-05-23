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
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <system_error>

#ifndef SEMVER_MAX_INPUT_LENGTH
#define SEMVER_MAX_INPUT_LENGTH 4096
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

#if __cpp_concepts >= 201907L
#include <concepts>
#endif

#if __cpp_lib_constexpr_string >= 201907L
#define SEMVER_CONSTEXPR constexpr
#else
#define SEMVER_CONSTEXPR inline
#endif

#ifdef __OpenBSD__
/* undef sys/types.h macro */
#undef major
#undef minor
#endif

namespace semver {

  namespace detail {

    template <typename Int>
    constexpr std::size_t length(Int n) noexcept {
      auto un = static_cast<std::make_unsigned_t<Int>>(n);
      std::size_t digits = 0;
      do { ++digits; un /= 10; } while (un != 0);
      return digits;
    }

    template <typename OutputIt, typename Int>
    constexpr OutputIt to_chars(OutputIt dest, Int n) noexcept {
      auto un = static_cast<std::make_unsigned_t<Int>>(n);
      do {
        *(--dest) = static_cast<char>('0' + (un % 10));
        un /= 10;
      } while (un != 0);
      return dest;
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

#if __cpp_concepts >= 201907L
  template <std::integral I1 = int, std::integral I2 = I1, std::integral I3 = I1>
#else
  template <typename I1 = int, typename I2 = I1, typename I3 = I1>
#endif
  class version {
    static_assert(std::is_integral_v<I1>, "semver: I1 must be an integral type");
    static_assert(std::is_integral_v<I2>, "semver: I2 must be an integral type");
    static_assert(std::is_integral_v<I3>, "semver: I3 must be an integral type");

    friend class detail::version_parser;

  public:
    SEMVER_CONSTEXPR version() = default; // https://semver.org/#how-should-i-deal-with-revisions-in-the-0yz-initial-development-phase
    SEMVER_CONSTEXPR version(const version&) = default;
    SEMVER_CONSTEXPR version(version&&) = default;
    SEMVER_CONSTEXPR ~version() = default;

    SEMVER_CONSTEXPR version(I1 major, I2 minor, I3 patch) noexcept : major_(major), minor_(minor), patch_(patch) {
      assert(!detail::cmp_less(major, I1{0}) && "major must be non-negative");
      assert(!detail::cmp_less(minor, I2{0}) && "minor must be non-negative");
      assert(!detail::cmp_less(patch, I3{0}) && "patch must be non-negative");
    }

    SEMVER_CONSTEXPR version& operator=(const version&) = default;
    SEMVER_CONSTEXPR version& operator=(version&&) = default;

    [[nodiscard]] SEMVER_CONSTEXPR I1 major() const noexcept { return major_; }
    [[nodiscard]] SEMVER_CONSTEXPR I2 minor() const noexcept { return minor_; }
    [[nodiscard]] SEMVER_CONSTEXPR I3 patch() const noexcept { return patch_; }

    [[nodiscard]] constexpr std::string_view prerelease_tag() const noexcept { return prerelease_tag_; }
    [[nodiscard]] constexpr std::string_view build_metadata() const noexcept { return build_metadata_; }

    [[nodiscard]] SEMVER_CONSTEXPR std::string to_string() const;

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

#if __cpp_concepts >= 201907L
  template <std::integral I1, std::integral I2, std::integral I3>
#else
  template <typename I1, typename I2, typename I3>
#endif
  SEMVER_CONSTEXPR std::string version<I1, I2, I3>::to_string() const {
    std::string result;
    result.resize(length());

    auto it = result.end();
    if (!build_metadata_.empty()) {
      it = std::copy_backward(build_metadata_.begin(), build_metadata_.end(), it);
      *(--it) = '+';
    }

    if (!prerelease_tag_.empty()) {
      it = std::copy_backward(prerelease_tag_.begin(), prerelease_tag_.end(), it);
      *(--it) = '-';
    }

    it = detail::to_chars(it, patch_);
    *(--it) = '.';

    it = detail::to_chars(it, minor_);
    *(--it) = '.';

    it = detail::to_chars(it, major_);

    return result;
  }

  struct from_chars_result {
    const char* ptr;
    std::errc ec;

    [[nodiscard]] constexpr operator bool() const noexcept { return ec == std::errc{}; }
  };

  enum class version_compare_option : std::uint8_t {
    exclude_prerelease,
    include_prerelease
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

constexpr int compare(std::string_view lhs, std::string_view rhs) noexcept {
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

    return lhs.size() < rhs.size() ? -1 : (lhs.size() > rhs.size() ? 1 : 0);
  } else {
    return lhs.compare(rhs);
  }
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
  logical_or
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
  SEMVER_CONSTEXPR token_stream() { tokens.reserve(32); }

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
    token token;
    return advance_if_match(token, type);
  }

  SEMVER_CONSTEXPR bool check(token_type type) const noexcept {
    return peek().type == type;
  }

private:
  std::size_t current = 0;
  std::vector<token> tokens;

  SEMVER_CONSTEXPR token get(std::size_t i) const noexcept {
    return tokens[i];
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
      add_token(stream, token_type::range_operator, advance_if_match('=') ? range_operator::less_or_equal : range_operator::less);
      break;
    case '>':
      add_token(stream, token_type::range_operator, advance_if_match('=') ? range_operator::greater_or_equal : range_operator::greater);
      break;
    case '=':
      add_token(stream, token_type::range_operator, range_operator::equal);
      break;
    default:
      if (is_digit(c)) {
        add_token(stream, token_type::digit, to_digit(c));
        break;
      }
      else if (is_letter(c)) {
        add_token(stream, token_type::letter, c);
        break;
      }
      return failure(get_prev_symbol());
    }

    return success(get_prev_symbol());
  }

  SEMVER_CONSTEXPR void add_token(token_stream& stream, token_type type) {
    stream.push({ type, {}, get_prev_symbol() });
  }

  SEMVER_CONSTEXPR void add_token(token_stream& stream, token_type type, std::uint8_t digit) {
    token t{}; t.type = type; t.value.digit = digit; t.lexeme = get_prev_symbol();
    stream.push(t);
  }

  SEMVER_CONSTEXPR void add_token(token_stream& stream, token_type type, char letter) {
    token t{}; t.type = type; t.value.letter = letter; t.lexeme = get_prev_symbol();
    stream.push(t);
  }

  SEMVER_CONSTEXPR void add_token(token_stream& stream, token_type type, range_operator op) {
    token t{}; t.type = type; t.value.op = op; t.lexeme = get_prev_symbol();
    stream.push(t);
  }

  SEMVER_CONSTEXPR char advance() noexcept {
    char c = text_[current_pos_];
    current_pos_ += 1;
    return c;
  }

  SEMVER_CONSTEXPR bool advance_if_match(char c) noexcept {
    if (is_eol()) {
      return false;
    }
    if (text_[current_pos_] != c) {
      return false;
    }
    current_pos_ += 1;
    return true;
  }

  SEMVER_CONSTEXPR const char* get_prev_symbol() const noexcept {
    return text_.data() + current_pos_ - 1;
  }

  SEMVER_CONSTEXPR bool is_eol() const noexcept { return current_pos_ >= text_.size(); }
};

class prerelease_comparator {
public:
  template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
  [[nodiscard]] static SEMVER_CONSTEXPR int compare(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
    const auto lhs_tag = lhs.prerelease_tag();
    const auto rhs_tag = rhs.prerelease_tag();

    if (lhs_tag.empty() != rhs_tag.empty()) {
      return lhs_tag.empty() ? 1 : -1;
    }

    if (lhs_tag.empty()) {
      return 0;
    }

    return compare_tags(lhs_tag, rhs_tag);
  }

private:
  [[nodiscard]] static constexpr bool is_numeric_identifier(std::string_view id) noexcept {
    for (char c : id) {
      if (!is_digit(c)) return false;
    }
    return !id.empty();
  }

  [[nodiscard]] static constexpr std::string_view next_identifier(std::string_view& tag) noexcept {
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

  [[nodiscard]] static SEMVER_CONSTEXPR int compare_tags(std::string_view lhs, std::string_view rhs) noexcept {
    while (!lhs.empty() && !rhs.empty()) {
      const auto lhs_id = next_identifier(lhs);
      const auto rhs_id = next_identifier(rhs);

      const bool lhs_numeric = is_numeric_identifier(lhs_id);
      const bool rhs_numeric = is_numeric_identifier(rhs_id);

      int cmp = 0;
      if (lhs_numeric && rhs_numeric) {
        cmp = compare_numerically(lhs_id, rhs_id);
      } else if (!lhs_numeric && !rhs_numeric) {
        cmp = detail::compare(lhs_id, rhs_id);
      } else {
        return lhs_numeric ? -1 : 1;
      }

      if (cmp != 0) return cmp;
    }

    if (lhs.empty() && rhs.empty()) return 0;
    return lhs.empty() ? -1 : 1;
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
      out = static_cast<Int>(result);
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

  SEMVER_CONSTEXPR from_chars_result parse_prerelease_tag(std::string& out) {
    out.clear();
    out.reserve(16);

    do {
      if (!out.empty()) out.push_back('.');
      const auto id_start = out.size();

      if (const auto res = parse_prerelease_identifier(out); !res) {
        if (id_start > 0) out.resize(id_start - 1); // roll back '.'
        return res;
      }

    } while (stream.advance_if_match(token_type::dot));

    return success(stream.peek().lexeme);
  }

  SEMVER_CONSTEXPR from_chars_result parse_build_metadata(std::string& out) {
    out.clear();

    do {
      if (!out.empty()) out.push_back('.');
      const auto id_start = out.size();

      if (const auto res = parse_build_identifier(out); !res) {
        if (id_start > 0) out.resize(id_start - 1); // roll back '.'
        return res;
      }

    } while (stream.advance_if_match(token_type::dot));

    return success(stream.peek().lexeme);
  }

  // Unified dot-separated identifier parser.
  // CheckLeadingZeros=true: prerelease identifiers obey spec §9 (no leading zeros in numeric IDs).
  // CheckLeadingZeros=false: build-metadata identifiers have no such restriction.
  template <bool CheckLeadingZeros>
  SEMVER_CONSTEXPR from_chars_result parse_identifier(std::string& out) {
    const auto start = out.size(); // append mode: track where this identifier begins
    token tok = stream.advance();

    do {
      switch (tok.type) {
      case token_type::hyphen:
        out.push_back('-');
        break;
      case token_type::letter:
        out.push_back(tok.value.letter);
        break;
      case token_type::digit: {
        const auto digit = tok.value.digit;
        if constexpr (CheckLeadingZeros) {
          // Purely numeric identifiers must not have leading zeros (spec §9).
          // "1.2.3-01.alpha" → invalid; "1.2.3-01b" → valid (alphanumeric).
          if (out.size() == start && is_leading_zero(digit)) {
            out.resize(start);
            return failure(tok.lexeme);
          }
        }
        out.push_back(to_char(digit));
        break;
      }
      default:
        out.resize(start);
        return failure(tok.lexeme);
      }
    } while (stream.advance_if_match(tok, token_type::hyphen)
          || stream.advance_if_match(tok, token_type::letter)
          || stream.advance_if_match(tok, token_type::digit));

    return success(stream.peek().lexeme);
  }

  SEMVER_CONSTEXPR from_chars_result parse_prerelease_identifier(std::string& out) {
    return parse_identifier<true>(out);
  }

  SEMVER_CONSTEXPR from_chars_result parse_build_identifier(std::string& out) {
    return parse_identifier<false>(out);
  }

  SEMVER_CONSTEXPR bool is_leading_zero(int digit) noexcept {
    if (digit != 0) return false;
    // '0' is a leading zero in a purely-numeric identifier.
    // Scan ahead: if any following alphanumeric char is a letter or hyphen,
    // the identifier is alphanumeric and leading-zero rules do not apply.
    // If all following alphanumeric chars are digits (and there's at least one),
    // the identifier is purely numeric with a leading zero — reject it.
    for (std::size_t k = 0; ; ++k) {
      const token t = stream.peek(k);
      if (!is_alphanumeric(t)) break;        // end of identifier
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
  return prerelease_comparator::compare(lhs, rhs);
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
SEMVER_CONSTEXPR from_chars_result parse_version(std::string_view str, version<I1, I2, I3>& out) {
  if (str.size() > SEMVER_MAX_INPUT_LENGTH) {
    return failure(str.data(), std::errc::value_too_large);
  }

  token_stream token_stream;
  from_chars_result result = lexer{ str }.scan_tokens(token_stream);
  if (!result) {
    return result;
  }

  result = version_parser{ token_stream }.parse(out);
  if (!result) {
    return result;
  }

  if (!token_stream.advance_if_match(token_type::eol)) {
    return failure(token_stream.peek().lexeme);
  }

  return success(token_stream.previous().lexeme);
}

} // namespace semver::detail

template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR bool operator==(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  return detail::compare_parsed(lhs, rhs, version_compare_option::include_prerelease) == 0;
}

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

#if __cpp_impl_three_way_comparison >= 201907L
template <typename L1, typename L2, typename L3, typename R1, typename R2, typename R3>
[[nodiscard]] SEMVER_CONSTEXPR std::weak_ordering operator<=>(const version<L1, L2, L3>& lhs, const version<R1, R2, R3>& rhs) noexcept {
  const int cmp = detail::compare_parsed(lhs, rhs, version_compare_option::include_prerelease);
  if (cmp == 0) return std::weak_ordering::equivalent;
  if (cmp > 0)  return std::weak_ordering::greater;
  return std::weak_ordering::less;
}
#endif

template <typename I1, typename I2, typename I3>
[[nodiscard]] SEMVER_CONSTEXPR from_chars_result parse(std::string_view str, version<I1, I2, I3>& output) {
  return detail::parse_version(str, output);
}

template <typename I1 = int, typename I2 = I1, typename I3 = I1>
[[nodiscard]] SEMVER_CONSTEXPR bool valid(std::string_view str) {
  version<I1, I2, I3> v{};
  return detail::parse_version(str, v);
}

template <typename OStream, typename I1, typename I2, typename I3,
          typename = decltype(std::declval<OStream&>() << std::declval<std::string>())>
OStream& operator<<(OStream& os, const version<I1, I2, I3>& v) {
  os << v.to_string();
  return os;
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
      }
      return false;
    }

    SEMVER_CONSTEXPR const version<I1, I2, I3>& get_version() const noexcept { return v; }

  private:
    version<I1, I2, I3> v;
    range_operator op;
  };

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

#if __cpp_concepts >= 201907L
template <std::integral I1 = int, std::integral I2 = I1, std::integral I3 = I1>
#else
template <typename I1 = int, typename I2 = I1, typename I3 = I1>
#endif
class range_set {
  static_assert(std::is_integral_v<I1>, "semver: I1 must be an integral type");
  static_assert(std::is_integral_v<I2>, "semver: I2 must be an integral type");
  static_assert(std::is_integral_v<I3>, "semver: I3 must be an integral type");

public:
  friend class detail::range_parser;

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
        detail::range<I1, I2, I3> range;
        if (const auto res = parse_range(range); !res) {
          return res;
        }

        out.ranges.push_back(std::move(range));
        skip_whitespaces();

      } while (stream.advance_if_match(token_type::logical_or));

      return success(stream.peek().lexeme);
    }
    
  private:
    token_stream& stream;

    template <typename I1, typename I2, typename I3>
    SEMVER_CONSTEXPR from_chars_result parse_range(detail::range<I1, I2, I3>& out) {
      do {
        skip_whitespaces();

        if (const auto res = parse_range_comparator(out.ranges_comparators); !res) {
          return res;
        }

        skip_whitespaces();

      } while (stream.check(token_type::range_operator) || stream.check(token_type::digit));
      
      return success(stream.peek().lexeme);
    }

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

  detail::token_stream token_stream;
  const from_chars_result result = detail::lexer{str}.scan_tokens(token_stream);
  if (!result) {
    return result;
  }

  const from_chars_result parse_result = detail::range_parser{ token_stream }.parse(out);
  if (!parse_result) {
    return parse_result;
  }

  if (!token_stream.advance_if_match(detail::token_type::eol)) {
    return detail::failure(token_stream.peek().lexeme);
  }

  return detail::success(token_stream.previous().lexeme);
}

#if __cpp_consteval >= 201811L && __cpp_lib_constexpr_string >= 201907L
namespace literals {
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

#undef SEMVER_CONSTEXPR

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

namespace std {
  template <typename I1, typename I2, typename I3>
  struct hash<semver::version<I1, I2, I3>> {
    std::size_t operator()(const semver::version<I1, I2, I3>& v) const noexcept {
      // build_metadata excluded per spec §10 (ignored in ==).
      auto hash_combine = [](std::size_t seed, std::size_t value) noexcept -> std::size_t {
        return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
      };
      std::size_t h = std::hash<I1>{}(v.major());
      h = hash_combine(h, std::hash<I2>{}(v.minor()));
      h = hash_combine(h, std::hash<I3>{}(v.patch()));
      h = hash_combine(h, std::hash<std::string_view>{}(v.prerelease_tag()));
      return h;
    }
  };
} // namespace std

#endif // NEARGYE_SEMANTIC_VERSIONING_HPP
