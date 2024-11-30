#include <semver.hpp>
#include <catch.hpp>
#include <array>

static void test_parse_and_check(std::string_view range_str, std::string_view ver_str,
  semver::version_compare_option option = semver::version_compare_option::exclude_prerelease)
{
  INFO(range_str << " : " << ver_str);

  semver::version v;
  REQUIRE(semver::parse(ver_str, v));

  semver::range_set rs;
  REQUIRE(semver::parse(range_str, rs));

  REQUIRE(rs.contains(v, option));
}

static void test_parse_and_check_false(std::string_view range_str, std::string_view ver_str,
  semver::version_compare_option option = semver::version_compare_option::exclude_prerelease)
{
  INFO(range_str << " : " << ver_str);

  semver::version v;
  REQUIRE(semver::parse(ver_str, v));

  semver::range_set rs;
  REQUIRE(semver::parse(range_str, rs));

  REQUIRE_FALSE(rs.contains(v, option));
}

TEST_CASE("ranges") {
  SECTION("constructor") {
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

  SECTION("one comparator set") {
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

  SECTION("multiple comparators set") {
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
  SECTION("prerelease tags") {
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

    SECTION("exclude prerelease") {
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

    SECTION("include prerelease") {
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

  SECTION("prerelease type comparison") {
    constexpr std::string_view v1{"1.0.0-alpha.123"};
    constexpr std::string_view v2{"1.0.0-beta.123"};
    constexpr std::string_view v3{"1.0.0-rc.123"};

    constexpr std::string_view r1{"<=1.0.0-alpha.123"};
    constexpr std::string_view r2{"<=1.0.0-beta.123"};
    constexpr std::string_view r3{"<=1.0.0-rc.123"};

    SECTION("exclude prerelease") {
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

    SECTION("include prerelease") {
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
