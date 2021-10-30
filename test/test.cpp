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
#include <string>
#include <string_view>
#include <sstream>

using namespace semver;

static_assert(semver_version.get_major() == SEMVER_VERSION_MAJOR);
static_assert(semver_version.get_minor() == SEMVER_VERSION_MINOR);
static_assert(semver_version.get_patch() == SEMVER_VERSION_PATCH);

// static_assert(alignof(version) == 1);
// static_assert(sizeof(version) == 5);

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
    static_assert(v0.get_major() == 0 &&
                      v0.get_minor() == 1 &&
                      v0.get_patch() == 0 &&
                      v0.get_prerelease().empty() &&
                      v0.get_build_metadata().empty());
  }

  SECTION("constructor") {
    constexpr version v1{1, 2, 3, "rc.0"};
    static_assert(v1.get_major() == 1 &&
                      v1.get_minor() == 2 &&
                      v1.get_patch() == 3 &&
                      detail::compare_equal(v1.get_prerelease(), "rc.0"));

    constexpr version v2{1, 2, 3, "rc.4"};
    static_assert(v2.get_major() == 1 &&
                      v2.get_minor() == 2 &&
                      v2.get_patch() == 3 &&
                      detail::compare_equal(v2.get_prerelease(), "rc.4"));

    constexpr version v3{1, 2, 3};
    static_assert(v3.get_major() == 1 &&
                      v3.get_minor() == 2 &&
                      v3.get_patch() == 3 &&
                      v3.get_prerelease().empty());

    constexpr version v4{1, 2, 3, ""};
    static_assert(v4.get_major() == 1 &&
                      v4.get_minor() == 2 &&
                      v4.get_patch() == 3 &&
                      detail::compare_equal(v4.get_prerelease(), ""));

    constexpr version v5{v4};
    static_assert(v5.get_major() == 1 &&
                      v5.get_minor() == 2 &&
                      v5.get_patch() == 3 &&
                      v5.get_prerelease().empty());

    constexpr version v6{v5};
    static_assert(v6.get_major() == 1 &&
                      v6.get_minor() == 2 &&
                      v6.get_patch() == 3 &&
                      v6.get_prerelease().empty());

    constexpr version v7{"1.2.3-alpha.4"};
    static_assert(v7.get_major() == 1 &&
                      v7.get_minor() == 2 &&
                      v7.get_patch() == 3 &&
                      detail::compare_equal(v7.get_prerelease(), "alpha.4"));

    std::string s = "1.1.1";
    version v10{s};
    REQUIRE(v10.get_major() == 1);
    REQUIRE(v10.get_minor() == 1);
    REQUIRE(v10.get_patch() == 1);
    REQUIRE(v10.get_prerelease().empty());
  }
}

