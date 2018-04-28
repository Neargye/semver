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
// vesion 0.1.4                               |___/
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

#include <cstdint>
#include <cstddef>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <array>
#include <string>
#include <ostream>
#include <istream>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

namespace semver {

constexpr std::size_t kVersionStringLength = 22; // 3(<major>) + 1(.) + 3(<minor>) + 1(.) + 3(<patch>) +
                                                 // 1(-) + 5(<prerelease>) +
                                                 // 1(.) + 3(<prereleaseversion>) + 1('\0') = 22

#pragma pack(push, 1)
struct Version final {
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

  constexpr Version(const std::uint8_t,
                    const std::uint8_t,
                    const std::uint8_t,
                    const PreReleaseType = PreReleaseType::kNone,
                    const std::uint8_t = static_cast<std::uint8_t>(0));

  constexpr Version();

  constexpr Version(const Version&) = default;

  constexpr Version(Version&&) = default;

  explicit Version(const std::string&);

  explicit Version(const char*);

  ~Version() = default;

  Version& operator=(const Version&) = default;

  Version& operator=(Version&&) = default;

  constexpr bool IsValid() const;

  std::size_t ToString(char*, const std::size_t = kVersionStringLength) const;

  std::string ToString() const;

  bool FromString(const char*);

  bool FromString(const std::string&);

  constexpr friend bool operator==(const Version&, const Version&);

  constexpr friend bool operator!=(const Version&, const Version&);

  constexpr friend bool operator>(const Version&, const Version&);

  constexpr friend bool operator>=(const Version&, const Version&);

  constexpr friend bool operator<(const Version&, const Version&);

  constexpr friend bool operator<=(const Version&, const Version&);

  friend std::ostream& operator<<(std::ostream&, const Version&);

  friend std::istream& operator>>(std::istream&, Version&);
};
#pragma pack(pop)

std::size_t ToString(const Version&, char*, const std::size_t = kVersionStringLength);

std::string ToString(const Version&);

Version FromString(const char*);

Version FromString(const std::string&);

Version operator"" _version(const char*, const std::size_t);

namespace detail {

constexpr bool IsEquals(const Version& v1, const Version& v2);

constexpr bool IsLess(const Version& v1, const Version& v2);

constexpr bool IsGreater(const Version& v1, const Version& v2);

} // namespace detail

} // namespace semver

