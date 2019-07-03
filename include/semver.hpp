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
// vesion 0.1.8                               |___/
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// Copyright (c) 2018 Daniil Goncharov <neargye@gmail.com>.
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

#include <cstdint>
#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>

// Allow to disable exceptions.
#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)) && !defined(SEMVER_NOEXCEPTION)
#  include <stdexcept>
#  define SEMVER_THROW(exception) throw exception
#else
#  include <cstdlib>
#  define SEMVER_THROW(exception) std::abort()
#endif

namespace semver {

enum class prerelease : std::uint8_t {
  alpha = 0,
  beta = 1,
  rc = 2,
  none = 3
};

namespace detail {

constexpr char char_to_lower(char c) {
  return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

constexpr bool is_digit(char c) {
  return c >= '0' && c <= '9';
}

constexpr std::uint8_t char_to_digit(char c) {
  return c - '0';
}

constexpr bool read_uint(std::string_view str, std::size_t& i, std::uint8_t& d) {
  if (d = 0; is_digit(str[i])) {
    for (std::size_t p = 0; (p < 3) && (i + p < str.length()); ++p) {
      if (detail::is_digit(str[p + i])) {
        d = d * 10 + detail::char_to_digit(str[p + i]);
      } else {
        i += p;
        return true;
      }
    }
    i += 3;
    return true;
  } else {
    return false;
  }
}

constexpr bool read_dot(std::string_view str, std::size_t& i) {
  if (str[i] == '.') {
    ++i;
    return true;
  } else {
    return false;
  }
}

constexpr bool read_prerelease(std::string_view str, std::size_t& i, prerelease& p) {
  constexpr std::string_view alpha{"-alpha"};
  constexpr std::string_view beta{"-beta"};
  constexpr std::string_view rc{"-rc"};

  if (str.compare(i, alpha.length(), alpha) == 0) {
    i += alpha.length();
    p = prerelease::alpha;
    return true;
  } else if (str.compare(i, beta.length(), beta) == 0) {
    i += beta.length();
    p = prerelease::beta;
    return true;
  } else if (str.compare(i, rc.length(), rc) == 0) {
    i += rc.length();
    p = prerelease::rc;
    return true;
  } else {
    return false;
  }
}

} // namespace semver::detail

struct alignas(1) version final {
  std::uint8_t major;
  std::uint8_t minor;
  std::uint8_t patch;
  prerelease prerelease_type;
  std::uint8_t pre_release_version;

  constexpr version(std::uint8_t major,
                    std::uint8_t minor,
                    std::uint8_t patch,
                    prerelease prerelease_type = prerelease::none,
                    std::uint8_t pre_release_version = 0) noexcept
      : major{major},
        minor{minor},
        patch{patch},
        prerelease_type{prerelease_type},
        pre_release_version{prerelease_type == prerelease::none ? static_cast<std::uint8_t>(0) : pre_release_version} {
  }

  constexpr version(std::string_view str): version() {
    from_string(str);
  }

  constexpr version() noexcept : version(0, 1, 0) {
    // https://semver.org/#how-should-i-deal-with-revisions-in-the-0yz-initial-development-phase
  }

  constexpr version(const version&) = default;

  constexpr version(version&&) = default;

  ~version() = default;

  version& operator=(const version&) = default;

  version& operator=(version&&) = default;

  std::string to_string() const {
    auto str = std::to_string(major)
                  .append(".")
                  .append(std::to_string(minor))
                  .append(".")
                  .append(std::to_string(patch));

    switch (prerelease_type) {
      case prerelease::alpha:
        str.append("-alpha");
        break;

      case prerelease::beta:
        str.append("-beta");
        break;

      case prerelease::rc:
        str.append("-rc");
        break;

      case prerelease::none:
        return str;

      default:
        SEMVER_THROW(std::invalid_argument{"invalid version"});
    }

    if (pre_release_version > 0) {
      str.append(".").append(std::to_string(pre_release_version));
    }

    return str;
  }

  constexpr void from_string(std::string_view str) {
    if (std::size_t i = 0;
        detail::read_uint(str, i, major) && detail::read_dot(str, i) &&
        detail::read_uint(str, i, minor) && detail::read_dot(str, i) &&
        detail::read_uint(str, i, patch)) {
      if (str.length() != i) {
        if (detail::read_prerelease(str, i, prerelease_type)) {
          if (str.length() != i) {
            if (detail::read_dot(str, i) && detail::read_uint(str, i, pre_release_version)) {
              if (str.length() != i) {
                SEMVER_THROW(std::invalid_argument{"invalid version"});
              }
            } else {
              SEMVER_THROW(std::invalid_argument{"invalid version"});
            }
          }
        } else {
          SEMVER_THROW(std::invalid_argument{"invalid version"});
        }
      }
    } else {
      SEMVER_THROW(std::invalid_argument{"invalid version"});
    }
  }

  constexpr int compare(const version& other) const noexcept {
    if (major == other.major) {
      if (minor == other.minor) {
        if (patch == other.patch) {
          if (prerelease_type == other.prerelease_type) {
            return pre_release_version - other.pre_release_version;
          } else {
            return static_cast<std::uint8_t>(prerelease_type) - static_cast<std::uint8_t>(other.prerelease_type);
          }
        } else {
          return patch - other.patch;
        }
      } else {
        return minor - other.minor;
      }
    } else {
      return major - other.major;
    }
  }
};

constexpr bool operator==(const version& lhs, const version& rhs) noexcept {
  return lhs.compare(rhs) == 0;
}

constexpr bool operator!=(const version& lhs, const version& rhs) noexcept {
  return lhs.compare(rhs) != 0;
}

constexpr bool operator>(const version& lhs, const version& rhs) noexcept {
  return lhs.compare(rhs) > 0;
}

constexpr bool operator>=(const version& lhs, const version& rhs) noexcept {
  return lhs.compare(rhs) >= 0;
}

constexpr bool operator<(const version& lhs, const version& rhs) noexcept {
  return lhs.compare(rhs) < 0;
}

constexpr bool operator<=(const version& lhs, const version& rhs) noexcept {
  return lhs.compare(rhs) <= 0;
}

inline std::ostream& operator<<(std::ostream& os, const version& v) {
  os << v.to_string();
  return os;
}

inline version operator"" _version(const char* str, std::size_t) {
  return version{str};
}

std::string to_string(const version& v) {
  return v.to_string();
}

version from_string(std::string_view str) {
  return version{str};
}

} // namespace semver

#endif // NEARGYE_SEMANTIC_VERSIONING_HPP
