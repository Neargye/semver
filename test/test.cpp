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

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <semver.hpp>

#include <cstddef>
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <sstream>

using namespace semver;

static_assert(semver_version.major == SEMVER_VERSION_MAJOR);
static_assert(semver_version.minor == SEMVER_VERSION_MINOR);
static_assert(semver_version.patch == SEMVER_VERSION_PATCH);

static_assert(alignof(version) == 2);
static_assert(alignof(prerelease) == 1);
static_assert(sizeof(version) == 12);
static_assert(sizeof(prerelease) == 1);

#define STATIC_CHECK_OP_AND_REVERSE(v1, op, v2) \
  static_assert(v1 op v2);                      \
  static_assert(v2 op v1);

#define STATIC_CHECK_OP_AND_REVERSE_FALSE(v1, op, v2) \
  static_assert(v1 op v2);                            \
  static_assert(!(v2 op v1));

#define STATIC_CHECK_FALSE_OP_AND_REVERSE(v1, op, v2) \
  static_assert(!(v1 op v2));                         \
  static_assert(!(v2 op v1));

#define CHECK_OP_AND_REVERSE(v1, op, v2) \
  REQUIRE(v1 op v2);                     \
  REQUIRE(v2 op v1);

#define CHECK_OP_AND_REVERSE_FALSE(v1, op, v2) \
  REQUIRE(v1 op v2);                           \
  REQUIRE_FALSE(v2 op v1);

#define CHECK_FALSE_OP_AND_REVERSE(v1, op, v2) \
  REQUIRE_FALSE(v1 op v2);                     \
  REQUIRE_FALSE(v2 op v1);

TEST_CASE("constructors") {
  SECTION("default") {
    constexpr version v0;
    static_assert(v0.major == 0 &&
                      v0.minor == 1 &&
                      v0.patch == 0 &&
                      v0.prerelease_type == prerelease::none &&
                      !v0.prerelease_number.has_value());
  }

  SECTION("constructor") {
    constexpr version v1{1, 2, 3, prerelease::rc};
    static_assert(v1.major == 1 &&
                      v1.minor == 2 &&
                      v1.patch == 3 &&
                      v1.prerelease_type == prerelease::rc &&
                      !v1.prerelease_number.has_value());

    constexpr version v2{1, 2, 3, prerelease::rc, 4};
    static_assert(v2.major == 1 &&
                      v2.minor == 2 &&
                      v2.patch == 3 &&
                      v2.prerelease_type == prerelease::rc &&
                      v2.prerelease_number.has_value() &&
                      v2.prerelease_number == 4);

    constexpr version v3{1, 2, 3};
    static_assert(v3.major == 1 &&
                      v3.minor == 2 &&
                      v3.patch == 3 &&
                      v3.prerelease_type == prerelease::none &&
                      !v3.prerelease_number.has_value());

    constexpr version v4{1, 2, 3, prerelease::none};
    static_assert(v4.major == 1 &&
                      v4.minor == 2 &&
                      v4.patch == 3 &&
                      v4.prerelease_type == prerelease::none &&
                      !v4.prerelease_number.has_value());

    constexpr version v5{1, 2, 3, prerelease::none, 0};
    static_assert(v5.major == 1 &&
                      v5.minor == 2 &&
                      v5.patch == 3 &&
                      v5.prerelease_type == prerelease::none &&
                      !v5.prerelease_number.has_value());

    constexpr version v6{1, 2, 3, prerelease::none, 4};
    static_assert(v6.major == 1 &&
                      v6.minor == 2 &&
                      v6.patch == 3 &&
                      v6.prerelease_type == prerelease::none &&
                      !v6.prerelease_number.has_value());

    constexpr version v7{v6};
    static_assert(v7.major == 1 &&
                      v7.minor == 2 &&
                      v7.patch == 3 &&
                      v7.prerelease_type == prerelease::none &&
                      !v7.prerelease_number.has_value());

    constexpr version v8{v6};
    static_assert(v8.major == 1 &&
                      v8.minor == 2 &&
                      v8.patch == 3 &&
                      v8.prerelease_type == prerelease::none &&
                      !v8.prerelease_number.has_value());

    constexpr version v9{"1.2.3-alpha.4"};
    static_assert(v9.major == 1 &&
                      v9.minor == 2 &&
                      v9.patch == 3 &&
                      v9.prerelease_type == prerelease::alpha &&
                      v9.prerelease_number.has_value() &&
                      v9.prerelease_number == 4);
  
    constexpr version v10{"1.2.3-alpha.0"};
    static_assert(v10.major == 1 &&
                      v10.minor == 2 &&
                      v10.patch == 3 &&
                      v10.prerelease_type == prerelease::alpha &&
                      v10.prerelease_number.has_value() &&
                      v10.prerelease_number == 0);

    std::string s = "1.1.1";
    version v11{s};
    REQUIRE(v11.major == 1);
    REQUIRE(v11.minor == 1);
    REQUIRE(v11.patch == 1);
    REQUIRE(v11.prerelease_type == prerelease::none);
    REQUIRE(!v11.prerelease_number.has_value());
  }
}