TEST_CASE("operators") {
   constexpr std::array<version, 56> versions = {{
       version{0, 0, 0, "alpha.0"},
       version{0, 0, 0, "alpha.1"},
       version{0, 0, 0, "beta.0"},
       version{0, 0, 0, "beta.1"},
       version{0, 0, 0, "rc.0"},
       version{0, 0, 0, "rc.1"},
       version{0, 0, 0},

       version{0, 0, 1, "alpha.0"},
       version{0, 0, 1, "alpha.1"},
       version{0, 0, 1, "beta.0"},
       version{0, 0, 1, "beta.1"},
       version{0, 0, 1, "rc.0"},
       version{0, 0, 1, "rc.1"},
       version{0, 0, 1},

       version{0, 1, 0, "alpha.0"},
       version{0, 1, 0, "alpha.1"},
       version{0, 1, 0, "beta.0"},
       version{0, 1, 0, "beta.1"},
       version{0, 1, 0, "rc.0"},
       version{0, 1, 0, "rc.1"},
       version{0, 1, 0},

       version{0, 1, 1, "alpha.0"},
       version{0, 1, 1, "alpha.1"},
       version{0, 1, 1, "beta.0"},
       version{0, 1, 1, "beta.1"},
       version{0, 1, 1, "rc.0"},
       version{0, 1, 1, "rc.1"},
       version{0, 1, 1},

       version{1, 0, 0, "alpha.0"},
       version{1, 0, 0, "alpha.1"},
       version{1, 0, 0, "beta.0"},
       version{1, 0, 0, "beta.1"},
       version{1, 0, 0, "rc.0"},
       version{1, 0, 0, "rc.1"},
       version{1, 0, 0},

       version{1, 0, 1, "alpha.0"},
       version{1, 0, 1, "alpha.1"},
       version{1, 0, 1, "beta.0"},
       version{1, 0, 1, "beta.1"},
       version{1, 0, 1, "rc.0"},
       version{1, 0, 1, "rc.1"},
       version{1, 0, 1},

       version{1, 1, 0, "alpha.0"},
       version{1, 1, 0, "alpha.1"},
       version{1, 1, 0, "beta.0"},
       version{1, 1, 0, "beta.1"},
       version{1, 1, 0, "rc.0"},
       version{1, 1, 0, "rc.1"},
       version{1, 1, 0},

       version{1, 1, 1, "alpha.0"},
       version{1, 1, 1, "alpha.1"},
       version{1, 1, 1, "beta.0"},
       version{1, 1, 1, "beta.1"},
       version{1, 1, 1, "rc.0"},
       version{1, 1, 1, "rc.1"},
       version{1, 1, 1},
   }};

   SECTION("operator =") {
     constexpr version v1{1, 2, 3, "rc.4"};
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
     constexpr version v1{1, 2, 3, "rc.4"};
     constexpr version v2{1, 2, 3, "rc.4"};
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
     constexpr version v1{1, 2, 3, "rc.4"};
     constexpr version v2{1, 2, 3};
     STATIC_CHECK_OP_AND_REVERSE(v1, !=, v2);

     for (std::size_t i = 1; i < versions.size(); ++i) {
       for (std::size_t j = 1; j < i; ++j) {
         CHECK_OP_AND_REVERSE(versions[i], !=, versions[i - j]);

         REQUIRE(compare(versions[i], versions[i - j], comparators_option::include_prerelease) != 0);
         REQUIRE(compare(versions[i - j], versions[i], comparators_option::include_prerelease) != 0);
         if ((i - j) / 7 == i / 7) {
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
     constexpr version v1{1, 2, 3, "rc.4"};
     constexpr version v2{1, 2, 3};
     STATIC_CHECK_OP_AND_REVERSE_FALSE(v2, >, v1);

     for (std::size_t i = 1; i < versions.size(); ++i) {
       for (std::size_t j = 1; j < i; ++j) {
         CHECK_OP_AND_REVERSE_FALSE(versions[i], >, versions[i - j]);

         REQUIRE(greater(versions[i], versions[i - j], comparators_option::include_prerelease));
         REQUIRE_FALSE(greater(versions[i - j], versions[i], comparators_option::include_prerelease));
         if ((i - j) / 7 == i / 7) {
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
     constexpr version v1{1, 2, 3, "rc.4"};
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
         if ((i - j) / 7 == i / 7) {
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
     constexpr version v1{1, 2, 3, "rc.4"};
     constexpr version v2{1, 2, 3};
     STATIC_CHECK_OP_AND_REVERSE_FALSE(v1, <, v2);

     for (std::size_t i = 1; i < versions.size(); ++i) {
       for (std::size_t j = 1; j < i; ++j) {
         CHECK_OP_AND_REVERSE_FALSE(versions[i - j], <, versions[i]);

         REQUIRE(less(versions[i - j], versions[i], comparators_option::include_prerelease));
         REQUIRE_FALSE(less(versions[i], versions[i - j], comparators_option::include_prerelease));
         if ((i - j) / 7 == i / 7) {
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
     constexpr version v1{1, 2, 3, "rc.4"};
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
         if ((i - j) / 7 == i / 7) {
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
     static_assert(v == version{1, 2, 3, "rc.4"});
   }
}

TEST_CASE("from/to string") {
  constexpr std::array<std::pair<version, std::string_view>, 32> valid_versions = {{
    {version{0, 0, 4}, "0.0.4"},
    {version{1, 2, 3}, "1.2.3"},
    {version{10, 20, 30}, "10.20.30"},
    {version{1, 1, 2, "prerelease", "meta"}, "1.1.2-prerelease+meta"},
    {version{1, 1, 2, "", "meta" }, "1.1.2+meta"},
    {version{1, 1, 2, "", "meta-valid" }, "1.1.2+meta-valid"},
    {version{1, 0, 0, "alpha"}, "1.0.0-alpha"},
    {version{1, 0, 0, "beta"}, "1.0.0-beta"},
    {version{1, 0, 0, "alpha.beta"}, "1.0.0-alpha.beta"},
    {version{1, 0, 0, "alpha.beta.1"}, "1.0.0-alpha.beta.1"},
    {version{1, 0, 0, "alpha.1"}, "1.0.0-alpha.1"},
    {version{1, 0, 0, "alpha0.valid"}, "1.0.0-alpha0.valid"},
    {version{1, 0, 0, "alpha.0valid"}, "1.0.0-alpha.0valid"},
    {version{1, 0, 0, "alpha-a.b-c-somethinglong", "build.1-aef.1-its-okay"}, "1.0.0-alpha-a.b-c-somethinglong+build.1-aef.1-its-okay"},
    {version{1, 2, 3, "rc.4"}, "1.2.3-rc.4"},
    {version{1, 0, 0, "rc.1", "build.1"}, "1.0.0-rc.1+build.1"},
    {version{2, 0, 0, "rc.1", "build.123"}, "2.0.0-rc.1+build.123"},
    {version{1, 2, 3, "beta"}, "1.2.3-beta"},
    {version{10, 2, 3, "DEV-SNAPSHOT"}, "10.2.3-DEV-SNAPSHOT"},
    {version{1, 2, 3, "SNAPSHOT-123"}, "1.2.3-SNAPSHOT-123"},
    {version{1, 0, 0}, "1.0.0"},
    {version{2, 0, 0}, "2.0.0"},
    {version{1, 1, 7}, "1.1.7"},
    {version{2, 0, 0, "", "build.1848"}, "2.0.0+build.1848"},
    {version{2, 0, 1, "alpha.1227"}, "2.0.1-alpha.1227"},
    {version{1, 0, 0, "alpha", "beta"}, "1.0.0-alpha+beta"},
    {version{1, 2, 3, "---RC-SNAPSHOT.12.9.1--.12", "788"}, "1.2.3----RC-SNAPSHOT.12.9.1--.12+788"},
    {version{1, 2, 3, "---R-S.12.9.1--.12", "meta"}, "1.2.3----R-S.12.9.1--.12+meta"},
    {version{1, 2, 3, "---RC-SNAPSHOT.12.9.1--.12"}, "1.2.3----RC-SNAPSHOT.12.9.1--.12"},
    {version{1, 0, 0, "", "0.build.1-rc.10000aaa-kk-0.1"}, "1.0.0+0.build.1-rc.10000aaa-kk-0.1"},
    {version{999999999, 999999999, 999999999}, "999999999.999999999.999999999"},
    {version{1, 0, 0, "0A.is.legal"}, "1.0.0-0A.is.legal"}
  }};

  constexpr std::array<std::string_view, 45> invalid_versions = {{
    "1",
    "1.2",
    "1.2.3-0123",
    "1.2.3-0123.0123",
    "1.0.0-",
    "1.0.0+",
    "1.0.0-.",
    "1.0.0+.",
    "1.0.0-.+.",
    "1.1.2+.123",
    "+invalid",
    "-invalid",
    "-invalid+invalid",
    "-invalid.01",
    "alpha",
    "alpha.beta",
    "alpha.beta.1",
    "alpha.1",
    "alpha+beta",
    "alpha_beta",
    "alpha.",
    "alpha..",
    "beta",
    "1.0.0-alpha_beta",
    "-alpha.",
    "1.0.0-alpha..",
    "1.0.0-alpha..1",
    "1.0.0-alpha...1",
    "1.0.0-alpha....1",
    "1.0.0-alpha.....1",
    "1.0.0-alpha......1",
    "1.0.0-alpha.......1",
    "01.1.1",
    "1.01.1",
    "1.1.01",
    "1.2",
    "1.2.3.DEV",
    "1.2-SNAPSHOT",
    "1.2.31.2.3----RC-SNAPSHOT.12.09.1--..12+788",
    "1.2-RC-SNAPSHOT",
    "-1.0.3-gamma+b7718",
    "+justmeta",
    "9.8.7+meta+meta",
    "9.8.7-whatever+meta+meta",
    "99999999999999999999999.999999999999999999.99999999999999999----RC-SNAPSHOT.12.09.1--------------------------------..12"
  }};

  SECTION("from chars") {
    for (const auto& [ver, str] : valid_versions) {
      version v;
      const auto result = v.from_chars(str.data(), str.data() + str.size());
      REQUIRE(result);
      REQUIRE(*result.ptr == '\0');
      REQUIRE(v == ver);
    }

    for (const auto& str : invalid_versions) {
      version v;
      REQUIRE_FALSE(v.from_chars(str.data(), str.data() + str.size()));
    }
  }

  SECTION("to chars") {
    for (const auto& [ver, str] : valid_versions) {
      std::array<char, 255> m = {'\0'};
      REQUIRE(ver.to_chars(m.data(), m.data() + m.size()));
      const auto s = std::string_view{m.data()};
      REQUIRE(s == str);
    }
  }

  SECTION("from string") {
    for (const auto& [ver, str] : valid_versions) {
      version v;
      v.from_string(str);
      REQUIRE(v == ver);
    }

     for (const auto& str : invalid_versions) {
      version v;
      REQUIRE_THROWS(v.from_string(str));
    }
  }

  SECTION("from string noexcept") {
    for (const auto& [ver, str] : valid_versions) {
      version v;
      REQUIRE(v.from_string_noexcept(str));
      REQUIRE(v == ver);
    }

     for (const auto& str : invalid_versions) {
      version v;
      REQUIRE_FALSE(v.from_string_noexcept(str));
    }
  }

  SECTION("to string") {
    for (const auto& [ver, str] : valid_versions) {
      const auto s = ver.to_string();
      REQUIRE(s == str);
    }
  }
}

TEST_CASE("validation") {
  SECTION("constexpr prerelease tag validation") {
    constexpr std::string_view pre = "alpha";
    STATIC_REQUIRE(detail::is_prerelease_valid(pre));

    constexpr std::string_view pre2 = "alpha.1";
    STATIC_REQUIRE(detail::is_prerelease_valid(pre2));

    constexpr std::string_view pre3 = "alpha.beta";
    STATIC_REQUIRE(detail::is_prerelease_valid(pre3));

    constexpr std::string_view pre4 = "alpha.beta.1";
    STATIC_REQUIRE(detail::is_prerelease_valid(pre4));

    constexpr std::string_view pre5 = "alpha0.valid";
    STATIC_REQUIRE(detail::is_prerelease_valid(pre5));

    constexpr std::string_view pre6 = "alpha.0valid";
    STATIC_REQUIRE(detail::is_prerelease_valid(pre6));

    constexpr std::string_view pre7 = "somethinglong.1-aef.1-its-okay";
    STATIC_REQUIRE(detail::is_prerelease_valid(pre7));

    constexpr std::string_view pre8 = "---R-S.12.9.1--.12";
    STATIC_REQUIRE(detail::is_prerelease_valid(pre8));

    constexpr std::string_view v_str = "1.2.3-alpha.3";
    constexpr std::string_view pre9 = {v_str.data() + 6, 7};
    STATIC_REQUIRE(detail::is_prerelease_valid(pre9));

    constexpr std::string_view pre10 = "%alpha%";
    STATIC_REQUIRE(!detail::is_prerelease_valid(pre10));

    constexpr std::string_view pre11 = "rc*123";
    STATIC_REQUIRE(!detail::is_prerelease_valid(pre11));

    constexpr std::string_view pre12 = "tag_with_underscores";
    STATIC_REQUIRE(!detail::is_prerelease_valid(pre12));

    constexpr std::string_view pre13 = "dev/snapshot-1.2.3";
    STATIC_REQUIRE(!detail::is_prerelease_valid(pre13));

    constexpr std::string_view pre14 = "alpha.beta@146";
    STATIC_REQUIRE(!detail::is_prerelease_valid(pre14));
  }

  SECTION("prerelease tag validation") {
    constexpr std::array<std::string_view, 20> valid_prerelease_tags = {{
      "prerelease",
      "alpha",
      "beta",
      "alpha.beta",
      "alpha.beta.1",
      "alpha.1",
      "alpha0.valid",
      "alpha.0valid",
      "alpha-a.b-c-somethinglong",
      "rc.4",
      "1.2.3-beta",
      "DEV-SNAPSHOT",
      "SNAPSHOT-123",
      "1227",
      "---RC-SNAPSHOT.12.9.1--.12",
      "---R-S.12.9.1--.12",
      "----RC-SNAPSHOT.12.9.1--.12",
      "-rc.10000aaa-kk-0.1",
      "999999999.999999999.999999999",
      "1.0.0-0A.is.legal"
    }};

    for (const auto& prerelease: valid_prerelease_tags) {
      version v;
      REQUIRE(v.set_prerelease_noexcept(prerelease));
      REQUIRE(detail::compare_equal(prerelease, v.get_prerelease()));
    }

    constexpr std::array<std::string_view, 23> invalid_prerelease_tags = {{
      "%alpha%",
      "rc*123",
      "tag_with_underscores",
      "dev/snapshot-1.2.3",
      "alpha.beta@146",
      "--rc.1.best_release_ever",
      "___main.01",
      "!tag",
      "^caret^tag^",
      "{alpha.beta.rc.0}",
      "alpha;beta.1",
      "////|\\\\",
      "$dev.0001",
      "beta#123",
      "snapshot№1",
      "???",
      "beta&alpha&rc",
      "<some.tag>",
      ">_<",
      "(((ololo)))",
      ":prerelease:",
      ".",
      "alpha."
    }};

    for (const auto& prerelease: invalid_prerelease_tags) {
      version v;
      REQUIRE(!v.set_prerelease_noexcept(prerelease));
      REQUIRE(v.get_prerelease().empty());
    }
  }

  SECTION("constexpr build_metadata tag validation") {
    constexpr std::string_view bm = "build";
    STATIC_REQUIRE(detail::is_build_metadata_valid(bm));

    constexpr std::string_view bm2 = "meta-valid";
    STATIC_REQUIRE(detail::is_build_metadata_valid(bm2));

    constexpr std::string_view bm3 = "build.1-aef.1-its-okay";
    STATIC_REQUIRE(detail::is_build_metadata_valid(bm3));

    constexpr std::string_view bm4 = "build.1";
    STATIC_REQUIRE(detail::is_build_metadata_valid(bm4));

    constexpr std::string_view bm5 = "build.123";
    STATIC_REQUIRE(detail::is_build_metadata_valid(bm5));

    constexpr std::string_view bm6 = "-DEV-SNAPSHOT";
    STATIC_REQUIRE(detail::is_build_metadata_valid(bm6));

    constexpr std::string_view bm7 = "788";
    STATIC_REQUIRE(detail::is_build_metadata_valid(bm7));

    constexpr std::string_view bm8 = "999999999.999999999.999999999";
    STATIC_REQUIRE(detail::is_build_metadata_valid(bm8));

    constexpr std::string_view bm9 = "+build.1848";
    STATIC_REQUIRE(!detail::is_build_metadata_valid(bm9));

    constexpr std::string_view bm10 = "alpha+beta";
    STATIC_REQUIRE(!detail::is_build_metadata_valid(bm10));

    constexpr std::string_view bm11 = "build++";
    STATIC_REQUIRE(!detail::is_build_metadata_valid(bm11));

    constexpr std::string_view bm12 = "some.build!";
    STATIC_REQUIRE(!detail::is_build_metadata_valid(bm12));

    constexpr std::string_view bm13 = "some.build?";
    STATIC_REQUIRE(!detail::is_build_metadata_valid(bm13));

    constexpr std::string_view bm14 = "//thebestbuild";
    STATIC_REQUIRE(!detail::is_build_metadata_valid(bm14));

    constexpr std::string_view bm15 = "meta_2021_10_26";
    STATIC_REQUIRE(!detail::is_build_metadata_valid(bm15));
  }

  SECTION("build_metadata tag validation") {
    constexpr std::array<std::string_view, 19> valid_build_metadata = {{
      "build",
      "meta-valid",
      "build.1-aef.1-its-okay",
      "build.1",
      "build.123",
      "-DEV-SNAPSHOT",
      "build.1848",
      "alpha.beta",
      "788",
      "999999999.999999999.999999999",
      "exp.sha.5114f85",
      "-",
      "build-meta-data",
      "my.cat.is.very.cute",
      "000001",
      "we.cant.use.plus.sign",
      "--and-hyphen-too-",
      "-123-456--",
      "21AF26D3--117B344092BD"
    }};

    for (const auto& bm: valid_build_metadata) {
      version v;
      REQUIRE(v.set_build_metadata_noexcept(bm));
      REQUIRE(detail::compare_equal(bm, v.get_build_metadata()));
    }

    constexpr std::array<std::string_view, 24> invalid_build_metadata = {{
      "some.build!",
      "some.build?",
      "//thebestbuild",
      "meta_2021_10_26",
      "%meta_dAtA%",
      "мета",
      "we&have&some&build",
      "cat*has*generated*this*meta",
      "_notvalidbuildmeta",
      "#",
      "#123",
      "no/",
      "you-shall-not-pass!",
      "(this-meta-invalid-too)",
      "[some-data-001]",
      "<and.this.invalid>",
      ">>>",
      "build@1937",
      "build^extrainfo",
      "additional;info;",
      "...",
      "build.",
      ".build",
      "build.."
    }};

    for (const auto& bm: invalid_build_metadata) {
      version v;
      REQUIRE(!v.set_build_metadata_noexcept(bm));
      REQUIRE(v.get_build_metadata().empty());
    }
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

// TEST_CASE("ranges with prerelase tags") {
//   SECTION("prerelease tags") {
//     constexpr std::string_view r1{">1.2.3-alpha.3"};
//     constexpr std::string_view r2{">=1.2.3 < 2.0.0"};
//     constexpr std::string_view r3{">=1.2.3-alpha.7 <2.0.0"};
//     constexpr std::string_view r4{">1.2.3 <2.0.0-alpha.10"};
//     constexpr std::string_view r5{">1.2.3 <2.0.0-alpha.1 || <=2.0.0-alpha.5"};
//     constexpr std::string_view r6{"<=2.0.0-alpha.4"};

//     constexpr version v1{"1.2.3-alpha.7"};
//     constexpr version v2{"3.4.5-alpha.9"};
//     constexpr version v3{"3.4.5"};
//     constexpr version v4{"1.2.3-alpha.4"};
//     constexpr version v5{"2.0.0-alpha.5"};

//     SECTION("exclude prerelease") {
//       STATIC_REQUIRE(range::satisfies(v1, r1));
//       STATIC_REQUIRE_FALSE(range::satisfies(v2, r1));
//       STATIC_REQUIRE(range::satisfies(v3, r1));
//       STATIC_REQUIRE(range::satisfies(v4, r1));
//       STATIC_REQUIRE_FALSE(range::satisfies(v1, r2));
//       STATIC_REQUIRE(range::satisfies(v1, r3));
//       STATIC_REQUIRE(range::satisfies(v5, r4));
//       STATIC_REQUIRE_FALSE(range::satisfies(v1, r4));
//       STATIC_REQUIRE(range::satisfies(v5, r5));
//       STATIC_REQUIRE_FALSE(range::satisfies(v5, r6));
//     }

//     SECTION("include prerelease") {
//       STATIC_REQUIRE(range::satisfies(v1, r1, range::satisfies_option::include_prerelease));
//       STATIC_REQUIRE(range::satisfies(v2, r1, range::satisfies_option::include_prerelease));
//       STATIC_REQUIRE(range::satisfies(v3, r1, range::satisfies_option::include_prerelease));
//       STATIC_REQUIRE(range::satisfies(v4, r1, range::satisfies_option::include_prerelease));
//       STATIC_REQUIRE_FALSE(range::satisfies(v1, r2, range::satisfies_option::include_prerelease));
//       STATIC_REQUIRE(range::satisfies(v1, r3, range::satisfies_option::include_prerelease));
//       STATIC_REQUIRE(range::satisfies(v5, r4, range::satisfies_option::include_prerelease));
//       STATIC_REQUIRE_FALSE(range::satisfies(v1, r4, range::satisfies_option::include_prerelease));
//       STATIC_REQUIRE(range::satisfies(v5, r5, range::satisfies_option::include_prerelease));
//       STATIC_REQUIRE_FALSE(range::satisfies(v5, r6, range::satisfies_option::include_prerelease));
//     }
//   }

//   SECTION("prelease type comparison") {
//     constexpr version v1{"1.0.0-alpha.123"};
//     constexpr version v2{"1.0.0-beta.123"};
//     constexpr version v3{"1.0.0-rc.123"};

//     constexpr std::string_view r1{"<=1.0.0-alpha.123"};
//     constexpr std::string_view r2{"<=1.0.0-beta.123"};
//     constexpr std::string_view r3{"<=1.0.0-rc.123"};

//     SECTION("exclude prerelease") {
//       STATIC_REQUIRE(range::satisfies(v1, r1));
//       STATIC_REQUIRE_FALSE(range::satisfies(v2, r1));
//       STATIC_REQUIRE_FALSE(range::satisfies(v3, r1));

//       STATIC_REQUIRE(range::satisfies(v1, r2));
//       STATIC_REQUIRE(range::satisfies(v2, r2));
//       STATIC_REQUIRE_FALSE(range::satisfies(v3, r2));

//       STATIC_REQUIRE(range::satisfies(v1, r3));
//       STATIC_REQUIRE(range::satisfies(v2, r3));
//       STATIC_REQUIRE(range::satisfies(v3, r3));
//     }

//     SECTION("include prerelease") {
//       STATIC_REQUIRE(range::satisfies(v1, r1, range::satisfies_option::include_prerelease));
//       STATIC_REQUIRE_FALSE(range::satisfies(v2, r1, range::satisfies_option::include_prerelease));
//       STATIC_REQUIRE_FALSE(range::satisfies(v3, r1, range::satisfies_option::include_prerelease));

//       STATIC_REQUIRE(range::satisfies(v1, r2, range::satisfies_option::include_prerelease));
//       STATIC_REQUIRE(range::satisfies(v2, r2, range::satisfies_option::include_prerelease));
//       STATIC_REQUIRE_FALSE(range::satisfies(v3, r2, range::satisfies_option::include_prerelease));

//       STATIC_REQUIRE(range::satisfies(v1, r3, range::satisfies_option::include_prerelease));
//       STATIC_REQUIRE(range::satisfies(v2, r3, range::satisfies_option::include_prerelease));
//       STATIC_REQUIRE(range::satisfies(v3, r3, range::satisfies_option::include_prerelease));
//     }
//   }
// }
