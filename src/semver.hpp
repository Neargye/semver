// semver c++11 https://github.com/Neargye/semver
// Vesion 0.1.1
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
#include <string>

namespace semver {

constexpr std::size_t kVersionStringLength = 27;

#pragma pack(push, 1)
struct Version {
  enum class PreReleaseType : std::uint8_t {
    kAlpha = 0,
    kBetha = 1,
    kReleaseCandidate = 2,
    kNone = 3,
  };

  std::uint16_t major;
  std::uint16_t minor;
  std::uint16_t patch;
  PreReleaseType pre_release_type;
  std::uint8_t pre_release_version;

  constexpr Version(const std::uint16_t major,
                    const std::uint16_t minor,
                    const std::uint16_t patch,
                    const PreReleaseType pre_release_type = PreReleaseType::kNone,
                    const std::uint8_t pre_release_version = static_cast<std::uint8_t>(0));

  constexpr Version();

  constexpr Version(const Version& v) = default;

  constexpr Version(Version&& v) = default;

  Version(const std::string& s);

  Version(const char* s);

  void ToString(char* s, const std::size_t length = kVersionStringLength) const;

  std::string ToString() const;

  void FromString(const char* s);

  void FromString(const std::string& s);

  constexpr friend bool operator==(const Version& v1, const Version& v2);

  constexpr friend bool operator!=(const Version& v1, const Version& v2);

  constexpr friend bool operator>(const Version& v1, const Version& v2);

  constexpr friend bool operator>=(const Version& v1, const Version& v2);

  constexpr friend bool operator<(const Version& v1, const Version& v2);

  constexpr friend bool operator<=(const Version& v1, const Version& v2);
};
#pragma pack(pop)

inline std::size_t ToString(const Version& v, char* s, const std::size_t length = kVersionStringLength);

inline std::string ToString(const Version& v);

inline void FromString(Version* v, const char* s);

inline void FromString(Version* v, const std::string& s);

inline constexpr Version::Version(const std::uint16_t major,
                                  const std::uint16_t minor,
                                  const std::uint16_t patch,
                                  const PreReleaseType pre_release_type,
                                  const std::uint8_t pre_release_version)
    : major{major},
      minor{minor},
      patch{patch},
      pre_release_type{pre_release_type},
      pre_release_version{pre_release_type == PreReleaseType::kNone ? static_cast<std::uint8_t>(0) : pre_release_version} {}

inline constexpr Version::Version() : Version(0, 1, 0) {
  // https://semver.org/#how-should-i-deal-with-revisions-in-the-0yz-initial-development-phase
}

inline Version::Version(const std::string& s) : Version(0, 0, 0) {
  semver::FromString(this, s);
}

inline Version::Version(const char* s) : Version(0, 0, 0) {
  semver::FromString(this, s);
}

inline void Version::ToString(char* s, const std::size_t length) const {
  semver::ToString(*this, s, length);
}

inline std::string Version::ToString() const {
  return semver::ToString(*this);
}

inline void Version::FromString(const char* s) {
  semver::FromString(this, s);
}

inline void Version::FromString(const std::string& s) {
  semver::FromString(this, s);
}

inline constexpr bool operator==(const Version& v1, const Version& v2) {
  return v1.major == v2.major && v1.minor == v2.minor && v1.patch == v2.patch &&
         v1.pre_release_type == v2.pre_release_type &&
         v1.pre_release_version == v2.pre_release_version;
}

inline constexpr bool operator!=(const Version& v1, const Version& v2) {
  return !(v1 == v2);
}

inline constexpr bool operator>(const Version& v1, const Version& v2) {
  // https://semver.org/#spec-item-11
  return (v1.major != v2.major)
           ? v1.major > v2.major
           : (v1.minor != v2.minor)
                 ? (v1.minor > v2.minor)
                 : (v1.patch != v2.patch)
                       ? (v1.patch > v2.patch)
                       : (v1.pre_release_type != v2.pre_release_type)
                             ? (v1.pre_release_type > v2.pre_release_type)
                             : (v1.pre_release_version > v2.pre_release_version);
}

inline constexpr bool operator>=(const Version& v1, const Version& v2) {
  return v1 == v2 || v1 > v2;
}

inline constexpr bool operator<(const Version& v1, const Version& v2) {
  return !(v1 >= v2);
}

inline constexpr bool operator<=(const Version& v1, const Version& v2) {
  return v1 == v2 || v1 < v2;
}

inline std::size_t ToString(const Version& v, char* s, const std::size_t length) {
  std::size_t size = 0;

  switch (v.pre_release_type) {
    case Version::PreReleaseType::kAlpha: {
      if (v.pre_release_version == 0) {
        size = std::snprintf(s, length,
                             "%" PRIu16 ".%" PRIu16 ".%" PRIu16 "-alpha",
                             v.major, v.minor, v.patch);
      } else {
        size = std::snprintf(s, length,
                             "%" PRIu16 ".%" PRIu16 ".%" PRIu16 "-alpha.%" PRIu8,
                             v.major, v.minor, v.patch, v.pre_release_version);
      }
      break;
    }
    case Version::PreReleaseType::kBetha: {
      if (v.pre_release_version == 0) {
        size = std::snprintf(s, length,
                             "%" PRIu16 ".%" PRIu16 ".%" PRIu16 "-betha",
                             v.major, v.minor, v.patch);
      } else {
        size = std::snprintf(s, length,
                             "%" PRIu16 ".%" PRIu16 ".%" PRIu16 "-betha.%" PRIu8,
                             v.major, v.minor, v.patch, v.pre_release_version);
      }
      break;
    }
    case Version::PreReleaseType::kReleaseCandidate: {
      if (v.pre_release_version == 0) {
        size =
            std::snprintf(s, length,
                          "%" PRIu16 ".%" PRIu16 ".%" PRIu16 "-rc",
                          v.major, v.minor, v.patch);
      } else {
        size = std::snprintf(s, length,
                             "%" PRIu16 ".%" PRIu16 ".%" PRIu16 "-rc.%" PRIu8,
                             v.major, v.minor, v.patch, v.pre_release_version);
      }
      break;
    }
    case Version::PreReleaseType::kNone: {
      size = std::snprintf(s, length,
                           "%" PRIu16 ".%" PRIu16 ".%" PRIu16,
                           v.major, v.minor, v.patch);
      break;
    }
    default:
      break;
  }

  return size;
}

inline std::string ToString(const Version& v) {
  std::string s(kVersionStringLength, 0);
  const std::size_t size = ToString(v, &s[0], s.length());
  s.resize(size);
  s.shrink_to_fit();
  return s;
}

inline void FromString(Version* v, const char* s) {
  char pre_release_type_str[6] = {0};
  std::sscanf(s, "%" SCNu16 ".%" SCNu16 ".%" SCNu16 "-%[^0-9]%" SCNu8,
              &v->major, &v->minor, &v->patch, pre_release_type_str,
              &v->pre_release_version);

  if (std::strncmp(pre_release_type_str, "alpha", 5) == 0) {
    v->pre_release_type = Version::PreReleaseType::kAlpha;
  } else if (std::strncmp(pre_release_type_str, "betha", 5) == 0) {
    v->pre_release_type = Version::PreReleaseType::kBetha;
  } else if (std::strncmp(pre_release_type_str, "rc", 2) == 0) {
    v->pre_release_type = Version::PreReleaseType::kReleaseCandidate;
  } else {
    v->pre_release_type = Version::PreReleaseType::kNone;
    v->pre_release_version = 0;
  }
}

inline void FromString(Version* v, const std::string& s) {
  FromString(v, s.c_str());
}
}
