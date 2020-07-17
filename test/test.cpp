// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2020 Daniil Goncharov <neargye@gmail.com>.
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
#include <string_view>
#include <sstream>

using namespace semver;

static_assert(semver_verion.major == SEMVER_VERSION_MAJOR);
static_assert(semver_verion.minor == SEMVER_VERSION_MINOR);
static_assert(semver_verion.patch == SEMVER_VERSION_PATCH);

static_assert(alignof(version) == 1);
static_assert(alignof(prerelease) == 1);
static_assert(sizeof(version) == 5);
static_assert(sizeof(prerelease) == 1);

#define STATIC_CKECK_OP_AND_REVERSE(v1, op, v2) \
  static_assert(v1 op v2);                      \
  static_assert(v2 op v1);

#define STATIC_CKECK_OP_AND_REVERSE_FALSE(v1, op, v2) \
  static_assert(v1 op v2);                            \
  static_assert(!(v2 op v1));

#define STATIC_CKECK_FALSE_OP_AND_REVERSE(v1, op, v2) \
  static_assert(!(v1 op v2));                         \
  static_assert(!(v2 op v1));

#define CKECK_OP_AND_REVERSE(v1, op, v2) \
  REQUIRE(v1 op v2);                     \
  REQUIRE(v2 op v1);

#define CKECK_OP_AND_REVERSE_FALSE(v1, op, v2) \
  REQUIRE(v1 op v2);                           \
  REQUIRE_FALSE(v2 op v1);

#define CKECK_FALSE_OP_AND_REVERSE(v1, op, v2) \
  REQUIRE_FALSE(v1 op v2);                     \
  REQUIRE_FALSE(v2 op v1);

TEST_CASE("constructors") {
  SECTION("default") {
    constexpr version v0;
    static_assert(v0.major == 0 &&
                      v0.minor == 1 &&
                      v0.patch == 0 &&
                      v0.prerelease_type == prerelease::none &&
                      v0.prerelease_number == 0);
  }

  SECTION("constructor") {
    constexpr version v1{1, 2, 3, prerelease::rc};
    static_assert(v1.major == 1 &&
                      v1.minor == 2 &&
                      v1.patch == 3 &&
                      v1.prerelease_type == prerelease::rc &&
                      v1.prerelease_number == 0);

    constexpr version v2{1, 2, 3, prerelease::rc, 4};
    static_assert(v2.major == 1 &&
                      v2.minor == 2 &&
                      v2.patch == 3 &&
                      v2.prerelease_type == prerelease::rc &&
                      v2.prerelease_number == 4);

    constexpr version v3{1, 2, 3};
    static_assert(v3.major == 1 &&
                      v3.minor == 2 &&
                      v3.patch == 3 &&
                      v3.prerelease_type == prerelease::none &&
                      v3.prerelease_number == 0);

    constexpr version v4{1, 2, 3, prerelease::none};
    static_assert(v4.major == 1 &&
                      v4.minor == 2 &&
                      v4.patch == 3 &&
                      v4.prerelease_type == prerelease::none &&
                      v4.prerelease_number == 0);

    constexpr version v5{1, 2, 3, prerelease::none, 0};
    static_assert(v5.major == 1 &&
                      v5.minor == 2 &&
                      v5.patch == 3 &&
                      v5.prerelease_type == prerelease::none &&
                      v5.prerelease_number == 0);

    constexpr version v6{1, 2, 3, prerelease::none, 4};
    static_assert(v6.major == 1 &&
                      v6.minor == 2 &&
                      v6.patch == 3 &&
                      v6.prerelease_type == prerelease::none &&
                      v6.prerelease_number == 0);

    constexpr version v7{v6};
    static_assert(v7.major == 1 &&
                      v7.minor == 2 &&
                      v7.patch == 3 &&
                      v7.prerelease_type == prerelease::none &&
                      v7.prerelease_number == 0);

    constexpr version v8{v6};
    static_assert(v8.major == 1 &&
                      v8.minor == 2 &&
                      v8.patch == 3 &&
                      v8.prerelease_type == prerelease::none &&
                      v8.prerelease_number == 0);

    constexpr version v9{"1.2.3-alpha.4"};
    static_assert(v9.major == 1 &&
                      v9.minor == 2 &&
                      v9.patch == 3 &&
                      v9.prerelease_type == prerelease::alpha &&
                      v9.prerelease_number == 4);

    std::string s = "1.1.1";
    version v10{s};
    REQUIRE(v10.major == 1);
    REQUIRE(v10.minor == 1);
    REQUIRE(v10.patch == 1);
    REQUIRE(v10.prerelease_type == prerelease::none);
    REQUIRE(v10.prerelease_number == 0);
  }
}

