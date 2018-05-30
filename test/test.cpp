// semver test
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

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <semver.hpp>

#include <cstddef>
#include <array>
#include <string>
#include <sstream>

using namespace semver;

#define STATIC_CKECK_OP_AND_REVERSE(v1, op, v2) \
  static_assert(v1 op v2, "");                  \
  static_assert(v2 op v1, "");

#define STATIC_CKECK_OP_AND_REVERSE_FALSE(v1, op, v2) \
  static_assert(v1 op v2, "");                        \
  static_assert(!(v2 op v1), "");

#define STATIC_CKECK_FALSE_OP_AND_REVERSE(v1, op, v2) \
  static_assert(!(v1 op v2), "");                    \
  static_assert(!(v2 op v1), "");

#define CKECK_OP_AND_REVERSE(v1, op, v2) \
  REQUIRE(v1 op v2);                     \
  REQUIRE(v2 op v1);

#define CKECK_OP_AND_REVERSE_FALSE(v1, op, v2) \
  REQUIRE(v1 op v2);                           \
  REQUIRE_FALSE(v2 op v1);

#define CKECK_FALSE_OP_AND_REVERSE(v1, op, v2) \
  REQUIRE_FALSE(v1 op v2);                    \
  REQUIRE_FALSE(v2 op v1);

TEST_CASE("constructors") {
  SECTION("default") {
    constexpr Version v0;
    static_assert(v0.major == 0 &&
                      v0.minor == 1 &&
                      v0.patch == 0 &&
                      v0.pre_release_type == Version::PreReleaseType::kNone &&
                      v0.pre_release_version == 0,
                  "");
  }
  SECTION("constructor") {
    constexpr Version v1{1, 2, 3, Version::PreReleaseType::kReleaseCandidate};
    static_assert(v1.major == 1 &&
                      v1.minor == 2 &&
                      v1.patch == 3 &&
                      v1.pre_release_type == Version::PreReleaseType::kReleaseCandidate &&
                      v1.pre_release_version == 0,
                  "");

    constexpr Version v2{1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4};
    static_assert(v2.major == 1 &&
                      v2.minor == 2 &&
                      v2.patch == 3 &&
                      v2.pre_release_type == Version::PreReleaseType::kReleaseCandidate &&
                      v2.pre_release_version == 4,
                  "");

    constexpr Version v3{1, 2, 3};
    static_assert(v3.major == 1 &&
                      v3.minor == 2 &&
                      v3.patch == 3 &&
                      v3.pre_release_type == Version::PreReleaseType::kNone &&
                      v3.pre_release_version == 0,
                  "");

    constexpr Version v4{1, 2, 3, Version::PreReleaseType::kNone};
    static_assert(v4.major == 1 &&
                      v4.minor == 2 &&
                      v4.patch == 3 &&
                      v4.pre_release_type == Version::PreReleaseType::kNone &&
                      v4.pre_release_version == 0,
                  "");

    constexpr Version v5{1, 2, 3, Version::PreReleaseType::kNone, 0};
    static_assert(v5.major == 1 &&
                      v5.minor == 2 &&
                      v5.patch == 3 &&
                      v5.pre_release_type == Version::PreReleaseType::kNone &&
                      v5.pre_release_version == 0,
                  "");

    constexpr Version v6{1, 2, 3, Version::PreReleaseType::kNone, 4};
    static_assert(v6.major == 1 &&
                      v6.minor == 2 &&
                      v6.patch == 3 &&
                      v6.pre_release_type == Version::PreReleaseType::kNone &&
                      v6.pre_release_version == 0,
                  "");

    constexpr Version v7{v6};
    static_assert(v7.major == 1 &&
                      v7.minor == 2 &&
                      v7.patch == 3 &&
                      v7.pre_release_type == Version::PreReleaseType::kNone &&
                      v7.pre_release_version == 0,
                  "");

    constexpr Version v8{ Version{1, 2, 3, Version::PreReleaseType::kNone, 4} };
    static_assert(v8.major == 1 &&
                      v8.minor == 2 &&
                      v8.patch == 3 &&
                      v8.pre_release_type == Version::PreReleaseType::kNone &&
                      v8.pre_release_version == 0,
                  "");
  }
}

