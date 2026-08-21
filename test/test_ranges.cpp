// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2026 Daniil Goncharov <neargye@gmail.com>.

#include <doctest.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <forward_list>
#include <limits>
#include <ostream>
#include <string>
#include <utility>
#include <vector>
#include <semver.hpp>

static void test_parse_and_check(std::string_view range_str, std::string_view ver_str,
  semver::prerelease_policy policy = semver::prerelease_policy::exclude)
{
  semver::version v;
  REQUIRE(semver::parse(ver_str, v));

  semver::range_set rs;
  REQUIRE(semver::parse(range_str, rs));

  REQUIRE(rs.contains(v, policy));
}

static void test_parse_and_check_false(std::string_view range_str, std::string_view ver_str,
  semver::prerelease_policy policy = semver::prerelease_policy::exclude)
{
  semver::version v;
  REQUIRE(semver::parse(ver_str, v));

  semver::range_set rs;
  REQUIRE(semver::parse(range_str, rs));

  REQUIRE_FALSE(rs.contains(v, policy));
}

TEST_CASE("ranges") {
  SUBCASE("default range matches nothing and empty input is invalid") {
    semver::range_set<> rs;
    REQUIRE_FALSE(rs.contains(semver::version<>{1, 2, 3}));

    constexpr std::array<std::string_view, 3> invalid = {{"", " ", "\t\r\n"}};
    for (const auto range : invalid) {
      const auto result = semver::parse(range, rs);
      REQUIRE_FALSE(result);
      REQUIRE(result.ptr == range.data() + range.size());
      REQUIRE_FALSE(rs.contains(semver::version<>{1, 2, 3}));
    }

    const std::string_view empty;
    const auto result = semver::parse(empty, rs);
    REQUIRE_FALSE(result);
    REQUIRE(result.ptr == empty.data());
  }

  SUBCASE("constructor") {
    constexpr std::string_view v1{"1.2.3"};
    constexpr std::string_view r1{">1.0.0 <=2.0.0"};
    test_parse_and_check(r1, v1);

    constexpr std::string_view v2{"2.1.0"};
    test_parse_and_check_false(r1, v2);

    constexpr std::string_view r2{"1.1.1"};
    constexpr std::string_view v3{"1.1.1"};
    test_parse_and_check(r2, v3);
  }

  struct range_test_case {
    std::string_view range;
    std::string_view ver;
    bool contains;
  };

  SUBCASE("one comparator set") {
    constexpr std::array<range_test_case, 6> tests = {{
      {"> 1.2.3", {"1.2.5"}, true},
      {"> 1.2.3", {"1.1.0"}, false},
      {">=1.2.0 <2.0.0", {"1.2.5"}, true},
      {">=1.2.0 <2.0.0", {"2.3.0"}, false},
      {"1.0.0", {"1.0.0"}, true},
      {"1.0.0 < 2.0.0", {"1.5.0"}, false}
    }};

    for (const auto& test : tests) {
      if (test.contains) {
        test_parse_and_check(test.range, test.ver);
      }
      else {
        test_parse_and_check_false(test.range, test.ver);
      }
    }
  }

  SUBCASE("multiple comparators set") {
    constexpr std::string_view range{"1.2.7 || >=1.2.9 <2.0.0"};
    constexpr std::string_view v1{"1.2.7"};
    constexpr std::string_view v2{"1.2.9"};
    constexpr std::string_view v3{"1.4.6"};
    constexpr std::string_view v4{"1.2.8"};
    constexpr std::string_view v5{"2.0.0"};

    test_parse_and_check(range, v1);
    test_parse_and_check(range, v2);
    test_parse_and_check(range, v3);
    test_parse_and_check_false(range, v4);
    test_parse_and_check_false(range, v5);
  }

  SUBCASE("wildcard aliases are terms in intersections") {
    test_parse_and_check(">=1 x", "1.2.3");
    test_parse_and_check_false(">=1 x", "0.9.9");
    test_parse_and_check(">=1 X <2", "1.5.0");
    test_parse_and_check_false(">=1 X <2", "2.0.0");
    test_parse_and_check("* x X", "42.0.0");
  }

  SUBCASE("partial lower-inclusive and upper-exclusive comparators") {
    constexpr std::array<range_test_case, 8> tests = {{
      {">=1", "1.0.0", true},
      {">=1", "0.9.9", false},
      {">=1.2", "1.2.0", true},
      {">=1.2", "1.1.99", false},
      {"<1.2", "1.1.99", true},
      {"<1.2", "1.2.0", false},
      {"<2", "1.99.99", true},
      {"<2", "2.0.0", false}
    }};

    for (const auto& test : tests) {
      if (test.contains)
        test_parse_and_check(test.range, test.ver);
      else
        test_parse_and_check_false(test.range, test.ver);
    }
  }

  SUBCASE("not-equal comparator excludes one complete version") {
    test_parse_and_check(">=1 <2 !=1.5.0", "1.4.9");
    test_parse_and_check_false(">=1 <2 !=1.5.0", "1.5.0");
    test_parse_and_check(">=1 <2 !=1.5.0", "1.5.1");
    test_parse_and_check_false("!= 1.2.3", "1.2.3+build.7");

    test_parse_and_check_false("!=1.2.3-alpha", "1.2.3-alpha");
    test_parse_and_check_false("!=1.2.3-alpha", "1.2.3-beta");
    test_parse_and_check_false("!=1.2.3-alpha", "1.2.3-alpha", semver::prerelease_policy::include);
    test_parse_and_check("!=1.2.3-alpha", "1.2.3-beta", semver::prerelease_policy::include);
    test_parse_and_check_false("!=1.2.3-alpha", "1.2.4-beta");
    test_parse_and_check("!=1.2.3-alpha", "1.2.4-beta", semver::prerelease_policy::include);

    constexpr auto prerelease_range = ">=1.2.3-alpha <1.2.3 !=1.2.3-beta";
    test_parse_and_check(prerelease_range, "1.2.3-alpha");
    test_parse_and_check_false(prerelease_range, "1.2.3-beta");
    test_parse_and_check(prerelease_range, "1.2.3-rc");
  }

  SUBCASE("not-equal requires a complete version") {
    constexpr std::array<std::string_view, 5> invalid = {{"!1.2.3", "!", "!=", "!=1", "!=1.2"}};
    for (const auto range : invalid) {
      semver::range_set<> rs;
      REQUIRE_FALSE(semver::parse(range, rs));
    }
  }

  SUBCASE("all ASCII whitespace separates comparators") {
    constexpr std::array<char, 6> whitespace = {{' ', '\t', '\n', '\r', '\v', '\f'}};
    for (const auto ws : whitespace) {
      const auto comparator_range = std::string{">=1"} + ws + "<2";
      test_parse_and_check(comparator_range, "1.5.0");
    }
  }
}

