#include <semver.hpp>
#include <catch.hpp>
#include <array>

using namespace semver;

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

TEST_CASE("ranges with prerelease tags") {
  SECTION("prerelease tags") {
    constexpr std::string_view r1{">1.2.3-alpha.3"};
    constexpr std::string_view r2{">=1.2.3 < 2.0.0"};
    constexpr std::string_view r3{">=1.2.3-alpha.7 <2.0.0"};
    constexpr std::string_view r4{">1.2.3 <2.0.0-alpha.10"};
    constexpr std::string_view r5{">1.2.3 <2.0.0-alpha.1 || <=2.0.0-alpha.5"};
    constexpr std::string_view r6{"<=2.0.0-alpha.4"};

    constexpr version v1{"1.2.3-alpha.7"};
    constexpr version v2{"3.4.5-alpha.9"};
    constexpr version v3{"3.4.5"};
    constexpr version v4{"1.2.3-alpha.4"};
    constexpr version v5{"2.0.0-alpha.5"};

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
    }
  }

  SECTION("prerelease type comparison") {
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