TEST_CASE("operators") {
  constexpr std::size_t versions_length = 56;
  const std::array<Version, versions_length> versions = {{
      Version{0, 0, 0, Version::PreReleaseType::kAlpha, 0},
      Version{0, 0, 0, Version::PreReleaseType::kAlpha, 1},
      Version{0, 0, 0, Version::PreReleaseType::kBetha, 0},
      Version{0, 0, 0, Version::PreReleaseType::kBetha, 1},
      Version{0, 0, 0, Version::PreReleaseType::kReleaseCandidate, 0},
      Version{0, 0, 0, Version::PreReleaseType::kReleaseCandidate, 1},
      Version{0, 0, 0},

      Version{0, 0, 1, Version::PreReleaseType::kAlpha, 0},
      Version{0, 0, 1, Version::PreReleaseType::kAlpha, 1},
      Version{0, 0, 1, Version::PreReleaseType::kBetha, 0},
      Version{0, 0, 1, Version::PreReleaseType::kBetha, 1},
      Version{0, 0, 1, Version::PreReleaseType::kReleaseCandidate, 0},
      Version{0, 0, 1, Version::PreReleaseType::kReleaseCandidate, 1},
      Version{0, 0, 1},

      Version{0, 1, 0, Version::PreReleaseType::kAlpha, 0},
      Version{0, 1, 0, Version::PreReleaseType::kAlpha, 1},
      Version{0, 1, 0, Version::PreReleaseType::kBetha, 0},
      Version{0, 1, 0, Version::PreReleaseType::kBetha, 1},
      Version{0, 1, 0, Version::PreReleaseType::kReleaseCandidate, 0},
      Version{0, 1, 0, Version::PreReleaseType::kReleaseCandidate, 1},
      Version{0, 1, 0},

      Version{0, 1, 1, Version::PreReleaseType::kAlpha, 0},
      Version{0, 1, 1, Version::PreReleaseType::kAlpha, 1},
      Version{0, 1, 1, Version::PreReleaseType::kBetha, 0},
      Version{0, 1, 1, Version::PreReleaseType::kBetha, 1},
      Version{0, 1, 1, Version::PreReleaseType::kReleaseCandidate, 0},
      Version{0, 1, 1, Version::PreReleaseType::kReleaseCandidate, 1},
      Version{0, 1, 1},

      Version{1, 0, 0, Version::PreReleaseType::kAlpha, 0},
      Version{1, 0, 0, Version::PreReleaseType::kAlpha, 1},
      Version{1, 0, 0, Version::PreReleaseType::kBetha, 0},
      Version{1, 0, 0, Version::PreReleaseType::kBetha, 1},
      Version{1, 0, 0, Version::PreReleaseType::kReleaseCandidate, 0},
      Version{1, 0, 0, Version::PreReleaseType::kReleaseCandidate, 1},
      Version{1, 0, 0},

      Version{1, 0, 1, Version::PreReleaseType::kAlpha, 0},
      Version{1, 0, 1, Version::PreReleaseType::kAlpha, 1},
      Version{1, 0, 1, Version::PreReleaseType::kBetha, 0},
      Version{1, 0, 1, Version::PreReleaseType::kBetha, 1},
      Version{1, 0, 1, Version::PreReleaseType::kReleaseCandidate, 0},
      Version{1, 0, 1, Version::PreReleaseType::kReleaseCandidate, 1},
      Version{1, 0, 1},

      Version{1, 1, 0, Version::PreReleaseType::kAlpha, 0},
      Version{1, 1, 0, Version::PreReleaseType::kAlpha, 1},
      Version{1, 1, 0, Version::PreReleaseType::kBetha, 0},
      Version{1, 1, 0, Version::PreReleaseType::kBetha, 1},
      Version{1, 1, 0, Version::PreReleaseType::kReleaseCandidate, 0},
      Version{1, 1, 0, Version::PreReleaseType::kReleaseCandidate, 1},
      Version{1, 1, 0},

      Version{1, 1, 1, Version::PreReleaseType::kAlpha, 0},
      Version{1, 1, 1, Version::PreReleaseType::kAlpha, 1},
      Version{1, 1, 1, Version::PreReleaseType::kBetha, 0},
      Version{1, 1, 1, Version::PreReleaseType::kBetha, 1},
      Version{1, 1, 1, Version::PreReleaseType::kReleaseCandidate, 0},
      Version{1, 1, 1, Version::PreReleaseType::kReleaseCandidate, 1},
      Version{1, 1, 1},
  }};

  SECTION("operator =") {
    // check constexpr
    constexpr Version v1{1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4};
    constexpr Version v2 = v1;
    static_assert(v1.major == v2.major &&
                      v1.minor == v2.minor &&
                      v1.patch == v2.patch &&
                      v1.pre_release_type == v2.pre_release_type  &&
                      v1.pre_release_version == v2.pre_release_version,
                  "");

    for (std::size_t i = 0; i < versions_length; ++i) {
      Version v_temp = versions[i];
      CKECK_OP_AND_REVERSE(v_temp, ==, versions[i]);
    }
  }

  SECTION("operator ==") {
    // check constexpr
    constexpr Version v1{1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4};
    constexpr Version v2{1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4};
    STATIC_CKECK_OP_AND_REVERSE(v1, ==, v2);

    for (std::size_t i = 0; i < versions_length; ++i) {
      Version v_temp = versions[i];
      CKECK_OP_AND_REVERSE(v_temp, ==, versions[i]);
    }
  }

  SECTION("operator !=") {
    // check constexpr
    constexpr Version v1{1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4};
    constexpr Version v2{1, 2, 3};
    STATIC_CKECK_OP_AND_REVERSE(v1, !=, v2);

    for (std::size_t i = 1; i < versions_length; ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        CKECK_OP_AND_REVERSE(versions[i], !=, versions[i - j]);
      }
    }
  }

  SECTION("operator >") {
    // check constexpr
    constexpr Version v1{1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4};
    constexpr Version v2{1, 2, 3};
    STATIC_CKECK_OP_AND_REVERSE_FALSE(v2, >, v1);

    for (std::size_t i = 1; i < versions_length; ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        CKECK_OP_AND_REVERSE_FALSE(versions[i], >, versions[i - j]);
      }
    }
  }

  SECTION("operator >=") {
    // check constexpr
    constexpr Version v1{1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4};
    constexpr Version v2{1, 2, 3};
    constexpr Version v3{1, 2, 3};
    STATIC_CKECK_OP_AND_REVERSE_FALSE(v2, >=, v1);
    STATIC_CKECK_OP_AND_REVERSE(v2, >=, v3);

    for (std::size_t i = 1; i < versions_length; ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        Version v_temp = versions[i];
        CKECK_OP_AND_REVERSE_FALSE(versions[i], >=, versions[i - j]);
        CKECK_OP_AND_REVERSE(v_temp, >=, versions[i]);
      }
    }
  }

  SECTION("operator <") {
    // check constexpr
    constexpr Version v1{1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4};
    constexpr Version v2{1, 2, 3};
    STATIC_CKECK_OP_AND_REVERSE_FALSE(v1, <, v2);

    for (std::size_t i = 1; i < versions_length; ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        CKECK_OP_AND_REVERSE_FALSE(versions[i - j], <, versions[i]);
      }
    }
  }

  SECTION("operator <=") {
    // check constexpr
    constexpr Version v1{1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4};
    constexpr Version v2{1, 2, 3};
    constexpr Version v3{1, 2, 3};
    STATIC_CKECK_OP_AND_REVERSE_FALSE(v1, <=, v2);
    STATIC_CKECK_OP_AND_REVERSE(v2, <=, v3);

    for (std::size_t i = 1; i < versions_length; ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        Version v_temp = versions[i - j];
        CKECK_OP_AND_REVERSE_FALSE(versions[i - j], <=, versions[i]);
        CKECK_OP_AND_REVERSE(v_temp, <=,versions[i - j]);
      }
    }
  }
}