TEST_CASE("ranges with prerelease tags") {
  SUBCASE("prerelease tags") {
    constexpr std::string_view r1{">1.2.3-alpha.3"};
    constexpr std::string_view r2{">=1.2.3 < 2.0.0"};
    constexpr std::string_view r3{">=1.2.3-alpha.7 <2.0.0"};
    constexpr std::string_view r4{">1.2.3 <2.0.0-alpha.10"};
    constexpr std::string_view r5{">1.2.3 <2.0.0-alpha.1 || <=2.0.0-alpha.5"};
    constexpr std::string_view r6{"<=2.0.0-alpha.4"};

    constexpr std::string_view v1{"1.2.3-alpha.7"};
    constexpr std::string_view v2{"3.4.5-alpha.9"};
    constexpr std::string_view v3{"3.4.5"};
    constexpr std::string_view v4{"1.2.3-alpha.4"};
    constexpr std::string_view v5{"2.0.0-alpha.5"};

    SUBCASE("exclude prerelease") {
      test_parse_and_check(r1, v1);
      test_parse_and_check_false(r1, v2);
      test_parse_and_check(r1, v3);
      test_parse_and_check(r1, v4);
      test_parse_and_check_false(r2, v1);
      test_parse_and_check(r3, v1);
      test_parse_and_check(r4, v5);
      test_parse_and_check_false(r4, v1);
      test_parse_and_check(r5, v5);
      test_parse_and_check_false(r6, v5);
    }

    SUBCASE("include prerelease") {
      test_parse_and_check(r1, v1, semver::prerelease_policy::include);
      test_parse_and_check(r1, v2, semver::prerelease_policy::include);
      test_parse_and_check(r1, v3, semver::prerelease_policy::include);
      test_parse_and_check(r1, v4, semver::prerelease_policy::include);
      test_parse_and_check_false(r2, v1, semver::prerelease_policy::include);
      test_parse_and_check(r3, v1, semver::prerelease_policy::include);
      test_parse_and_check(r4, v5, semver::prerelease_policy::include);
      test_parse_and_check_false(r4, v1, semver::prerelease_policy::include);
      test_parse_and_check(r5, v5, semver::prerelease_policy::include);
      test_parse_and_check_false(r6, v5, semver::prerelease_policy::include);
    }
  }

  SUBCASE("prerelease type comparison") {
    constexpr std::string_view v1{"1.0.0-alpha.123"};
    constexpr std::string_view v2{"1.0.0-beta.123"};
    constexpr std::string_view v3{"1.0.0-rc.123"};

    constexpr std::string_view r1{"<=1.0.0-alpha.123"};
    constexpr std::string_view r2{"<=1.0.0-beta.123"};
    constexpr std::string_view r3{"<=1.0.0-rc.123"};

    SUBCASE("exclude prerelease") {
      test_parse_and_check(r1, v1);
      test_parse_and_check_false(r1, v2);
      test_parse_and_check_false(r1, v3);

      test_parse_and_check(r2, v1);
      test_parse_and_check(r2, v2);
      test_parse_and_check_false(r2, v3);

      test_parse_and_check(r3, v1);
      test_parse_and_check(r3, v2);
      test_parse_and_check(r3, v3);
    }

    SUBCASE("include prerelease") {
      test_parse_and_check(r1, v1, semver::prerelease_policy::include);
      test_parse_and_check_false(r1, v2, semver::prerelease_policy::include);
      test_parse_and_check_false(r1, v3, semver::prerelease_policy::include);

      test_parse_and_check(r2, v1, semver::prerelease_policy::include);
      test_parse_and_check(r2, v2, semver::prerelease_policy::include);
      test_parse_and_check_false(r2, v3, semver::prerelease_policy::include);

      test_parse_and_check(r3, v1, semver::prerelease_policy::include);
      test_parse_and_check(r3, v2, semver::prerelease_policy::include);
      test_parse_and_check(r3, v3, semver::prerelease_policy::include);
    }
  }
}

