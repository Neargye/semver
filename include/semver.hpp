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
// vesion 0.1.5                               |___/
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

#pragma once

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4996)
#endif

#include <cstdint>
#include <cstddef>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <array>
#include <string>
#include <ostream>
#include <istream>

namespace semver {

struct alignas(1) Version final {
  static constexpr const std::size_t kVersionStringLength = 22;
  // 3(<major>) + 1(.) + 3(<minor>) + 1(.) + 3(<patch>) +
  // 1(-) + 5(<prerelease>) + 1(.) + 3(<prereleaseversion>) + 1('\0') = 22

  enum class PreReleaseType : std::int8_t {
    kAlpha = 0,
    kBetha = 1,
    kReleaseCandidate = 2,
    kNone = 3,
  };

  std::uint8_t major;
  std::uint8_t minor;
  std::uint8_t patch;
  PreReleaseType pre_release_type;
  std::uint8_t pre_release_version;

  constexpr Version(std::uint8_t major,
                    std::uint8_t minor,
                    std::uint8_t patch,
                    PreReleaseType pre_release_type = PreReleaseType::kNone,
                    std::uint8_t pre_release_version = static_cast<std::uint8_t>(0)) noexcept;

  constexpr Version() noexcept;

  constexpr Version(const Version&) = default;

  constexpr Version(Version&&) = default;

  explicit Version(const std::string& str) noexcept;

  explicit Version(const char* str) noexcept;

  ~Version() = default;

  Version& operator=(const Version&) = default;

  Version& operator=(Version&&) = default;

  constexpr bool IsValid() const noexcept;

  std::size_t ToString(char* str, std::size_t length = kVersionStringLength) const noexcept;

  std::string ToString() const noexcept;

  bool FromString(const char* str) noexcept;

  bool FromString(const std::string& str) noexcept;
};

std::size_t ToString(const Version& v, char* str, std::size_t length = Version::kVersionStringLength) noexcept;

std::string ToString(const Version& v) noexcept;

Version FromString(const char* str) noexcept;

Version FromString(const std::string& str) noexcept;

constexpr bool operator==(const Version& lhs, const Version& rhs) noexcept;

constexpr bool operator!=(const Version& lhs, const Version& rhs) noexcept;

constexpr bool operator>(const Version& lhs, const Version& rhs) noexcept;

constexpr bool operator>=(const Version& lhs, const Version& rhs) noexcept;

constexpr bool operator<(const Version& lhs, const Version& rhs) noexcept;

constexpr bool operator<=(const Version& lhs, const Version& rhs) noexcept;

std::ostream& operator<<(std::ostream& os, const Version& v) noexcept;

std::istream& operator>>(std::istream& is, Version& v) noexcept;

Version operator"" _version(const char* str, std::size_t length) noexcept;

namespace detail {

constexpr bool IsEquals(const Version&, const Version&) noexcept;

constexpr bool IsLess(const Version&, const Version&) noexcept;

constexpr bool IsGreater(const Version&, const Version&) noexcept;

} // namespace detail

} // namespace semver