TEST_CASE("operators") {
  constexpr std::array<version, 56> versions = {{
      version{0, 0, 0, prerelease::alpha, 0},
      version{0, 0, 0, prerelease::alpha, 1},
      version{0, 0, 0, prerelease::beta, 0},
      version{0, 0, 0, prerelease::beta, 1},
      version{0, 0, 0, prerelease::rc, 0},
      version{0, 0, 0, prerelease::rc, 1},
      version{0, 0, 0},

      version{0, 0, 1, prerelease::alpha, 0},
      version{0, 0, 1, prerelease::alpha, 1},
      version{0, 0, 1, prerelease::beta, 0},
      version{0, 0, 1, prerelease::beta, 1},
      version{0, 0, 1, prerelease::rc, 0},
      version{0, 0, 1, prerelease::rc, 1},
      version{0, 0, 1},

      version{0, 1, 0, prerelease::alpha, 0},
      version{0, 1, 0, prerelease::alpha, 1},
      version{0, 1, 0, prerelease::beta, 0},
      version{0, 1, 0, prerelease::beta, 1},
      version{0, 1, 0, prerelease::rc, 0},
      version{0, 1, 0, prerelease::rc, 1},
      version{0, 1, 0},

      version{0, 1, 1, prerelease::alpha, 0},
      version{0, 1, 1, prerelease::alpha, 1},
      version{0, 1, 1, prerelease::beta, 0},
      version{0, 1, 1, prerelease::beta, 1},
      version{0, 1, 1, prerelease::rc, 0},
      version{0, 1, 1, prerelease::rc, 1},
      version{0, 1, 1},

      version{1, 0, 0, prerelease::alpha, 0},
      version{1, 0, 0, prerelease::alpha, 1},
      version{1, 0, 0, prerelease::beta, 0},
      version{1, 0, 0, prerelease::beta, 1},
      version{1, 0, 0, prerelease::rc, 0},
      version{1, 0, 0, prerelease::rc, 1},
      version{1, 0, 0},

      version{1, 0, 1, prerelease::alpha, 0},
      version{1, 0, 1, prerelease::alpha, 1},
      version{1, 0, 1, prerelease::beta, 0},
      version{1, 0, 1, prerelease::beta, 1},
      version{1, 0, 1, prerelease::rc, 0},
      version{1, 0, 1, prerelease::rc, 1},
      version{1, 0, 1},

      version{1, 1, 0, prerelease::alpha, 0},
      version{1, 1, 0, prerelease::alpha, 1},
      version{1, 1, 0, prerelease::beta, 0},
      version{1, 1, 0, prerelease::beta, 1},
      version{1, 1, 0, prerelease::rc, 0},
      version{1, 1, 0, prerelease::rc, 1},
      version{1, 1, 0},

      version{1, 1, 1, prerelease::alpha, 0},
      version{1, 1, 1, prerelease::alpha, 1},
      version{1, 1, 1, prerelease::beta, 0},
      version{1, 1, 1, prerelease::beta, 1},
      version{1, 1, 1, prerelease::rc, 0},
      version{1, 1, 1, prerelease::rc, 1},
      version{1, 1, 1},
  }};

  SECTION("operator =") {
    constexpr version v1{1, 2, 3, prerelease::rc, 4};
    constexpr version v2 = v1;
    STATIC_CKECK_OP_AND_REVERSE(v1, ==, v2);

    for (std::size_t i = 0; i < versions.size(); ++i) {
      version v_temp = versions[i];
      CKECK_OP_AND_REVERSE(v_temp, ==, versions[i]);
    }
  }

  SECTION("operator ==") {
    constexpr version v1{1, 2, 3, prerelease::rc, 4};
    constexpr version v2{1, 2, 3, prerelease::rc, 4};
    STATIC_CKECK_OP_AND_REVERSE(v1, ==, v2);

    for (std::size_t i = 0; i < versions.size(); ++i) {
      version v_temp = versions[i];
      CKECK_OP_AND_REVERSE(v_temp, ==, versions[i]);
    }
  }

  SECTION("operator !=") {
    constexpr version v1{1, 2, 3, prerelease::rc, 4};
    constexpr version v2{1, 2, 3};
    STATIC_CKECK_OP_AND_REVERSE(v1, !=, v2);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        CKECK_OP_AND_REVERSE(versions[i], !=, versions[i - j]);
      }
    }
  }

  SECTION("operator >") {
    constexpr version v1{1, 2, 3, prerelease::rc, 4};
    constexpr version v2{1, 2, 3};
    STATIC_CKECK_OP_AND_REVERSE_FALSE(v2, >, v1);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        CKECK_OP_AND_REVERSE_FALSE(versions[i], >, versions[i - j]);
      }
    }
  }

  SECTION("operator >=") {
    constexpr version v1{1, 2, 3, prerelease::rc, 4};
    constexpr version v2{1, 2, 3};
    constexpr version v3{1, 2, 3};
    STATIC_CKECK_OP_AND_REVERSE_FALSE(v2, >=, v1);
    STATIC_CKECK_OP_AND_REVERSE(v2, >=, v3);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        version v_temp = versions[i];
        CKECK_OP_AND_REVERSE_FALSE(versions[i], >=, versions[i - j]);
        CKECK_OP_AND_REVERSE(v_temp, >=, versions[i]);
      }
    }
  }

  SECTION("operator <") {
    constexpr version v1{1, 2, 3, prerelease::rc, 4};
    constexpr version v2{1, 2, 3};
    STATIC_CKECK_OP_AND_REVERSE_FALSE(v1, <, v2);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        CKECK_OP_AND_REVERSE_FALSE(versions[i - j], <, versions[i]);
      }
    }
  }

  SECTION("operator <=") {
    constexpr version v1{1, 2, 3, prerelease::rc, 4};
    constexpr version v2{1, 2, 3};
    constexpr version v3{1, 2, 3};
    STATIC_CKECK_OP_AND_REVERSE_FALSE(v1, <=, v2);
    STATIC_CKECK_OP_AND_REVERSE(v2, <=, v3);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        version v_temp = versions[i - j];
        CKECK_OP_AND_REVERSE_FALSE(versions[i - j], <=, versions[i]);
        CKECK_OP_AND_REVERSE(v_temp, <=, versions[i - j]);
      }
    }
  }

  SECTION("operator _version") {
    constexpr version v = "1.2.3-rc.4"_version;
    static_assert(v == version{1, 2, 3, prerelease::rc, 4});
  }
}