namespace semver {

namespace detail {

inline constexpr bool IsEquals(const Version& v1, const Version& v2) {
  return (v1.major == v2.major) &&
         (v1.minor == v2.minor) &&
         (v1.patch == v2.patch) &&
         (v1.pre_release_type == v2.pre_release_type) &&
         (v1.pre_release_version == v2.pre_release_version);
}

inline constexpr bool IsLess(const Version& v1, const Version& v2) {
  // https://semver.org/#spec-item-11
  return (v1.major != v2.major)
             ? (v1.major < v2.major)
             : (v1.minor != v2.minor)
                   ? (v1.minor < v2.minor)
                   : (v1.patch != v2.patch)
                         ? (v1.patch < v2.patch)
                         : (v1.pre_release_type != v2.pre_release_type)
                               ? (v1.pre_release_type < v2.pre_release_type)
                               : (v1.pre_release_version < v2.pre_release_version);
}

inline constexpr bool IsGreater(const Version& v1, const Version& v2) {
  // https://semver.org/#spec-item-11
  return (v1.major != v2.major)
             ? (v1.major > v2.major)
             : (v1.minor != v2.minor)
                   ? (v1.minor > v2.minor)
                   : (v1.patch != v2.patch)
                         ? (v1.patch > v2.patch)
                         : (v1.pre_release_type != v2.pre_release_type)
                               ? (v1.pre_release_type > v2.pre_release_type)
                               : (v1.pre_release_version > v2.pre_release_version);
}

} // namespace detail

inline constexpr Version::Version(const std::uint8_t major,
                                  const std::uint8_t minor,
                                  const std::uint8_t patch,
                                  const PreReleaseType pre_release_type,
                                  const std::uint8_t pre_release_version)
    : major{major},
      minor{minor},
      patch{patch},
      pre_release_type{pre_release_type},
      pre_release_version{(pre_release_type == PreReleaseType::kNone)
                              ? static_cast<std::uint8_t>(0)
                              : pre_release_version} {}

inline constexpr Version::Version() : Version(0, 1, 0) {
  // https://semver.org/#how-should-i-deal-with-revisions-in-the-0yz-initial-development-phase
}

inline Version::Version(const std::string& s) : Version(0, 0, 0) {
  (*this) = semver::FromString(s);
}

inline Version::Version(const char* s) : Version(0, 0, 0) {
  (*this) = semver::FromString(s);
}

inline constexpr bool Version::IsValid() const {
  return ((pre_release_type >= PreReleaseType::kAlpha) && (pre_release_type <= PreReleaseType::kNone));
}

inline std::size_t Version::ToString(char* s, const std::size_t length) const {
  return semver::ToString(*this, s, length);
}

inline std::string Version::ToString() const {
  return semver::ToString(*this);
}

inline bool Version::FromString(const char* s) {
  return ((*this) = semver::FromString(s)).IsValid();
}

inline bool Version::FromString(const std::string& s) {
  return ((*this) = semver::FromString(s)).IsValid();
}

inline constexpr bool operator==(const Version& v1, const Version& v2) {
  return (v1.IsValid() && v2.IsValid() && detail::IsEquals(v1, v2));
}

inline constexpr bool operator!=(const Version& v1, const Version& v2) {
  return ((v1.IsValid() && v2.IsValid()) && !detail::IsEquals(v1, v2));
}

inline constexpr bool operator>(const Version& v1, const Version& v2) {
  return (v1.IsValid() && v2.IsValid() && detail::IsGreater(v1, v2));
}

inline constexpr bool operator>=(const Version& v1, const Version& v2) {
  return ((v1.IsValid() && v2.IsValid()) && (detail::IsEquals(v1, v2) || detail::IsGreater(v1, v2)));
}

inline constexpr bool operator<(const Version& v1, const Version& v2) {
  return (v1.IsValid() && v2.IsValid() && detail::IsLess(v1, v2));
}

inline constexpr bool operator<=(const Version& v1, const Version& v2) {
  return ((v1.IsValid() && v2.IsValid()) && (detail::IsEquals(v1, v2) || detail::IsLess(v1, v2)));
}

inline std::ostream& operator<<(std::ostream& os, const Version& v) {
  std::array<char, kVersionStringLength> version = {{'\0'}};
  v.ToString(version.data());
  os << version.data();
  return os;
}

inline std::istream& operator>>(std::istream& is, Version& v) {
  std::array<char, kVersionStringLength> version = {{'\0'}};
  is >> version.data();
  v.FromString(version.data());
  return is;
}

inline std::size_t ToString(const Version& v, char* s, const std::size_t length) {
  if (s == nullptr || !v.IsValid()) {
    return 0;
  }

  int size = 0;

  switch (v.pre_release_type) {
    case Version::PreReleaseType::kAlpha: {
      if (v.pre_release_version == 0) {
        size = std::snprintf(s, length,
                             "%" PRIu8 ".%" PRIu8 ".%" PRIu8 "-alpha",
                             v.major, v.minor, v.patch);
      } else {
        size = std::snprintf(s, length,
                             "%" PRIu8 ".%" PRIu8 ".%" PRIu8 "-alpha.%" PRIu8,
                             v.major, v.minor, v.patch, v.pre_release_version);
      }
      break;
    }
    case Version::PreReleaseType::kBetha: {
      if (v.pre_release_version == 0) {
        size = std::snprintf(s, length,
                             "%" PRIu8 ".%" PRIu8 ".%" PRIu8 "-betha",
                             v.major, v.minor, v.patch);
      } else {
        size = std::snprintf(s, length,
                             "%" PRIu8 ".%" PRIu8 ".%" PRIu8 "-betha.%" PRIu8,
                             v.major, v.minor, v.patch, v.pre_release_version);
      }
      break;
    }
    case Version::PreReleaseType::kReleaseCandidate: {
      if (v.pre_release_version == 0) {
        size = std::snprintf(s, length,
                             "%" PRIu8 ".%" PRIu8 ".%" PRIu8 "-rc",
                             v.major, v.minor, v.patch);
      } else {
        size = std::snprintf(s, length,
                             "%" PRIu8 ".%" PRIu8 ".%" PRIu8 "-rc.%" PRIu8,
                             v.major, v.minor, v.patch, v.pre_release_version);
      }
      break;
    }
    case Version::PreReleaseType::kNone: {
      size = std::snprintf(s, length,
                           "%" PRIu8 ".%" PRIu8 ".%" PRIu8,
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

inline std::string ToString(const Version& v) {
  std::string s(kVersionStringLength, '\0');
  const std::size_t size = ToString(v, &s[0], s.length());
  if (size > 0) {
    s.resize(size);
    s.shrink_to_fit();
    return s;
  }

  return std::string{};
}

inline Version FromString(const char* s) {
  Version v{0, 0, 0, static_cast<Version::PreReleaseType>(-1), 0};
  if (s == nullptr) {
    return v;
  }

  std::array<char, 7> prerelease = {{'\0'}}; // 5(<prerelease>) + 1(.) + 1('\0') = 7
  const int num = std::sscanf(s, "%" SCNu8 ".%" SCNu8 ".%" SCNu8 "-%[^0-9]%" SCNu8,
                              &(v.major), &(v.minor), &(v.patch),
                              prerelease.data(), &(v.pre_release_version));

  if (num >= 3 && num <= 5) {
    if (std::strncmp(prerelease.data(), "alpha.", 6) == 0) {
      v.pre_release_type = Version::PreReleaseType::kAlpha;
    } else if (std::strncmp(prerelease.data(), "alpha", 5) == 0) {
      v.pre_release_type = Version::PreReleaseType::kAlpha;
      v.pre_release_version = 0;
    } else if (std::strncmp(prerelease.data(), "betha.", 6) == 0) {
      v.pre_release_type = Version::PreReleaseType::kBetha;
    } else if (std::strncmp(prerelease.data(), "betha", 5) == 0) {
      v.pre_release_type = Version::PreReleaseType::kBetha;
      v.pre_release_version = 0;
    } else if (std::strncmp(prerelease.data(), "rc.", 3) == 0) {
      v.pre_release_type = Version::PreReleaseType::kReleaseCandidate;
    } else if (std::strncmp(prerelease.data(), "rc", 2) == 0) {
      v.pre_release_type = Version::PreReleaseType::kReleaseCandidate;
      v.pre_release_version = 0;
    } else if (std::strncmp(prerelease.data(), "\0\0\0\0\0\0\0", 7) == 0) {
      v.pre_release_type = Version::PreReleaseType::kNone;
      v.pre_release_version = 0;
    }

    if(std::strcmp(v.ToString().c_str(), s) != 0) {
      v.pre_release_type = static_cast<Version::PreReleaseType>(-1);
    }
  }

  return v;
}

inline Version FromString(const std::string& s) {
  return FromString(s.c_str());
}

inline Version operator"" _version(const char* s, const std::size_t) {
  return FromString(s);
}

} // namespace semver

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
