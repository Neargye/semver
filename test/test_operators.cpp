#include "test_utils.hpp"
#include <semver.hpp>
#include <array>

template <typename Operator>
static void test_parse_and_compare_reverse(const std::string_view v1, const std::string_view v2, Operator op) {
  semver::version parsed_v1;
  REQUIRE(semver::parse(v1, parsed_v1));

  semver::version parsed_v2;
  REQUIRE(semver::parse(v2, parsed_v2));

  REQUIRE(op(parsed_v1, parsed_v2));
  REQUIRE(op(parsed_v2, parsed_v1));
}

template <typename Operator>
static void test_parse_and_compare_reverse_false(const std::string_view v1, const std::string_view v2, Operator op) {
  INFO(v1);
  INFO(v2);

  semver::version parsed_v1;
  REQUIRE(semver::parse(v1, parsed_v1));

  semver::version parsed_v2;
  REQUIRE(semver::parse(v2, parsed_v2));

  REQUIRE(op(parsed_v1, parsed_v2));
  REQUIRE_FALSE(op(parsed_v2, parsed_v1));
}

TEST_CASE("operators") {
  constexpr std::array<std::string_view, 56> versions = { {
    std::string_view{"0.0.0-alpha.0"},
    std::string_view{"0.0.0-alpha.1"},
    std::string_view{"0.0.0-beta.0"},
    std::string_view{"0.0.0-beta.1"},
    std::string_view{"0.0.0-rc.0"},
    std::string_view{"0.0.0-rc.1"},
    std::string_view{"0.0.0"},

    std::string_view{"0.0.1-alpha.0"},
    std::string_view{"0.0.1-alpha.1"},
    std::string_view{"0.0.1-beta.0"},
    std::string_view{"0.0.1-beta.1"},
    std::string_view{"0.0.1-rc.0"},
    std::string_view{"0.0.1-rc.1"},
    std::string_view{"0.0.1"},

    std::string_view{"0.1.0-alpha.0"},
    std::string_view{"0.1.0-alpha.1"},
    std::string_view{"0.1.0-beta.0"},
    std::string_view{"0.1.0-beta.1"},
    std::string_view{"0.1.0-rc.0"},
    std::string_view{"0.1.0-rc.1"},
    std::string_view{"0.1.0"},

    std::string_view{"0.1.1-alpha.0"},
    std::string_view{"0.1.1-alpha.1"},
    std::string_view{"0.1.1-beta.0"},
    std::string_view{"0.1.1-beta.1"},
    std::string_view{"0.1.1-rc.0"},
    std::string_view{"0.1.1-rc.1"},
    std::string_view{"0.1.1"},

    std::string_view{"1.0.0-alpha.0"},
    std::string_view{"1.0.0-alpha.1"},
    std::string_view{"1.0.0-beta.0"},
    std::string_view{"1.0.0-beta.1"},
    std::string_view{"1.0.0-rc.0"},
    std::string_view{"1.0.0-rc.1"},
    std::string_view{"1.0.0"},

    std::string_view{"1.0.1-alpha.0"},
    std::string_view{"1.0.1-alpha.1"},
    std::string_view{"1.0.1-beta.0"},
    std::string_view{"1.0.1-beta.1"},
    std::string_view{"1.0.1-rc.0"},
    std::string_view{"1.0.1-rc.1"},
    std::string_view{"1.0.1"},

    std::string_view{"1.1.0-alpha.0"},
    std::string_view{"1.1.0-alpha.1"},
    std::string_view{"1.1.0-beta.0"},
    std::string_view{"1.1.0-beta.1"},
    std::string_view{"1.1.0-rc.0"},
    std::string_view{"1.1.0-rc.1"},
    std::string_view{"1.1.0"},

    std::string_view{"1.1.1-alpha.0"},
    std::string_view{"1.1.1-alpha.1"},
    std::string_view{"1.1.1-beta.0"},
    std::string_view{"1.1.1-beta.1"},
    std::string_view{"1.1.1-rc.0"},
    std::string_view{"1.1.1-rc.1"},
    std::string_view{"1.1.1"},
  } };

  SECTION("operator ==") {
    for (auto version : versions) {
      test_parse_and_compare_reverse(version, version, semver::operator==<int, int, int>);
    }
  }

  SECTION("operator !=") {
    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        test_parse_and_compare_reverse(versions[i], versions[i - j], semver::operator!=<int, int, int>);
      }
    }
  }

  SECTION("operator >") {
    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        test_parse_and_compare_reverse_false(versions[i], versions[i - j], semver::operator><int, int, int>);
      }
    }
  }

  SECTION("operator >=") {
    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        test_parse_and_compare_reverse_false(versions[i], versions[i - j], semver::operator>=<int, int, int>);
        test_parse_and_compare_reverse(versions[i], versions[i], semver::operator>=<int, int, int>);
      }
    }
  }

  SECTION("operator <") {
    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        test_parse_and_compare_reverse_false(versions[i - j], versions[i], semver::operator< <int, int, int>);
      }
    }
  }

  SECTION("operator <=") {
    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        test_parse_and_compare_reverse_false(versions[i - j], versions[i], semver::operator<=<int, int, int>);
        test_parse_and_compare_reverse(versions[i - j], versions[i - j], semver::operator<=<int, int, int>);
      }
    }
  }

  SECTION("prerelease compare") {
    // 1.0.0-alpha < 1.0.0-alpha.1 < 1.0.0-alpha.beta < 1.0.0-beta < 1.0.0-beta.2 < 1.0.0-beta.11 < 1.0.0-rc.1 < 1.0.0.
    constexpr std::string_view v1 = "1.0.0-alpha";
    constexpr std::string_view v2 = "1.0.0-alpha.1";
    constexpr std::string_view v3 = "1.0.0-alpha.beta";
    constexpr std::string_view v4 = "1.0.0-beta";
    constexpr std::string_view v5 = "1.0.0-beta.2";
    constexpr std::string_view v6 = "1.0.0-beta.11";
    constexpr std::string_view v7 = "1.0.0-rc.1";
    constexpr std::string_view v8 = "1.0.0";

    test_parse_and_compare_reverse_false(v1, v2, semver::operator< <int, int, int>);
    test_parse_and_compare_reverse_false(v2, v3, semver::operator< <int, int, int>);
    test_parse_and_compare_reverse_false(v3, v4, semver::operator< <int, int, int>);
    test_parse_and_compare_reverse_false(v4, v5, semver::operator< <int, int, int>);
    test_parse_and_compare_reverse_false(v5, v6, semver::operator< <int, int, int>);
    test_parse_and_compare_reverse_false(v6, v7, semver::operator< <int, int, int>);
    test_parse_and_compare_reverse_false(v7, v8, semver::operator< <int, int, int>);

    test_parse_and_compare_reverse_false(v2, v1, semver::operator><int, int, int>);
    test_parse_and_compare_reverse_false(v3, v2, semver::operator><int, int, int>);
    test_parse_and_compare_reverse_false(v4, v3, semver::operator><int, int, int>);
    test_parse_and_compare_reverse_false(v5, v4, semver::operator><int, int, int>);
    test_parse_and_compare_reverse_false(v6, v5, semver::operator><int, int, int>);
    test_parse_and_compare_reverse_false(v7, v6, semver::operator><int, int, int>);
    test_parse_and_compare_reverse_false(v8, v7, semver::operator><int, int, int>);

    constexpr std::string_view v9 = "1.0.0-alpha.5";
    constexpr std::string_view v10 = "1.0.0-alpha.10";
    test_parse_and_compare_reverse_false(v9, v10, semver::operator< <int, int, int>);
    test_parse_and_compare_reverse_false(v10, v9, semver::operator><int, int, int>);
  }
}