TEST_CASE("from/to string") {
  constexpr std::size_t versions_length = 19;
  const std::array<Version, versions_length> versions = {{
      Version{1, 2, 3},
      Version{255, 255, 255},
      Version{0, 0, 0},
      //
      Version{1, 2, 3, Version::PreReleaseType::kNone, 0},
      Version{1, 2, 3, Version::PreReleaseType::kNone, 4},
      Version{255, 255, 255, Version::PreReleaseType::kNone, 255},
      Version{0, 0, 0, Version::PreReleaseType::kNone, 0},
      //
      Version{1, 2, 3, Version::PreReleaseType::kAlpha, 0},
      Version{1, 2, 3, Version::PreReleaseType::kAlpha, 4},
      Version{255, 255, 255, Version::PreReleaseType::kAlpha, 255},
      Version{0, 0, 0, Version::PreReleaseType::kAlpha, 0},
      //
      Version{1, 2, 3, Version::PreReleaseType::kBetha, 0},
      Version{1, 2, 3, Version::PreReleaseType::kBetha, 4},
      Version{255, 255, 255, Version::PreReleaseType::kBetha, 255},
      Version{0, 0, 0, Version::PreReleaseType::kBetha, 0},
      //
      Version{1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 0},
      Version{1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4},
      Version{255, 255, 255, Version::PreReleaseType::kReleaseCandidate, 255},
      Version{0, 0, 0, Version::PreReleaseType::kReleaseCandidate, 0},
  }};
  const std::array<std::string, versions_length> versions_strings = {{
      "1.2.3",
      "255.255.255",
      "0.0.0",
      //
      "1.2.3",
      "1.2.3",
      "255.255.255",
      "0.0.0",
      //
      "1.2.3-alpha",
      "1.2.3-alpha.4",
      "255.255.255-alpha.255",
      "0.0.0-alpha",
      //
      "1.2.3-betha",
      "1.2.3-betha.4",
      "255.255.255-betha.255",
      "0.0.0-betha",
      //
      "1.2.3-rc",
      "1.2.3-rc.4",
      "255.255.255-rc.255",
      "0.0.0-rc",
  }};

  SECTION("from std::string") {
    for (std::size_t i = 0; i < versions_length; ++i) {
      Version v1{versions_strings[i]};
      REQUIRE(versions[i] == v1);

      Version v2;
      REQUIRE(v2.FromString(versions_strings[i]));
      REQUIRE(versions[i] == v2);

      Version v3 = FromString(versions_strings[i]);
      REQUIRE(v3.IsValid());
      REQUIRE(versions[i] == v3);
    }
  }

  SECTION("from char*") {
    for (std::size_t i = 0; i < versions_length; ++i) {
      Version v1{versions_strings[i].c_str()};
      REQUIRE(versions[i] == v1);

      Version v2;
      REQUIRE(v2.FromString(versions_strings[i].c_str()));
      REQUIRE(versions[i] == v2);

      Version v3 = FromString(versions_strings[i].c_str());
      REQUIRE(v3.IsValid());
      REQUIRE(versions[i] == v3);
    }
  }

  SECTION("to std::string") {
    for (std::size_t i = 0; i < versions_length; ++i) {
      const std::string s1 = versions[i].ToString();
      REQUIRE(s1 == versions_strings[i]);

      const std::string s2 = ToString(versions[i]);
      REQUIRE(s2 == versions_strings[i]);
    }
  }

  SECTION("to char*") {
    for (std::size_t i = 0; i < versions_length; ++i) {
      char s1[Version::kVersionStringLength];
      const auto size1 = versions[i].ToString(s1);
      REQUIRE(s1 == versions_strings[i]);
      REQUIRE(size1 == versions_strings[i].length());

      char s2[Version::kVersionStringLength];
      const auto size2 = ToString(versions[i], s2);
      REQUIRE(s2 == versions_strings[i]);
      REQUIRE(size2 == versions_strings[i].length());
    }
  }

  SECTION("operator <<") {
    for (std::size_t i = 0; i < versions_length; ++i) {
      std::stringstream os;
      os << versions[i];
      REQUIRE(os.str() == versions_strings[i]);
    }
  }

  SECTION("operator >>") {
    for (std::size_t i = 0; i < versions_length; ++i) {
      std::stringstream is{versions_strings[i]};
      Version vi;
      is >> vi;
      REQUIRE(versions[i] == vi);
    }
  }

  SECTION("operator _version") {
    Version v = "1.2.3-rc.4"_version;
    REQUIRE(v == Version{1, 2, 3, Version::PreReleaseType::kReleaseCandidate, 4});
  }
}

TEST_CASE("is valid") {
  SECTION("operators") {
    constexpr Version v1(1, 4, 3, static_cast<Version::PreReleaseType>(-100));
    constexpr Version v2(1, 4, 3, static_cast<Version::PreReleaseType>(-50));
    STATIC_CKECK_FALSE_OP_AND_REVERSE(v1, ==, v2); // false
    STATIC_CKECK_OP_AND_REVERSE(v1, != , v2); // true
    STATIC_CKECK_FALSE_OP_AND_REVERSE(v1, > , v2); // false
    STATIC_CKECK_FALSE_OP_AND_REVERSE(v1, >= , v2); // false
    STATIC_CKECK_FALSE_OP_AND_REVERSE(v1, < , v2); // false
    STATIC_CKECK_FALSE_OP_AND_REVERSE(v1, <= , v2); // false
  }
}

#undef STATIC_CKECK_OP_AND_REVERSE
#undef STATIC_CKECK_OP_AND_REVERSE_FALSE
#undef STATIC_CKECK_FALSE_OP_AND_REVERSE
#undef CKECK_OP_AND_REVERSE
#undef CKECK_OP_AND_REVERSE_FALSE
#undef CKECK_FALSE_OP_AND_REVERSE