TEST_CASE("range parse failures") {
  semver::range_set rs;

  SUBCASE("operator without version") {
    constexpr std::array<std::string_view, 5> invalid = {{"<", "<=", ">", ">=", "="}};
    for (const auto range : invalid)
      REQUIRE_FALSE(semver::parse(range, rs));
  }

  SUBCASE("malformed operator combinations are rejected") {
    constexpr std::array<std::string_view, 6> invalid = {{"==1.2.3", "=>1.2.3", "=<1.2.3", ">>1.2.3", "<<1.2.3", "!1.2.3"}};
    for (const auto range : invalid)
      REQUIRE_FALSE(semver::parse(range, rs));
  }

  SUBCASE("legacy range aliases are rejected") {
    REQUIRE_FALSE(semver::parse("~>1.2.3", rs));
    REQUIRE_FALSE(semver::parse("v1.2.3", rs));
    REQUIRE_FALSE(semver::parse("=v1.2.3", rs));
  }

  SUBCASE("dangling union separator") {
    REQUIRE_FALSE(semver::parse(">=1.0.0 ||", rs));
  }

  SUBCASE("isolated or misplaced union separators") {
    REQUIRE_FALSE(semver::parse("|", rs));
    REQUIRE_FALSE(semver::parse("||", rs));
    REQUIRE_FALSE(semver::parse("|| >=1.0.0", rs));
    REQUIRE_FALSE(semver::parse(">=1.0.0 || || <2.0.0", rs));
  }

  SUBCASE("non-ASCII and embedded null are rejected") {
    REQUIRE_FALSE(semver::parse("1.2.3-\xCE\xB2", rs));

    constexpr char embedded_null[] = "1.2.3\0||2.0.0";
    REQUIRE_FALSE(semver::parse(std::string_view{embedded_null, sizeof(embedded_null) - 1}, rs));
  }

  SUBCASE("comparator sets require spaces between adjacent comparators") {
    REQUIRE_FALSE(semver::parse(">=1.0.0<2.0.0", rs));
    REQUIRE_FALSE(semver::parse("1.2.3>=1.0.0", rs));
    REQUIRE_FALSE(semver::parse("1.*2.0.0", rs));
    REQUIRE_FALSE(semver::parse("1.2.*2.0.0", rs));
    REQUIRE_FALSE(semver::parse("~1.2.3<2.0.0", rs));
    REQUIRE_FALSE(semver::parse("^1.2.3<2.0.0", rs));
  }

  SUBCASE("logical-or remains valid without surrounding spaces") {
    REQUIRE(semver::parse("1.2.3||2.0.0", rs));
  }

  SUBCASE("leading and trailing whitespace is accepted") {
    REQUIRE(semver::parse(" >=1.0.0", rs));
    REQUIRE(semver::parse(">=1.0.0\t", rs));
  }

  SUBCASE("hyphen ranges are not part of the range grammar") {
    REQUIRE_FALSE(semver::parse("1.2.3 - 2.0.0", rs));
    REQUIRE_FALSE(semver::parse("1.2 - 2", rs));
    REQUIRE_FALSE(semver::parse("1.2- 2.0.0", rs));
    REQUIRE_FALSE(semver::parse("1.2 -2.0.0", rs));
  }

  SUBCASE("trailing garbage after valid comparator") {
    REQUIRE_FALSE(semver::parse(">=1.0.0 abc", rs));
    REQUIRE_FALSE(semver::parse("<=2.0.0 ???", rs));
  }

  SUBCASE("trailing garbage after valid union") {
    REQUIRE_FALSE(semver::parse(">=1.0.0 || <2.0.0 tail", rs));
  }

  SUBCASE("tilde or caret without a following version") {
    REQUIRE_FALSE(semver::parse("~",  rs));
    REQUIRE_FALSE(semver::parse("^",  rs));
    REQUIRE_FALSE(semver::parse("~ ", rs));
    REQUIRE_FALSE(semver::parse("^ ", rs));
  }

  SUBCASE("leading zeros rejected in advanced range versions") {
    REQUIRE_FALSE(semver::parse("~01.2.3", rs));
    REQUIRE_FALSE(semver::parse("~1.02.3", rs));
    REQUIRE_FALSE(semver::parse("^01.0.0", rs));
  }

  SUBCASE("a dot in a partial version requires a following component") {
    REQUIRE_FALSE(semver::parse("1.", rs));
    REQUIRE_FALSE(semver::parse("1.2.", rs));
    REQUIRE_FALSE(semver::parse("~1.", rs));
    REQUIRE_FALSE(semver::parse("~1.2.", rs));
    REQUIRE_FALSE(semver::parse("^1.", rs));
    REQUIRE_FALSE(semver::parse("^1.2.", rs));
  }

  SUBCASE("wildcards and qualifiers are rejected outside supported positions") {
    constexpr std::array<std::string_view, 15> invalid = {{
      "*.1", "1.*.3", "1.2.*-alpha", "~*", "~1.*", "^*", "^1.2.*",
      "x.1", "1.x.3", "1.2.X-alpha", "~x", "^1.x",
      "1.2-alpha", "1.2+build", "1.2.x+build"
    }};
    for (const auto range : invalid)
      REQUIRE_FALSE(semver::parse(range, rs));
  }

  SUBCASE("ambiguous partial comparators are rejected") {
    constexpr std::array<std::string_view, 6> invalid = {{">1", ">1.2", "<=1", "<=1.2", "=1", "=1.2"}};
    for (const auto range : invalid)
      REQUIRE_FALSE(semver::parse(range, rs));
  }

  SUBCASE("invalid prerelease qualifiers are rejected in all range forms") {
    REQUIRE_FALSE(semver::parse("^1.2.3-01", rs));
    REQUIRE_FALSE(semver::parse("~1.2.3-alpha.", rs));
    REQUIRE_FALSE(semver::parse("1.2.3-01", rs));
  }

  SUBCASE("build metadata is not accepted in range boundaries") {
    REQUIRE_FALSE(semver::parse("1.2.3+build", rs));
    REQUIRE_FALSE(semver::parse(">=1.2.3+build", rs));
    REQUIRE_FALSE(semver::parse("^1.2.3+build", rs));
    REQUIRE_FALSE(semver::parse("^1.2.3+", rs));
    REQUIRE_FALSE(semver::parse("^1.2.3+build.", rs));
    REQUIRE_FALSE(semver::parse("1.2.3+", rs));
  }

  SUBCASE("wildcards are rejected after operators") {
    REQUIRE_FALSE(semver::parse(">=*", rs));
    REQUIRE_FALSE(semver::parse(">=1.*", rs));
    REQUIRE_FALSE(semver::parse("<1.2.*", rs));
    REQUIRE_FALSE(semver::parse("<=x", rs));
    REQUIRE_FALSE(semver::parse(">X", rs));
  }

  SUBCASE("typed component overflow is reported for every range form") {
    constexpr std::array<std::string_view, 28> invalid = {{
      "~256", "~1.256", "~1.2.256", "^256", "^1.256", "^1.2.256",
      "256.*", "1.256.*", "256.0.0", "1.256.0", "1.2.256",
      ">=256", ">=256.0.0", ">=1.256.0", ">=1.2.256",
      "18446744073709551616.0.0", "1.18446744073709551616.0",
      "1.2.18446744073709551616",
      "255", "255.*", "1.255", "1.255.*", "255.255.*", "~255", "~1.255",
      "^255", "^0.255", "^0.0.255"
    }};
    semver::range_set<std::uint8_t> small;
    REQUIRE(semver::parse("1.*", small));

    for (const auto range : invalid) {
      const auto result = semver::parse(range, small);
      REQUIRE_FALSE(result);
      REQUIRE(result.ec == std::errc::result_out_of_range);
      REQUIRE(small.contains(semver::version<std::uint8_t>{1, 2, 3}));
    }
  }
}