TEST_CASE("operators") {
  constexpr std::array<version, 80> versions = {{
      version{0, 0, 0, prerelease::alpha, std::nullopt},
      version{0, 0, 0, prerelease::alpha, 0},
      version{0, 0, 0, prerelease::alpha, 1},
      version{0, 0, 0, prerelease::beta, std::nullopt},
      version{0, 0, 0, prerelease::beta, 0},
      version{0, 0, 0, prerelease::beta, 1},
      version{0, 0, 0, prerelease::rc, std::nullopt},
      version{0, 0, 0, prerelease::rc, 0},
      version{0, 0, 0, prerelease::rc, 1},
      version{0, 0, 0},

      version{0, 0, 1, prerelease::alpha, std::nullopt},
      version{0, 0, 1, prerelease::alpha, 0},
      version{0, 0, 1, prerelease::alpha, 1},
      version{0, 0, 1, prerelease::beta, std::nullopt},
      version{0, 0, 1, prerelease::beta, 0},
      version{0, 0, 1, prerelease::beta, 1},
      version{0, 0, 1, prerelease::rc, std::nullopt},
      version{0, 0, 1, prerelease::rc, 0},
      version{0, 0, 1, prerelease::rc, 1},
      version{0, 0, 1},

      version{0, 1, 0, prerelease::alpha, std::nullopt},
      version{0, 1, 0, prerelease::alpha, 0},
      version{0, 1, 0, prerelease::alpha, 1},
      version{0, 1, 0, prerelease::beta, std::nullopt},
      version{0, 1, 0, prerelease::beta, 0},
      version{0, 1, 0, prerelease::beta, 1},
      version{0, 1, 0, prerelease::rc, std::nullopt},
      version{0, 1, 0, prerelease::rc, 0},
      version{0, 1, 0, prerelease::rc, 1},
      version{0, 1, 0},

      version{0, 1, 1, prerelease::alpha, std::nullopt},
      version{0, 1, 1, prerelease::alpha, 0},
      version{0, 1, 1, prerelease::alpha, 1},
      version{0, 1, 1, prerelease::beta, std::nullopt},
      version{0, 1, 1, prerelease::beta, 0},
      version{0, 1, 1, prerelease::beta, 1},
      version{0, 1, 1, prerelease::rc, std::nullopt},
      version{0, 1, 1, prerelease::rc, 0},
      version{0, 1, 1, prerelease::rc, 1},
      version{0, 1, 1},

      version{1, 0, 0, prerelease::alpha, std::nullopt},
      version{1, 0, 0, prerelease::alpha, 0},
      version{1, 0, 0, prerelease::alpha, 1},
      version{1, 0, 0, prerelease::beta, std::nullopt},
      version{1, 0, 0, prerelease::beta, 0},
      version{1, 0, 0, prerelease::beta, 1},
      version{1, 0, 0, prerelease::rc, std::nullopt},
      version{1, 0, 0, prerelease::rc, 0},
      version{1, 0, 0, prerelease::rc, 1},
      version{1, 0, 0},

      version{1, 0, 1, prerelease::alpha, std::nullopt},
      version{1, 0, 1, prerelease::alpha, 0},
      version{1, 0, 1, prerelease::alpha, 1},
      version{1, 0, 1, prerelease::beta, std::nullopt},
      version{1, 0, 1, prerelease::beta, 0},
      version{1, 0, 1, prerelease::beta, 1},
      version{1, 0, 1, prerelease::rc, std::nullopt},
      version{1, 0, 1, prerelease::rc, 0},
      version{1, 0, 1, prerelease::rc, 1},
      version{1, 0, 1},

      version{1, 1, 0, prerelease::alpha, std::nullopt},
      version{1, 1, 0, prerelease::alpha, 0},
      version{1, 1, 0, prerelease::alpha, 1},
      version{1, 1, 0, prerelease::beta, std::nullopt},
      version{1, 1, 0, prerelease::beta, 0},
      version{1, 1, 0, prerelease::beta, 1},
      version{1, 1, 0, prerelease::rc, std::nullopt},
      version{1, 1, 0, prerelease::rc, 0},
      version{1, 1, 0, prerelease::rc, 1},
      version{1, 1, 0},

      version{1, 1, 1, prerelease::alpha, std::nullopt},
      version{1, 1, 1, prerelease::alpha, 0},
      version{1, 1, 1, prerelease::alpha, 1},
      version{1, 1, 1, prerelease::beta, std::nullopt},
      version{1, 1, 1, prerelease::beta, 0},
      version{1, 1, 1, prerelease::beta, 1},
      version{1, 1, 1, prerelease::rc, std::nullopt},
      version{1, 1, 1, prerelease::rc, 0},
      version{1, 1, 1, prerelease::rc, 1},
      version{1, 1, 1},
  }};

  SECTION("operator =") {
    constexpr version v1{1, 2, 3, prerelease::rc, 4};
    constexpr version v2 = v1;
    STATIC_CHECK_OP_AND_REVERSE(v1, ==, v2);
    static_assert(compare(v1, v2) == 0);
    static_assert(compare(v2, v1) == 0);

    for (std::size_t i = 0; i < versions.size(); ++i) {
      version v = versions[i];
      CHECK_OP_AND_REVERSE(v, ==, versions[i]);
    }
  }

  SECTION("operator ==") {
    constexpr version v1{1, 2, 3, prerelease::rc, 4};
    constexpr version v2{1, 2, 3, prerelease::rc, 4};
    STATIC_CHECK_OP_AND_REVERSE(v1, ==, v2);

    for (std::size_t i = 0; i < versions.size(); ++i) {
      version v = versions[i];
      CHECK_OP_AND_REVERSE(v, ==, versions[i]);

      REQUIRE(compare(v, versions[i], comparators_option::include_prerelease) == 0);
      REQUIRE(compare(versions[i], v, comparators_option::include_prerelease) == 0);
      REQUIRE(compare(v, versions[i], comparators_option::exclude_prerelease) == 0);
      REQUIRE(compare(versions[i], v, comparators_option::exclude_prerelease) == 0);
    }
  }

  SECTION("operator !=") {
    constexpr version v1{1, 2, 3, prerelease::rc, 4};
    constexpr version v2{1, 2, 3};
    STATIC_CHECK_OP_AND_REVERSE(v1, !=, v2);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        CHECK_OP_AND_REVERSE(versions[i], !=, versions[i - j]);

        REQUIRE(compare(versions[i], versions[i - j], comparators_option::include_prerelease) != 0);
        REQUIRE(compare(versions[i - j], versions[i], comparators_option::include_prerelease) != 0);
        if ((i - j) / 10 == i / 10) {
          REQUIRE(compare(versions[i], versions[i - j], comparators_option::exclude_prerelease) == 0);
          REQUIRE(compare(versions[i - j], versions[i], comparators_option::exclude_prerelease) == 0);
        } else {
          REQUIRE(compare(versions[i], versions[i - j], comparators_option::exclude_prerelease) != 0);
          REQUIRE(compare(versions[i - j], versions[i], comparators_option::exclude_prerelease) != 0);
        }
      }
    }
  }

  SECTION("operator >") {
    constexpr version v1{1, 2, 3, prerelease::rc, 4};
    constexpr version v2{1, 2, 3};
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v2, >, v1);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        CHECK_OP_AND_REVERSE_FALSE(versions[i], >, versions[i - j]);

        REQUIRE(greater(versions[i], versions[i - j], comparators_option::include_prerelease));
        REQUIRE_FALSE(greater(versions[i - j], versions[i], comparators_option::include_prerelease));
        if ((i - j) / 10 == i / 10) {
          REQUIRE_FALSE(greater(versions[i], versions[i - j], comparators_option::exclude_prerelease));
          REQUIRE_FALSE(greater(versions[i - j], versions[i], comparators_option::exclude_prerelease));
        } else {
          REQUIRE(greater(versions[i], versions[i - j], comparators_option::exclude_prerelease));
          REQUIRE_FALSE(greater(versions[i - j], versions[i], comparators_option::exclude_prerelease));
        }
      }
    }
  }

  SECTION("operator >=") {
    constexpr version v1{1, 2, 3, prerelease::rc, 4};
    constexpr version v2{1, 2, 3};
    constexpr version v3{1, 2, 3};
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v2, >=, v1);
    STATIC_CHECK_OP_AND_REVERSE(v2, >=, v3);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        version v = versions[i];
        CHECK_OP_AND_REVERSE_FALSE(versions[i], >=, versions[i - j]);
        CHECK_OP_AND_REVERSE(v, >=, versions[i]);

        REQUIRE(greater_equal(versions[i], versions[i - j], comparators_option::include_prerelease));
        REQUIRE_FALSE(greater_equal(versions[i - j], versions[i], comparators_option::include_prerelease));
        REQUIRE(greater_equal(versions[i], v, comparators_option::include_prerelease));
        REQUIRE(greater_equal(v, versions[i], comparators_option::include_prerelease));
        if ((i - j) / 10 == i / 10) {
          REQUIRE(greater_equal(versions[i], versions[i - j], comparators_option::exclude_prerelease));
          REQUIRE(greater_equal(versions[i - j], versions[i], comparators_option::exclude_prerelease));
          REQUIRE(greater_equal(versions[i], v, comparators_option::exclude_prerelease));
          REQUIRE(greater_equal(v, versions[i], comparators_option::exclude_prerelease));
        } else {
          REQUIRE(greater_equal(versions[i], versions[i - j], comparators_option::exclude_prerelease));
          REQUIRE_FALSE(greater_equal(versions[i - j], versions[i], comparators_option::exclude_prerelease));
        }
      }
    }
  }

  SECTION("operator <") {
    constexpr version v1{1, 2, 3, prerelease::rc, 4};
    constexpr version v2{1, 2, 3};
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v1, <, v2);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        CHECK_OP_AND_REVERSE_FALSE(versions[i - j], <, versions[i]);

        REQUIRE(less(versions[i - j], versions[i], comparators_option::include_prerelease));
        REQUIRE_FALSE(less(versions[i], versions[i - j], comparators_option::include_prerelease));
        if ((i - j) / 10 == i / 10) {
          REQUIRE_FALSE(less(versions[i - j], versions[i], comparators_option::exclude_prerelease));
          REQUIRE_FALSE(less(versions[i], versions[i - j], comparators_option::exclude_prerelease));
        } else {
          REQUIRE(less(versions[i - j], versions[i], comparators_option::exclude_prerelease));
          REQUIRE_FALSE(less(versions[i], versions[i - j], comparators_option::exclude_prerelease));
        }
      }
    }
  }

  SECTION("operator <=") {
    constexpr version v1{1, 2, 3, prerelease::rc, 4};
    constexpr version v2{1, 2, 3};
    constexpr version v3{1, 2, 3};
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v1, <=, v2);
    STATIC_CHECK_OP_AND_REVERSE(v2, <=, v3);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        version v = versions[i - j];
        CHECK_OP_AND_REVERSE_FALSE(versions[i - j], <=, versions[i]);
        CHECK_OP_AND_REVERSE(v, <=, versions[i - j]);

        REQUIRE(less_equal(versions[i - j], versions[i], comparators_option::include_prerelease));
        REQUIRE_FALSE(less_equal(versions[i], versions[i - j], comparators_option::include_prerelease));
        REQUIRE(less_equal(v, versions[i - j], comparators_option::include_prerelease));
        REQUIRE(less_equal(versions[i - j], v, comparators_option::include_prerelease));
        if ((i - j) / 10 == i / 10) {
          REQUIRE(less_equal(versions[i - j], versions[i], comparators_option::exclude_prerelease));
          REQUIRE(less_equal(versions[i], versions[i - j], comparators_option::exclude_prerelease));
        } else {
          REQUIRE(less_equal(versions[i - j], versions[i], comparators_option::exclude_prerelease));
          REQUIRE_FALSE(less_equal(versions[i], versions[i - j], comparators_option::exclude_prerelease));
        }
      }
    }
  }

  SECTION("operator _version") {
    constexpr version v = "1.2.3-rc.4"_version;
    static_assert(v == version{1, 2, 3, prerelease::rc, 4});
  }
}