TEST_CASE("from/to string") {
  constexpr std::array<version, 19> versions = {{
      version{1, 2, 3},
      version{255, 255, 255},
      version{0, 0, 0},
      //
      version{1, 2, 3, prerelease::none, 0},
      version{1, 2, 3, prerelease::none, 4},
      version{255, 255, 255, prerelease::none, 255},
      version{0, 0, 0, prerelease::none, 0},
      //
      version{1, 2, 3, prerelease::alpha, 0},
      version{1, 2, 3, prerelease::alpha, 4},
      version{255, 255, 255, prerelease::alpha, 255},
      version{0, 0, 0, prerelease::alpha, 0},
      //
      version{1, 2, 3, prerelease::beta, 0},
      version{1, 2, 3, prerelease::beta, 4},
      version{255, 255, 255, prerelease::beta, 255},
      version{0, 0, 0, prerelease::beta, 0},
      //
      version{1, 2, 3, prerelease::rc, 0},
      version{1, 2, 3, prerelease::rc, 4},
      version{255, 255, 255, prerelease::rc, 255},
      version{0, 0, 0, prerelease::rc, 0},
  }};

  constexpr std::array<std::string_view, 19> versions_strings = {{
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
      "1.2.3-beta",
      "1.2.3-beta.4",
      "255.255.255-beta.255",
      "0.0.0-beta",
      //
      "1.2.3-rc",
      "1.2.3-rc.4",
      "255.255.255-rc.255",
      "0.0.0-rc",
  }};

  SECTION("from chars") {
    version v;
    for (std::size_t i = 0; i < versions.size(); ++i) {
      REQUIRE(v.from_chars(versions_strings[i].data(), versions_strings[i].data() + versions_strings[i].size()));
      REQUIRE(versions[i] == v);
    }
  }

  SECTION("to chars") {
    for (std::size_t i = 0; i < versions.size(); ++i) {
      std::array<char, semver::max_version_string_length + 1> m = {};
      REQUIRE(versions[i].to_chars(m.data(), m.data() + m.size()));
      auto s = std::string_view{m.data()};
      REQUIRE(s == versions_strings[i]);
    }
  }

  SECTION("from string") {
    version v;
    for (std::size_t i = 0; i < versions.size(); ++i) {
      v.from_string(versions_strings[i]);
      REQUIRE(versions[i] == v);
    }
  }

  SECTION("to string") {
    for (std::size_t i = 0; i < versions.size(); ++i) {
      auto s = versions[i].to_string();
      REQUIRE(s == versions_strings[i]);
    }
  }

  SECTION("valid") {
    for (std::size_t i = 0; i < versions.size(); ++i) {
      REQUIRE(semver::valid(versions_strings[i]));
    }

    REQUIRE(!semver::valid("a"));
    REQUIRE(!semver::valid("1.2.3.4"));
    REQUIRE(!semver::valid("v1.2.4"));
    REQUIRE(!semver::valid("1.2"));
  }
}