TEST_CASE("try_parse_range returns an optional range") {
  const auto parsed = semver::try_parse_range("1.x || 3.2.X");
  REQUIRE(parsed);
  CHECK(parsed->contains(semver::version<>{1, 9, 0}));
  CHECK(parsed->contains(semver::version<>{3, 2, 7}));
  CHECK_FALSE(parsed->contains(semver::version<>{2, 0, 0}));
  CHECK_FALSE(semver::try_parse_range("1.x ||"));
  CHECK_FALSE(semver::try_parse_range<std::uint8_t>("256.x"));
}

TEST_CASE("range from_chars_result contract") {
  SUBCASE("ptr points past last consumed byte on success") {
    semver::range_set rs;
    constexpr std::string_view sv = ">=1.0.0 <2.0.0 || 3.0.0";
    const auto [ptr, ec] = semver::parse(sv, rs);
    REQUIRE(ec == std::errc{});
    REQUIRE(ptr == sv.data() + sv.size());
  }

  SUBCASE("errc::invalid_argument for structurally malformed input") {
    semver::range_set rs;
    const auto result = semver::parse("||", rs);
    REQUIRE_FALSE(result);
    REQUIRE(result.ec == std::errc::invalid_argument);
  }

  SUBCASE("ptr points to first invalid byte after a valid range prefix") {
    semver::range_set rs;
    constexpr std::string_view bad = ">=1.0.0 abc";
    const auto [ptr, ec] = semver::parse(bad, rs);
    REQUIRE(ec == std::errc::invalid_argument);
    REQUIRE(ptr == bad.data() + 8);
  }

  SUBCASE("ptr reports the earliest parse failure") {
    semver::range_set rs;
    constexpr std::string_view structural = ">=1..2@";
    auto result = semver::parse(structural, rs);
    REQUIRE(result.ec == std::errc::invalid_argument);
    REQUIRE(result.ptr == structural.data() + 4);

    constexpr std::string_view trailing = ">=1.0.0 tail@";
    result = semver::parse(trailing, rs);
    REQUIRE(result.ec == std::errc::invalid_argument);
    REQUIRE(result.ptr == trailing.data() + 8);
  }

  SUBCASE("typed overflow reports the failing union term") {
    semver::range_set<std::uint8_t> rs;
    const semver::version<std::uint8_t> preserved{2, 0, 0};
    REQUIRE(semver::parse(">=2", rs));

    constexpr std::string_view overflow = "1 || >=256";
    const auto result = semver::parse(overflow, rs);
    REQUIRE_FALSE(result);
    REQUIRE(result.ec == std::errc::result_out_of_range);
    REQUIRE(result.ptr == overflow.data() + 5);
    REQUIRE(rs.contains(preserved));
  }

  SUBCASE("failed parse leaves output unchanged") {
    semver::range_set rs;
    semver::version v;
    REQUIRE(semver::parse(">=1.0.0", rs));
    REQUIRE(semver::parse("2.0.0", v));
    REQUIRE(rs.contains(v));

    const auto result = semver::parse("broken", rs);
    REQUIRE_FALSE(result);
    REQUIRE(result.ec == std::errc::invalid_argument);
    REQUIRE(rs.contains(v)); // rs still holds >=1.0.0.
  }

  SUBCASE("lexer failure preserves the previous range prefix") {
    semver::range_set rs;
    semver::version v;
    REQUIRE(semver::parse(">=1.0.0 <2.0.0", rs));
    REQUIRE(semver::parse("1.5.0", v));
    REQUIRE(rs.contains(v));

    const auto result = semver::parse(">=1.0.0,<2.0.0", rs);
    REQUIRE_FALSE(result);
    REQUIRE(result.ec == std::errc::invalid_argument);
    REQUIRE(rs.contains(v));
  }

  SUBCASE("trailing garbage does not modify output") {
    semver::range_set rs;
    semver::version v;
    REQUIRE(semver::parse("1.5.0", v));

    const auto result = semver::parse(">=1.0.0 <2.0.0 tail", rs);
    REQUIRE_FALSE(result);
    REQUIRE(result.ec == std::errc::invalid_argument);
    REQUIRE_FALSE(rs.contains(v)); // rs remains empty.
  }
}

TEST_CASE("parse(range_set) transactional guarantee") {
  semver::range_set rs;
  semver::version v;
  REQUIRE(semver::parse(">=1.0.0 <2.0.0", rs));
  REQUIRE(semver::parse("1.5.0", v));
  REQUIRE(rs.contains(v));

  SUBCASE("failure retains a previously valid range_set") {
    REQUIRE_FALSE(semver::parse(">=3.0.0 <4.0.0 !!", rs));
    REQUIRE(rs.contains(v)); // old value preserved, not corrupted
  }

  SUBCASE("success overwrites out atomically") {
    REQUIRE(semver::parse(">=2.0.0", rs));
    REQUIRE_FALSE(rs.contains(v)); // new range replaces old
  }

  SUBCASE("trailing garbage does not partially update out") {
    const auto result = semver::parse(">=3.0.0 <4.0.0 garbage", rs);
    REQUIRE_FALSE(result);
    REQUIRE(rs.contains(v)); // old value unchanged
  }
}

TEST_CASE("range boundary conditions") {
  SUBCASE("exact version as range") {
    test_parse_and_check("1.0.0", "1.0.0");
    test_parse_and_check_false("1.0.0", "1.0.1");
    test_parse_and_check_false("1.0.0", "0.9.9");
  }

  SUBCASE("lower-inclusive upper-exclusive [>=, <)") {
    test_parse_and_check(">=1.0.0 <2.0.0", "1.0.0"); // left boundary included
    test_parse_and_check(">=1.0.0 <2.0.0", "1.9.9"); // interior
    test_parse_and_check_false(">=1.0.0 <2.0.0", "2.0.0"); // right boundary excluded
    test_parse_and_check_false(">=1.0.0 <2.0.0", "0.9.9"); // below range
  }

  SUBCASE("lower-exclusive (>)") {
    test_parse_and_check(">=1.0.0", "1.0.0"); // boundary included for >=
    test_parse_and_check(">1.0.0",  "1.0.1"); // just above
    test_parse_and_check_false(">1.0.0", "1.0.0"); // boundary excluded
    test_parse_and_check_false(">1.0.0", "0.9.9"); // below
  }

  SUBCASE("upper-inclusive (<=)") {
    test_parse_and_check("<=2.0.0", "2.0.0"); // boundary included
    test_parse_and_check("<=2.0.0", "1.9.9"); // below
    test_parse_and_check_false("<=2.0.0", "2.0.1"); // above
  }

  SUBCASE("partial comparators use zero-filled typed boundaries") {
    using V = semver::version<std::uint8_t>;
    semver::range_set<std::uint8_t> rs;

    REQUIRE(semver::parse(">=255", rs));
    REQUIRE(rs.contains(V{255, 0, 0}));
    REQUIRE(rs.contains(V{255, 255, 255}));

    REQUIRE(semver::parse("<255", rs));
    REQUIRE(rs.contains(V{254, 255, 255}));
    REQUIRE_FALSE(rs.contains(V{255, 0, 0}));
  }

  SUBCASE("candidate build metadata is ignored for matching") {
    test_parse_and_check("1.2.3", "1.2.3+candidate.2");
  }

  SUBCASE("three-set union covers distinct sub-ranges") {
    constexpr std::string_view r = ">=1.0.0 <2.0.0 || >=3.0.0 <4.0.0 || >=5.0.0";
    // inside each sub-range
    test_parse_and_check(r, "1.5.0");
    test_parse_and_check(r, "3.5.0");
    test_parse_and_check(r, "5.0.0");
    test_parse_and_check(r, "99.0.0");
    // gaps between sub-ranges
    test_parse_and_check_false(r, "2.0.0");
    test_parse_and_check_false(r, "4.5.0");
    test_parse_and_check_false(r, "0.9.9");
  }
}

TEST_CASE("range prerelease behavior in unions") {
  // The exclude policy should accept prerelease only when at least one
  // comparator in the matching comparator-set carries a prerelease.
  constexpr std::string_view union_range = ">=1.2.3 <2.0.0 || >=1.2.3-alpha.1 <1.2.3";

  SUBCASE("matching prerelease branch includes prerelease") {
    test_parse_and_check(union_range, "1.2.3-alpha.2", semver::prerelease_policy::exclude);
  }

  SUBCASE("plain branch does not include prerelease by default") {
    test_parse_and_check_false(">=1.2.3 <2.0.0", "1.5.0-alpha.1", semver::prerelease_policy::exclude);
  }

  SUBCASE("include policy overrides exclusion") {
    test_parse_and_check(">=1.2.3 <2.0.0", "1.5.0-alpha.1", semver::prerelease_policy::include);
  }
}

TEST_CASE("satisfies") {
  semver::version v;

  SUBCASE("version in range returns true") {
    REQUIRE(semver::parse("1.5.0", v));
    REQUIRE(semver::satisfies(v, ">=1.0.0 <2.0.0"));
  }

  SUBCASE("version outside range returns false") {
    REQUIRE(semver::parse("2.0.0", v));
    REQUIRE_FALSE(semver::satisfies(v, ">=1.0.0 <2.0.0"));
  }

  SUBCASE("prerelease excluded by default") {
    REQUIRE(semver::parse("1.5.0-alpha", v));
    REQUIRE_FALSE(semver::satisfies(v, ">=1.0.0 <2.0.0"));
  }

  SUBCASE("prerelease included with include policy") {
    REQUIRE(semver::parse("1.5.0-alpha", v));
    REQUIRE(semver::satisfies(v, ">=1.0.0-alpha <2.0.0", semver::prerelease_policy::include));
  }

  SUBCASE("invalid range string returns false") {
    REQUIRE(semver::parse("1.0.0", v));
    REQUIRE_FALSE(semver::satisfies(v, "not a range"));
  }

  SUBCASE("OR-separated ranges") {
    REQUIRE(semver::parse("2.5.0", v));
    REQUIRE(semver::satisfies(v, "<1.0.0 || >=2.0.0"));
  }
}

TEST_CASE("prerelease_policy") {
  // Ranges exclude prereleases by default when no comparator targets a
  // prerelease with the same M.m.p tuple.
  semver::range_set<> rs;
  REQUIRE(semver::parse(">=1.0.0 <2.0.0", rs));

  semver::version v_pre, v_rel;
  REQUIRE(semver::parse("1.5.0-alpha", v_pre));
  REQUIRE(semver::parse("1.5.0", v_rel));

  SUBCASE("exclude policy excludes prerelease versions") {
    REQUIRE_FALSE(rs.contains(v_pre, semver::prerelease_policy::exclude));
    REQUIRE(rs.contains(v_rel, semver::prerelease_policy::exclude));
  }

  SUBCASE("include policy allows prerelease versions") {
    REQUIRE(rs.contains(v_pre, semver::prerelease_policy::include));
    REQUIRE(rs.contains(v_rel, semver::prerelease_policy::include));
  }

  // include_build_metadata was removed in the breaking-change refactor;
  // Use the include policy to allow pre-release versions through.
}

TEST_CASE("max_satisfying and min_satisfying") {
  using V = semver::version<std::uint64_t>;
  std::vector<V> vs;
  for (auto s : {"1.0.0", "1.2.0", "1.5.0", "2.0.0", "2.1.0"}) {
    V v;
    REQUIRE(semver::parse(s, v));
    vs.push_back(v);
  }

  semver::range_set<> rs;
  REQUIRE(semver::parse(">=1.0.0 <2.0.0", rs));

  SUBCASE("max_satisfying returns iterator to highest matching version") {
    const auto it = semver::max_satisfying(vs.begin(), vs.end(), rs);
    REQUIRE(it != vs.end());
    V expected;
    REQUIRE(semver::parse("1.5.0", expected));
    REQUIRE(*it == expected);
  }

  SUBCASE("min_satisfying returns iterator to lowest matching version") {
    const auto it = semver::min_satisfying(vs.begin(), vs.end(), rs);
    REQUIRE(it != vs.end());
    V expected;
    REQUIRE(semver::parse("1.0.0", expected));
    REQUIRE(*it == expected);
  }

  SUBCASE("max_satisfying returns last when nothing satisfies") {
    semver::range_set<> empty_match;
    REQUIRE(semver::parse(">9.0.0", empty_match));
    REQUIRE(semver::max_satisfying(vs.begin(), vs.end(), empty_match) == vs.end());
  }

  SUBCASE("min_satisfying returns last when nothing satisfies") {
    semver::range_set<> empty_match;
    REQUIRE(semver::parse(">9.0.0", empty_match));
    REQUIRE(semver::min_satisfying(vs.begin(), vs.end(), empty_match) == vs.end());
  }

  SUBCASE("empty range returns last") {
    REQUIRE(semver::max_satisfying(vs.end(), vs.end(), rs) == vs.end());
    REQUIRE(semver::min_satisfying(vs.end(), vs.end(), rs) == vs.end());
  }

  SUBCASE("the algorithms accept forward iterators") {
    std::forward_list<V> forward_versions{
      semver::from_string<std::uint64_t>("2.0.0"),
      semver::from_string<std::uint64_t>("1.5.0"),
      semver::from_string<std::uint64_t>("1.0.0")
    };

    const auto lowest = semver::min_satisfying(forward_versions.begin(), forward_versions.end(), rs);
    const auto highest = semver::max_satisfying(forward_versions.begin(), forward_versions.end(), rs);

    REQUIRE(lowest != forward_versions.end());
    REQUIRE(highest != forward_versions.end());
    CHECK(lowest->to_string() == "1.0.0");
    CHECK(highest->to_string() == "1.5.0");
  }
}

TEST_CASE("range matching supports mixed component types") {
  SUBCASE("candidate components are never narrowed to range storage") {
    semver::range_set<> rs;
    REQUIRE(semver::parse(">=1", rs));

    const semver::version<std::uint64_t> wide{
      static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) + 1, 0, 0};
    REQUIRE(rs.contains(wide));
  }

  SUBCASE("wide range storage accepts a narrow candidate") {
    semver::range_set<std::uint64_t> rs;
    REQUIRE(semver::parse(">=1 <2", rs));

    const semver::version<std::uint8_t> narrow{1, 2, 3};
    REQUIRE(rs.contains(narrow));
  }

  SUBCASE("mixed prerelease matching keeps the same-core filter") {
    semver::range_set<std::uint8_t> rs;
    REQUIRE(semver::parse(">=1.2.3-alpha <2", rs));

    const semver::version<std::uint64_t> same_core{1, 2, 3, "beta"};
    const semver::version<std::uint64_t> other_core{1, 2, 4, "beta"};
    REQUIRE(rs.contains(same_core));
    REQUIRE_FALSE(rs.contains(other_core));
    REQUIRE(rs.contains(other_core, semver::prerelease_policy::include));
  }

  SUBCASE("heterogeneous component storage preserves each boundary") {
    using mixed_version = semver::version<std::uint8_t, std::uint16_t, std::uint32_t>;
    using mixed_range = semver::range_set<std::uint8_t, std::uint16_t, std::uint32_t>;

    mixed_version boundary;
    REQUIRE(semver::parse("255.65535.4294967295", boundary));
    CHECK(boundary.major() == std::numeric_limits<std::uint8_t>::max());
    CHECK(boundary.minor() == std::numeric_limits<std::uint16_t>::max());
    CHECK(boundary.patch() == std::numeric_limits<std::uint32_t>::max());

    for (const auto overflow : {"256.0.0", "1.65536.0", "1.2.4294967296"}) {
      const auto result = semver::parse(overflow, boundary);
      CHECK_FALSE(result);
      CHECK(result.ec == std::errc::result_out_of_range);
    }

    mixed_range strict;
    REQUIRE(semver::parse(">1.65535.4294967295", strict));
    const auto minimum = semver::min_version(strict);
    REQUIRE(minimum.has_value());
    CHECK(minimum->to_string() == "2.0.0");
  }

  SUBCASE("intersects widens each component independently") {
    semver::range_set<std::uint16_t, std::uint8_t, std::uint8_t> wide_major;
    semver::range_set<std::uint8_t, std::uint16_t, std::uint8_t> wide_minor;
    REQUIRE(semver::parse(">=256.1.0 <300", wide_major));
    REQUIRE(semver::parse(">=200.300.0", wide_minor));
    CHECK(semver::intersects(wide_major, wide_minor));
    CHECK(semver::intersects(wide_minor, wide_major));
  }
}

TEST_CASE("min_version returns the lowest representable match") {
  SUBCASE("any range follows prerelease policy") {
    semver::range_set<> rs;
    REQUIRE(semver::parse("*", rs));
    REQUIRE(semver::min_version(rs)->to_string() == "0.0.0");
    REQUIRE(semver::min_version(rs, semver::prerelease_policy::include)->to_string() == "0.0.0-0");
  }

  SUBCASE("strict and excluded lower bounds advance to the next candidate") {
    semver::range_set<> rs;
    REQUIRE(semver::parse(">=1 <2 !=1.0.0 !=1.0.1", rs));
    REQUIRE(semver::min_version(rs)->to_string() == "1.0.2");
    REQUIRE(semver::min_version(rs, semver::prerelease_policy::include)->to_string() == "1.0.0-0");

    REQUIRE(semver::parse("!=0.0.0", rs));
    REQUIRE(semver::min_version(rs)->to_string() == "0.0.1");
    REQUIRE(semver::min_version(rs, semver::prerelease_policy::include)->to_string() == "0.0.0-0");

    REQUIRE(semver::parse(">1.2.3 <2", rs));
    REQUIRE(semver::min_version(rs)->to_string() == "1.2.4");
    REQUIRE(semver::min_version(rs, semver::prerelease_policy::include)->to_string() == "1.2.4-0");
  }

  SUBCASE("strict lower bounds carry across component limits") {
    semver::range_set<std::uint8_t> rs;
    REQUIRE(semver::parse(">1.2.255", rs));
    REQUIRE(semver::min_version(rs)->to_string() == "1.3.0");

    REQUIRE(semver::parse(">1.255.255", rs));
    REQUIRE(semver::min_version(rs)->to_string() == "2.0.0");
  }

  SUBCASE("explicit prerelease boundary has an exact successor") {
    semver::range_set<> rs;
    REQUIRE(semver::parse(">1.2.3-alpha <1.2.3", rs));
    REQUIRE(semver::min_version(rs)->to_string() == "1.2.3-alpha.0");

    REQUIRE(semver::parse(">0.0.0-0 <0.0.0-0.0", rs));
    REQUIRE_FALSE(semver::min_version(rs).has_value());
  }

  SUBCASE("OR branches choose the global minimum") {
    semver::range_set<> rs;
    REQUIRE(semver::parse(">=3 || >=1.2 <2", rs));
    REQUIRE(semver::min_version(rs)->to_string() == "1.2.0");
  }

  SUBCASE("empty and unrepresentable sets return nullopt") {
    semver::range_set<> empty;
    REQUIRE_FALSE(semver::min_version(empty).has_value());

    semver::range_set<std::uint8_t> above_max;
    REQUIRE(semver::parse(">255.255.255", above_max));
    REQUIRE_FALSE(semver::min_version(above_max).has_value());
  }
}

TEST_CASE("intersects checks whether two range sets share a version") {
  SUBCASE("overlapping, touching, and disjoint bounds") {
    semver::range_set<> lhs, rhs;
    REQUIRE(semver::parse(">=1 <2", lhs));
    REQUIRE(semver::parse(">=1.5 <3", rhs));
    REQUIRE(semver::intersects(lhs, rhs));

    REQUIRE(semver::parse("<=1.0.0", lhs));
    REQUIRE(semver::parse(">=1.0.0", rhs));
    REQUIRE(semver::intersects(lhs, rhs));

    REQUIRE(semver::parse("<1.0.0", lhs));
    REQUIRE_FALSE(semver::intersects(lhs, rhs));
  }

  SUBCASE("OR branches and exclusions are respected") {
    semver::range_set<> lhs, rhs;
    REQUIRE(semver::parse("<1 || >=3 <4", lhs));
    REQUIRE(semver::parse(">=2 <3.5", rhs));
    REQUIRE(semver::intersects(lhs, rhs));

    REQUIRE(semver::parse("1.2.3", lhs));
    REQUIRE(semver::parse("!=1.2.3", rhs));
    REQUIRE_FALSE(semver::intersects(lhs, rhs));
  }

  SUBCASE("prerelease filters apply independently") {
    semver::range_set<> lhs, rhs;
    REQUIRE(semver::parse(">=1.0.0-alpha <1.0.0", lhs));
    REQUIRE(semver::parse("<1.0.0", rhs));
    REQUIRE_FALSE(semver::intersects(lhs, rhs));
    REQUIRE(semver::intersects(lhs, rhs, semver::prerelease_policy::include));

    REQUIRE(semver::parse(">1.0.0-beta <1.0.0", rhs));
    REQUIRE(semver::intersects(lhs, rhs));
  }

  SUBCASE("a generated prerelease floor can be the only overlap") {
    semver::range_set<> lhs, rhs;
    REQUIRE(semver::parse(">=1", lhs));
    REQUIRE(semver::parse("<1.0.0", rhs));
    REQUIRE_FALSE(semver::intersects(lhs, rhs));
    REQUIRE(semver::intersects(lhs, rhs, semver::prerelease_policy::include));
  }

  SUBCASE("strict lower bound witnesses carry across component limits") {
    semver::range_set<std::uint8_t> lhs, rhs;
    REQUIRE(semver::parse(">1.2.255 <2", lhs));
    REQUIRE(semver::parse(">=1.3.0 <2", rhs));
    REQUIRE(semver::intersects(lhs, rhs));
  }

  SUBCASE("adjacent versions leave no hidden value") {
    semver::range_set<> lhs, rhs;
    REQUIRE(semver::parse(">1.0.0-alpha", lhs));
    REQUIRE(semver::parse("<1.0.0-alpha.0", rhs));
    REQUIRE_FALSE(semver::intersects(lhs, rhs));

    REQUIRE(semver::parse(">1.0.0", lhs));
    REQUIRE(semver::parse("<1.0.1-0", rhs));
    REQUIRE_FALSE(semver::intersects(lhs, rhs, semver::prerelease_policy::include));
  }

  SUBCASE("an any range does not opt into prereleases by default") {
    semver::range_set<> prereleases, any;
    REQUIRE(semver::parse("<0.0.0", prereleases));
    REQUIRE(semver::parse("*", any));
    REQUIRE_FALSE(semver::intersects(prereleases, any));
    REQUIRE(semver::intersects(prereleases, any, semver::prerelease_policy::include));
  }

  SUBCASE("mixed storage types and unbounded tails") {
    semver::range_set<std::uint8_t> narrow;
    semver::range_set<std::uint16_t> wide;
    REQUIRE(semver::parse(">255.255.255", narrow));
    REQUIRE(semver::parse(">=256", wide));
    REQUIRE(semver::intersects(narrow, wide));
  }

  SUBCASE("unbounded narrow ranges intersect beyond their storage types") {
    semver::range_set<std::uint8_t> lhs, rhs;
    REQUIRE(semver::parse(">255.255.255", lhs));
    REQUIRE(semver::parse(">255.255.255", rhs));
    REQUIRE_FALSE(semver::min_version(lhs).has_value());
    REQUIRE_FALSE(semver::min_version(rhs).has_value());
    CHECK(semver::intersects(lhs, rhs));
  }

  SUBCASE("default-constructed range intersects nothing") {
    semver::range_set<> empty, any;
    REQUIRE(semver::parse("*", any));
    REQUIRE_FALSE(semver::intersects(empty, any));
  }

  SUBCASE("representative ranges are symmetric") {
    constexpr std::array<std::string_view, 12> cases = {{
      "*", "<1", ">=1 <2", "1.2", "~1.2", "^0.2.3",
      ">=1.0.0-alpha <1.0.0", "!=1.2.3", "<1 || >=3", "1.2.3",
      ">=1 <2 !=1.5.0", "^0.0.3 || 2.x"
    }};
    constexpr std::array policies = {
      semver::prerelease_policy::exclude,
      semver::prerelease_policy::include
    };

    std::array<semver::range_set<>, cases.size()> ranges;
    for (std::size_t i = 0; i < cases.size(); ++i)
      REQUIRE(semver::parse(cases[i], ranges[i]));

    for (std::size_t i = 0; i < ranges.size(); ++i) {
      for (std::size_t j = 0; j < ranges.size(); ++j) {
        for (const auto policy : policies) {
          const auto include_prereleases = policy == semver::prerelease_policy::include;
          CAPTURE(cases[i]);
          CAPTURE(cases[j]);
          CAPTURE(include_prereleases);
          CHECK(semver::intersects(ranges[i], ranges[j], policy) == semver::intersects(ranges[j], ranges[i], policy));
        }
      }
    }
  }
}

TEST_CASE("range utility candidates agree with bounded exhaustive search") {
  const std::vector<std::string_view> range_texts{
    "*", "0", "1", "2", "3", "0.0", "0.1", "1.0", "1.2", "2.0",
    "0.0.0", "0.0.0-0", "0.0.0-alpha", "1.2.3", "1.2.3-alpha",
    ">0.0.0", ">0.0.0-0", ">0.0.0-alpha", ">1.2.3", ">1.2.3-alpha",
    ">=0.0.0", ">=0.0.0-alpha", ">=1.2.3", ">=1.2.3-alpha",
    "<0.0.0", "<0.0.0-alpha", "<1", "<1.2", "<1.2.3", "<1.2.3-alpha",
    "<=0.0.0", "<=0.0.0-alpha", "<=1.2.3", "<=1.2.3-alpha",
    "!=0.0.0", "!=0.0.0-0", "!=0.0.0-alpha", "!=1.2.3",
    ">=0 <1", ">=1 <2", ">=1.2 <1.3", ">=1.2.3 <2",
    ">0.0.0 <0.0.1", ">0.0.0-0 <0.0.0", ">0.0.0-alpha <0.0.0-beta",
    ">=0.0.0 !=0.0.0 <0.0.2", ">=1 <3 !=2.0.0",
    "~0", "~1", "~1.2", "~1.2.3", "^0", "^0.0.3", "^0.2.3", "^1.2.3",
    "0 || 2", ">=0 <1 || >=2 <3", ">4.4.4"
  };
  constexpr std::array<std::string_view, 15> prereleases{{
    "", "0", "0.0", "0.0.0", "1", "1.0",
    "alpha", "alpha.0", "alpha.0.0", "alpha.1", "alpha.1.0",
    "beta", "beta.0", "z", "z.0"
  }};

  std::vector<semver::range_set<std::uint8_t>> ranges;
  ranges.reserve(range_texts.size());
  for (const auto text : range_texts) {
    const auto range = semver::try_parse_range<std::uint8_t>(text);
    CAPTURE(text);
    REQUIRE(range.has_value());
    ranges.push_back(*range);
  }

  std::vector<semver::version<std::uint8_t>> versions;
  for (std::uint8_t major = 0; major <= 6; ++major) {
    for (std::uint8_t minor = 0; minor <= 6; ++minor) {
      for (std::uint8_t patch = 0; patch <= 6; ++patch) {
        for (const auto prerelease : prereleases)
          versions.emplace_back(major, minor, patch, prerelease);
      }
    }
  }
  std::sort(versions.begin(), versions.end());
  versions.erase(std::unique(versions.begin(), versions.end()), versions.end());

  constexpr std::array policies{
    semver::prerelease_policy::exclude,
    semver::prerelease_policy::include
  };
  for (const auto policy : policies) {
    const auto include_prereleases = policy == semver::prerelease_policy::include;
    for (std::size_t i = 0; i < ranges.size(); ++i) {
      const auto brute_min = std::find_if(versions.begin(), versions.end(), [&](const auto& version) { return ranges[i].contains(version, policy); });
      const auto actual_min = semver::min_version(ranges[i], policy);

      CAPTURE(range_texts[i]);
      CAPTURE(include_prereleases);
      CHECK(actual_min.has_value() == (brute_min != versions.end()));
      if (actual_min && brute_min != versions.end())
        CHECK(*actual_min == *brute_min);

      for (std::size_t j = 0; j < ranges.size(); ++j) {
        const auto brute_intersects = std::any_of(versions.begin(), versions.end(), [&](const auto& version) {
          return ranges[i].contains(version, policy) && ranges[j].contains(version, policy);
        });

        CAPTURE(range_texts[j]);
        CHECK(semver::intersects(ranges[i], ranges[j], policy) == brute_intersects);
      }
    }
  }
}

static_assert(noexcept(std::declval<const semver::range_set<>&>().contains(
  std::declval<const semver::version<std::uint64_t>&>())));