TEST_CASE("from/to string") {
  constexpr std::array<version, 22> versions = {{
      version{1, 2, 3},
      version{65535, 65535, 65535},
      version{0, 0, 0},
      //
      version{1, 2, 3, prerelease::none, std::nullopt},
      version{1, 2, 3, prerelease::none, 4},
      version{65535, 65535, 65535, prerelease::none, 65535},
      version{0, 0, 0, prerelease::none, std::nullopt},
      //
      version{1, 2, 3, prerelease::alpha, std::nullopt},
      version{1, 2, 3, prerelease::alpha, 0},
      version{1, 2, 3, prerelease::alpha, 4},
      version{65535, 65535, 65535, prerelease::alpha, 65535},
      version{0, 0, 0, prerelease::alpha, std::nullopt},
      //
      version{1, 2, 3, prerelease::beta, std::nullopt},
      version{1, 2, 3, prerelease::beta, 0},
      version{1, 2, 3, prerelease::beta, 4},
      version{65535, 65535, 65535, prerelease::beta, 65535},
      version{0, 0, 0, prerelease::beta, std::nullopt},
      //
      version{1, 2, 3, prerelease::rc, std::nullopt},
      version{1, 2, 3, prerelease::rc, 0},
      version{1, 2, 3, prerelease::rc, 4},
      version{65535, 65535, 65535, prerelease::rc, 65535},
      version{0, 0, 0, prerelease::rc, std::nullopt},
  }};

  constexpr std::array<std::string_view, 22> versions_strings = {{
      "1.2.3",
      "65535.65535.65535",
      "0.0.0",
      //
      "1.2.3",
      "1.2.3",
      "65535.65535.65535",
      "0.0.0",
      //
      "1.2.3-alpha",
      "1.2.3-alpha.0",
      "1.2.3-alpha.4",
      "65535.65535.65535-alpha.65535",
      "0.0.0-alpha",
      //
      "1.2.3-beta",
      "1.2.3-beta.0",
      "1.2.3-beta.4",
      "65535.65535.65535-beta.65535",
      "0.0.0-beta",
      //
      "1.2.3-rc",
      "1.2.3-rc.0",
      "1.2.3-rc.4",
      "65535.65535.65535-rc.65535",
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

TEST_CASE("ranges") {
  SECTION("constructor") {
    constexpr version v1{"1.2.3"};
    constexpr std::string_view r1{">1.0.0 <=2.0.0"};
    STATIC_REQUIRE(range::satisfies(v1, r1));

    constexpr version v2{"2.1.0"};
    STATIC_REQUIRE_FALSE(range::satisfies(v2, r1));

    constexpr std::string_view r2{"1.1.1"};
    constexpr version v3{"1.1.1"};
    STATIC_REQUIRE(range::satisfies(v3, r2));
  }

  struct range_test_case {
    std::string_view range;
    version ver;
    bool contains;
  };

  SECTION("one comparator set") {
    constexpr std::array<range_test_case, 6> tests = {{
      {"> 1.2.3", {1, 2, 5}, true},
      {"> 1.2.3", {1, 1, 0}, false},
      {">=1.2.0 <2.0.0", {1, 2, 5}, true},
      {">=1.2.0 <2.0.0", {2, 3, 0}, false},
      {"1.0.0", {1, 0, 0}, true},
      {"1.0.0 < 2.0.0", {1, 5, 0}, false}
    }};

    for (const auto& test : tests) {
      REQUIRE(range::satisfies(test.ver, test.range) == test.contains);
    }
  }

  SECTION("multiple comparators set") {
    constexpr std::string_view range{"1.2.7 || >=1.2.9 <2.0.0"};
    constexpr version v1{"1.2.7"};
    constexpr version v2{"1.2.9"};
    constexpr version v3{"1.4.6"};
    constexpr version v4{"1.2.8"};
    constexpr version v5{"2.0.0"};

    STATIC_REQUIRE(range::satisfies(v1, range));
    STATIC_REQUIRE(range::satisfies(v2, range));
    STATIC_REQUIRE(range::satisfies(v3, range));
    STATIC_REQUIRE_FALSE(range::satisfies(v4, range));
    STATIC_REQUIRE_FALSE(range::satisfies(v5, range));
  }
}

TEST_CASE("ranges with prerelase tags") {
  SECTION("prerelease tags") {
    constexpr std::string_view r1{">1.2.3-alpha.3"};
    constexpr std::string_view r2{">=1.2.3 < 2.0.0"};
    constexpr std::string_view r3{">=1.2.3-alpha.7 <2.0.0"};
    constexpr std::string_view r4{">1.2.3 <2.0.0-alpha.10"};
    constexpr std::string_view r5{">1.2.3 <2.0.0-alpha.1 || <=2.0.0-alpha.5"};
    constexpr std::string_view r6{"<=2.0.0-alpha.4"};
    constexpr std::string_view r7{">=2.0.0-alpha"};
    constexpr std::string_view r8{"<2.0.0-alpha"};

    constexpr version v1{"1.2.3-alpha.7"};
    constexpr version v2{"3.4.5-alpha.9"};
    constexpr version v3{"3.4.5"};
    constexpr version v4{"1.2.3-alpha.4"};
    constexpr version v5{"2.0.0-alpha.5"};
    constexpr version v6{"2.0.0-alpha.0"};

    SECTION("exclude prerelease") {
      STATIC_REQUIRE(range::satisfies(v1, r1));
      STATIC_REQUIRE_FALSE(range::satisfies(v2, r1));
      STATIC_REQUIRE(range::satisfies(v3, r1));
      STATIC_REQUIRE(range::satisfies(v4, r1));
      STATIC_REQUIRE_FALSE(range::satisfies(v1, r2));
      STATIC_REQUIRE(range::satisfies(v1, r3));
      STATIC_REQUIRE(range::satisfies(v5, r4));
      STATIC_REQUIRE_FALSE(range::satisfies(v1, r4));
      STATIC_REQUIRE(range::satisfies(v5, r5));
      STATIC_REQUIRE_FALSE(range::satisfies(v5, r6));
      STATIC_REQUIRE(range::satisfies(v5, r7));
      STATIC_REQUIRE(range::satisfies(v6, r7));
      STATIC_REQUIRE_FALSE(range::satisfies(v5, r8));
      STATIC_REQUIRE_FALSE(range::satisfies(v6, r8));
    }

    SECTION("include prerelease") {
      STATIC_REQUIRE(range::satisfies(v1, r1, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE(range::satisfies(v2, r1, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE(range::satisfies(v3, r1, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE(range::satisfies(v4, r1, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE_FALSE(range::satisfies(v1, r2, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE(range::satisfies(v1, r3, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE(range::satisfies(v5, r4, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE_FALSE(range::satisfies(v1, r4, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE(range::satisfies(v5, r5, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE_FALSE(range::satisfies(v5, r6, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE(range::satisfies(v5, r7, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE(range::satisfies(v6, r7, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE_FALSE(range::satisfies(v5, r8, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE_FALSE(range::satisfies(v6, r8, range::satisfies_option::include_prerelease));
    }
  }

  SECTION("prelease type comparison") {
    constexpr version v1{"1.0.0-alpha.123"};
    constexpr version v2{"1.0.0-beta.123"};
    constexpr version v3{"1.0.0-rc.123"};

    constexpr std::string_view r1{"<=1.0.0-alpha.123"};
    constexpr std::string_view r2{"<=1.0.0-beta.123"};
    constexpr std::string_view r3{"<=1.0.0-rc.123"};

    SECTION("exclude prerelease") {
      STATIC_REQUIRE(range::satisfies(v1, r1));
      STATIC_REQUIRE_FALSE(range::satisfies(v2, r1));
      STATIC_REQUIRE_FALSE(range::satisfies(v3, r1));

      STATIC_REQUIRE(range::satisfies(v1, r2));
      STATIC_REQUIRE(range::satisfies(v2, r2));
      STATIC_REQUIRE_FALSE(range::satisfies(v3, r2));

      STATIC_REQUIRE(range::satisfies(v1, r3));
      STATIC_REQUIRE(range::satisfies(v2, r3));
      STATIC_REQUIRE(range::satisfies(v3, r3));
    }

    SECTION("include prerelease") {
      STATIC_REQUIRE(range::satisfies(v1, r1, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE_FALSE(range::satisfies(v2, r1, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE_FALSE(range::satisfies(v3, r1, range::satisfies_option::include_prerelease));

      STATIC_REQUIRE(range::satisfies(v1, r2, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE(range::satisfies(v2, r2, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE_FALSE(range::satisfies(v3, r2, range::satisfies_option::include_prerelease));

      STATIC_REQUIRE(range::satisfies(v1, r3, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE(range::satisfies(v2, r3, range::satisfies_option::include_prerelease));
      STATIC_REQUIRE(range::satisfies(v3, r3, range::satisfies_option::include_prerelease));
    }
  }
}