namespace semver {

namespace detail {

inline constexpr bool IsEquals(const Version& lhs, const Version& rhs) noexcept {
  return (lhs.major == rhs.major) &&
         (lhs.minor == rhs.minor) &&
         (lhs.patch == rhs.patch) &&
         (lhs.pre_release_type == rhs.pre_release_type) &&
         (lhs.pre_release_version == rhs.pre_release_version);
}

inline constexpr bool IsLess(const Version& lhs, const Version& rhs) noexcept {
  // https://semver.org/#spec-item-11
  return (lhs.major != rhs.major)
             ? (lhs.major < rhs.major)
             : (lhs.minor != rhs.minor)
                   ? (lhs.minor < rhs.minor)
                   : (lhs.patch != rhs.patch)
                         ? (lhs.patch < rhs.patch)
                         : (lhs.pre_release_type != rhs.pre_release_type)
                               ? (lhs.pre_release_type < rhs.pre_release_type)
                               : (lhs.pre_release_version < rhs.pre_release_version);
}

inline constexpr bool IsGreater(const Version& lhs, const Version& rhs) noexcept {
  // https://semver.org/#spec-item-11
  return (lhs.major != rhs.major)
             ? (lhs.major > rhs.major)
             : (lhs.minor != rhs.minor)
                   ? (lhs.minor > rhs.minor)
                   : (lhs.patch != rhs.patch)
                         ? (lhs.patch > rhs.patch)
                         : (lhs.pre_release_type != rhs.pre_release_type)
                               ? (lhs.pre_release_type > rhs.pre_release_type)
                               : (lhs.pre_release_version > rhs.pre_release_version);
}

} // namespace detail

inline constexpr Version::Version(std::uint8_t major,
                                  std::uint8_t minor,
                                  std::uint8_t patch,
                                  PreReleaseType pre_release_type,
                                  std::uint8_t pre_release_version) noexcept
    : major{major},
      minor{minor},
      patch{patch},
      pre_release_type{pre_release_type},
      pre_release_version{(pre_release_type == PreReleaseType::kNone)
                              ? static_cast<std::uint8_t>(0)
                              : pre_release_version} {}

inline constexpr Version::Version() noexcept : Version(0, 1, 0) {
  // https://semver.org/#how-should-i-deal-with-revisions-in-the-0yz-initial-development-phase
}

inline Version::Version(const std::string& str) noexcept : Version(0, 0, 0) {
  (*this) = semver::FromString(str);
}

inline Version::Version(const char* str) noexcept : Version(0, 0, 0) {
  (*this) = semver::FromString(str);
}

inline constexpr bool Version::IsValid() const noexcept {
  return ((pre_release_type >= PreReleaseType::kAlpha) && (pre_release_type <= PreReleaseType::kNone));
}

inline std::size_t Version::ToString(char* str, std::size_t length) const noexcept {
  return semver::ToString(*this, str, length);
}

inline std::string Version::ToString() const noexcept {
  return semver::ToString(*this);
}

inline bool Version::FromString(const char* str) noexcept {
  return ((*this) = semver::FromString(str)).IsValid();
}

inline bool Version::FromString(const std::string& str) noexcept {
  return ((*this) = semver::FromString(str)).IsValid();
}

inline constexpr bool operator==(const Version& lhs, const Version& rhs) noexcept {
  return (lhs.IsValid() && rhs.IsValid() && detail::IsEquals(lhs, rhs));
}

inline constexpr bool operator!=(const Version& lhs, const Version& rhs) noexcept {
  return !(lhs == rhs);
}

inline constexpr bool operator>(const Version& lhs, const Version& rhs) noexcept {
  return (lhs.IsValid() && rhs.IsValid() && detail::IsGreater(lhs, rhs));
}

inline constexpr bool operator>=(const Version& lhs, const Version& rhs) noexcept {
  return ((lhs.IsValid() && rhs.IsValid()) && (detail::IsEquals(lhs, rhs) || detail::IsGreater(lhs, rhs)));
}

inline constexpr bool operator<(const Version& lhs, const Version& rhs) noexcept {
  return (lhs.IsValid() && rhs.IsValid() && detail::IsLess(lhs, rhs));
}

inline constexpr bool operator<=(const Version& lhs, const Version& rhs) noexcept {
  return ((lhs.IsValid() && rhs.IsValid()) && (detail::IsEquals(lhs, rhs) || detail::IsLess(lhs, rhs)));
}

inline std::ostream& operator<<(std::ostream& os, const Version& v) noexcept {
  std::array<char, Version::kVersionStringLength> version = {{'\0'}};
  v.ToString(version.data());
  os << version.data();
  return os;
}

inline std::istream& operator>>(std::istream& is, Version& v) noexcept {
  std::array<char, Version::kVersionStringLength> version = {{'\0'}};
  is >> version.data();
  v.FromString(version.data());
  return is;
}

inline std::size_t ToString(const Version& v, char* str, std::size_t length) noexcept {
  if (str == nullptr || !v.IsValid()) {
    return 0;
  }

  int size = 0;

  switch (v.pre_release_type) {
    case Version::PreReleaseType::kAlpha: {
      if (v.pre_release_version == 0) {
        size = std::snprintf(str, length, "%" PRIu8 ".%" PRIu8 ".%" PRIu8 "-alpha",
                             v.major, v.minor, v.patch);
      } else {
        size = std::snprintf(str, length, "%" PRIu8 ".%" PRIu8 ".%" PRIu8 "-alpha.%" PRIu8,
                             v.major, v.minor, v.patch, v.pre_release_version);
      }
      break;
    }
    case Version::PreReleaseType::kBetha: {
      if (v.pre_release_version == 0) {
        size = std::snprintf(str, length, "%" PRIu8 ".%" PRIu8 ".%" PRIu8 "-betha",
                             v.major, v.minor, v.patch);
      } else {
        size = std::snprintf(str, length, "%" PRIu8 ".%" PRIu8 ".%" PRIu8 "-betha.%" PRIu8,
                             v.major, v.minor, v.patch, v.pre_release_version);
      }
      break;
    }
    case Version::PreReleaseType::kReleaseCandidate: {
      if (v.pre_release_version == 0) {
        size = std::snprintf(str, length, "%" PRIu8 ".%" PRIu8 ".%" PRIu8 "-rc",
                             v.major, v.minor, v.patch);
      } else {
        size = std::snprintf(str, length, "%" PRIu8 ".%" PRIu8 ".%" PRIu8 "-rc.%" PRIu8,
                             v.major, v.minor, v.patch, v.pre_release_version);
      }
      break;
    }
    case Version::PreReleaseType::kNone: {
      size = std::snprintf(str, length, "%" PRIu8 ".%" PRIu8 ".%" PRIu8,
                           v.major, v.minor, v.patch);
      break;
    }
    default:
      break;
  }

  if (size > 0) {
    return static_cast<std::size_t>(size);
  }

  return 0;
}

inline std::string ToString(const Version& v) noexcept {
  std::string str(Version::kVersionStringLength, '\0');
  const auto size = ToString(v, &str[0], str.length());
  if (size > 0) {
    str.resize(size);
    str.shrink_to_fit();
    return str;
  }

  return std::string{};
}

inline Version FromString(const char* str) noexcept {
  Version v{0, 0, 0, static_cast<Version::PreReleaseType>(-1), 0};
  if (str == nullptr) {
    return v;
  }
  std::array<char, 8> prerelease = {{'\0'}};
  const auto num = std::sscanf(str, "%" SCNu8 ".%" SCNu8 ".%" SCNu8 "%7[^0-9]%" SCNu8,
                               &(v.major), &(v.minor), &(v.patch),
                               prerelease.data(), &(v.pre_release_version));

  if (num > 3 && num <= 5) {
    if (std::strcmp(prerelease.data(), "-alpha.") == 0) {
      v.pre_release_type = Version::PreReleaseType::kAlpha;
    } else if (std::strcmp(prerelease.data(), "-alpha") == 0) {
      v.pre_release_type = Version::PreReleaseType::kAlpha;
      v.pre_release_version = 0;
    } else if (std::strcmp(prerelease.data(), "-betha.") == 0) {
      v.pre_release_type = Version::PreReleaseType::kBetha;
    } else if (std::strcmp(prerelease.data(), "-betha") == 0) {
      v.pre_release_type = Version::PreReleaseType::kBetha;
      v.pre_release_version = 0;
    } else if (std::strcmp(prerelease.data(), "-rc.") == 0) {
      v.pre_release_type = Version::PreReleaseType::kReleaseCandidate;
    } else if (std::strcmp(prerelease.data(), "-rc") == 0) {
      v.pre_release_type = Version::PreReleaseType::kReleaseCandidate;
      v.pre_release_version = 0;
    } else {
      v.pre_release_type = static_cast<Version::PreReleaseType>(-1);
    }
  } else if (num == 3) {
      v.pre_release_type = Version::PreReleaseType::kNone;
      v.pre_release_version = 0;
  }

  return v;
}

inline Version FromString(const std::string& str) noexcept {
  return FromString(str.c_str());
}

inline Version operator"" _version(const char* str, std::size_t length) noexcept {
  (void)length;
  return FromString(str);
}

} // namespace semver

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif
