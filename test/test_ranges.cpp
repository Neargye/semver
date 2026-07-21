#include <semver.hpp>
#include <doctest.h>
#include <array>
#include <ostream>

static void test_parse_and_check(std::string_view range_str, std::string_view ver_str,
  semver::version_compare_option option = semver::version_compare_option::exclude_prerelease)
{
  semver::version v;
  REQUIRE(semver::parse(ver_str, v));

  semver::range_set rs;
  REQUIRE(semver::parse(range_str, rs));

  REQUIRE(rs.contains(v, option));
}

static void test_parse_and_check_false(std::string_view range_str, std::string_view ver_str,
  semver::version_compare_option option = semver::version_compare_option::exclude_prerelease)
{
  semver::version v;
  REQUIRE(semver::parse(ver_str, v));

  semver::range_set rs;
  REQUIRE(semver::parse(range_str, rs));

  REQUIRE_FALSE(rs.contains(v, option));
}

TEST_CASE("ranges") {
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
      test_parse_and_check(r1, v1, semver::version_compare_option::include_prerelease);
      test_parse_and_check(r1, v2, semver::version_compare_option::include_prerelease);
      test_parse_and_check(r1, v3, semver::version_compare_option::include_prerelease);
      test_parse_and_check(r1, v4, semver::version_compare_option::include_prerelease);
      test_parse_and_check_false(r2, v1, semver::version_compare_option::include_prerelease);
      test_parse_and_check(r3, v1, semver::version_compare_option::include_prerelease);
      test_parse_and_check(r4, v5, semver::version_compare_option::include_prerelease);
      test_parse_and_check_false(r4, v1, semver::version_compare_option::include_prerelease);
      test_parse_and_check(r5, v5, semver::version_compare_option::include_prerelease);
      test_parse_and_check_false(r6, v5, semver::version_compare_option::include_prerelease);
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
      test_parse_and_check(r1, v1, semver::version_compare_option::include_prerelease);
      test_parse_and_check_false(r1, v2, semver::version_compare_option::include_prerelease);
      test_parse_and_check_false(r1, v3, semver::version_compare_option::include_prerelease);

      test_parse_and_check(r2, v1, semver::version_compare_option::include_prerelease);
      test_parse_and_check(r2, v2, semver::version_compare_option::include_prerelease);
      test_parse_and_check_false(r2, v3, semver::version_compare_option::include_prerelease);

      test_parse_and_check(r3, v1, semver::version_compare_option::include_prerelease);
      test_parse_and_check(r3, v2, semver::version_compare_option::include_prerelease);
      test_parse_and_check(r3, v3, semver::version_compare_option::include_prerelease);
    }
  }
}

TEST_CASE("range parse failures") {
  semver::range_set rs;

  SUBCASE("empty string") {
    REQUIRE_FALSE(semver::parse("", rs));
  }

  SUBCASE("operator without version") {
    REQUIRE_FALSE(semver::parse(">",  rs));
    REQUIRE_FALSE(semver::parse(">=", rs));
    REQUIRE_FALSE(semver::parse("<",  rs));
  }

  SUBCASE("dangling union separator") {
    REQUIRE_FALSE(semver::parse(">=1.0.0 ||", rs));
  }

  SUBCASE("isolated or misplaced union separators") {
    REQUIRE_FALSE(semver::parse("||", rs));
    REQUIRE_FALSE(semver::parse("|| >=1.0.0", rs));
    REQUIRE_FALSE(semver::parse(">=1.0.0 || || <2.0.0", rs));
  }

  SUBCASE("tabs are invalid separators") {
    REQUIRE_FALSE(semver::parse(">=1.0.0\t<2.0.0", rs));
  }

  SUBCASE("leading and trailing spaces are rejected") {
    REQUIRE_FALSE(semver::parse(" >=1.0.0", rs));
    REQUIRE_FALSE(semver::parse(">=1.0.0 ", rs));
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
    REQUIRE_FALSE(semver::parse("~ ", rs)); // leading space rejected before parser
    REQUIRE_FALSE(semver::parse("^ ", rs));
  }

  SUBCASE("leading zeros rejected in advanced range versions") {
    REQUIRE_FALSE(semver::parse("~01.2.3", rs));
    REQUIRE_FALSE(semver::parse("~1.02.3", rs));
    REQUIRE_FALSE(semver::parse("^01.0.0", rs));
  }

  SUBCASE("invalid prerelease qualifiers are rejected in all range forms") {
    REQUIRE_FALSE(semver::parse("^1.2.3-01", rs));
    REQUIRE_FALSE(semver::parse("~1.2.3-alpha.", rs));
    REQUIRE_FALSE(semver::parse("1.2.3-01", rs));
    REQUIRE_FALSE(semver::parse("1.2.3 - 2.0.0-01", rs));
  }

  SUBCASE("invalid build metadata is rejected even though ranges ignore it") {
    REQUIRE_FALSE(semver::parse("^1.2.3+", rs));
    REQUIRE_FALSE(semver::parse("^1.2.3+build.", rs));
    REQUIRE_FALSE(semver::parse("1.2.3+", rs));
    REQUIRE_FALSE(semver::parse("1.2.3 - 2.0.0+build.", rs));
  }

  SUBCASE("explicit comparators require a complete version") {
    REQUIRE_FALSE(semver::parse(">=1", rs));
    REQUIRE_FALSE(semver::parse(">=1.2", rs));
    REQUIRE_FALSE(semver::parse(">=1.x", rs));
  }
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

    const auto result = semver::parse(">=1.0.0\t<2.0.0", rs);
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
  // exclude_prerelease should accept prerelease only when at least one
  // comparator in the matching comparator-set carries a prerelease.
  constexpr std::string_view union_range = ">=1.2.3 <2.0.0 || >=1.2.3-alpha.1 <1.2.3";

  SUBCASE("matching prerelease branch includes prerelease") {
    test_parse_and_check(union_range, "1.2.3-alpha.2", semver::version_compare_option::exclude_prerelease);
  }

  SUBCASE("plain branch does not include prerelease by default") {
    test_parse_and_check_false(">=1.2.3 <2.0.0", "1.5.0-alpha.1", semver::version_compare_option::exclude_prerelease);
  }

  SUBCASE("include_prerelease overrides exclusion") {
    test_parse_and_check(">=1.2.3 <2.0.0", "1.5.0-alpha.1", semver::version_compare_option::include_prerelease);
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

  SUBCASE("prerelease included with include_prerelease option") {
    REQUIRE(semver::parse("1.5.0-alpha", v));
    REQUIRE(semver::satisfies(v, ">=1.0.0-alpha <2.0.0", semver::include_prerelease));
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

TEST_CASE("version_compare_option aliases") {
  // Range has no prerelease tag in comparators, so pre-release versions are
  // excluded by default (semver spec §11).
  semver::range_set<> rs;
  REQUIRE(semver::parse(">=1.0.0 <2.0.0", rs));

  semver::version v_pre, v_rel;
  REQUIRE(semver::parse("1.5.0-alpha", v_pre));
  REQUIRE(semver::parse("1.5.0", v_rel));

  SUBCASE("exclude_prerelease alias excludes prerelease versions") {
    REQUIRE_FALSE(rs.contains(v_pre, semver::exclude_prerelease));
    REQUIRE(rs.contains(v_rel, semver::exclude_prerelease));
  }

  SUBCASE("include_prerelease alias allows prerelease versions") {
    REQUIRE(rs.contains(v_pre, semver::include_prerelease));
    REQUIRE(rs.contains(v_rel, semver::include_prerelease));
  }

  // include_build_metadata was removed in the breaking-change refactor;
  // use include_prerelease to allow pre-release versions through.
}

TEST_CASE("max_satisfying and min_satisfying") {
  using V = semver::version<>;
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
}

