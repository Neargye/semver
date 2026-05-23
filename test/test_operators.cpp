#include <semver.hpp>
#include <doctest.h>
#include <algorithm>
#include <array>
#include <ostream>
#include <set>
#include <unordered_map>
#include <vector>

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

  SUBCASE("operator ==") {
    for (auto version : versions) {
      test_parse_and_compare_reverse(version, version,
          [](const semver::version<>& a, const semver::version<>& b) { return a == b; });
    }
  }

  SUBCASE("operator !=") {
    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        test_parse_and_compare_reverse(versions[i], versions[i - j],
            [](const semver::version<>& a, const semver::version<>& b) { return a != b; });
      }
    }
  }

  SUBCASE("operator >") {
    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        test_parse_and_compare_reverse_false(versions[i], versions[i - j],
            [](const semver::version<>& a, const semver::version<>& b) { return a > b; });
      }
    }
  }

  SUBCASE("operator >=") {
    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        test_parse_and_compare_reverse_false(versions[i], versions[i - j],
            [](const semver::version<>& a, const semver::version<>& b) { return a >= b; });
        test_parse_and_compare_reverse(versions[i], versions[i],
            [](const semver::version<>& a, const semver::version<>& b) { return a >= b; });
      }
    }
  }

  SUBCASE("operator <") {
    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        test_parse_and_compare_reverse_false(versions[i - j], versions[i],
            [](const semver::version<>& a, const semver::version<>& b) { return a < b; });
      }
    }
  }

  SUBCASE("operator <=") {
    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        test_parse_and_compare_reverse_false(versions[i - j], versions[i],
            [](const semver::version<>& a, const semver::version<>& b) { return a <= b; });
        test_parse_and_compare_reverse(versions[i - j], versions[i - j],
            [](const semver::version<>& a, const semver::version<>& b) { return a <= b; });
      }
    }
  }

  SUBCASE("prerelease compare") {
    // 1.0.0-alpha < 1.0.0-alpha.1 < 1.0.0-alpha.beta < 1.0.0-beta < 1.0.0-beta.2 < 1.0.0-beta.11 < 1.0.0-rc.1 < 1.0.0.
    constexpr std::string_view v1 = "1.0.0-alpha";
    constexpr std::string_view v2 = "1.0.0-alpha.1";
    constexpr std::string_view v3 = "1.0.0-alpha.beta";
    constexpr std::string_view v4 = "1.0.0-beta";
    constexpr std::string_view v5 = "1.0.0-beta.2";
    constexpr std::string_view v6 = "1.0.0-beta.11";
    constexpr std::string_view v7 = "1.0.0-rc.1";
    constexpr std::string_view v8 = "1.0.0";

    auto lt = [](const semver::version<>& a, const semver::version<>& b) { return a < b; };
    auto gt = [](const semver::version<>& a, const semver::version<>& b) { return a > b; };

    test_parse_and_compare_reverse_false(v1, v2, lt);
    test_parse_and_compare_reverse_false(v2, v3, lt);
    test_parse_and_compare_reverse_false(v3, v4, lt);
    test_parse_and_compare_reverse_false(v4, v5, lt);
    test_parse_and_compare_reverse_false(v5, v6, lt);
    test_parse_and_compare_reverse_false(v6, v7, lt);
    test_parse_and_compare_reverse_false(v7, v8, lt);

    test_parse_and_compare_reverse_false(v2, v1, gt);
    test_parse_and_compare_reverse_false(v3, v2, gt);
    test_parse_and_compare_reverse_false(v4, v3, gt);
    test_parse_and_compare_reverse_false(v5, v4, gt);
    test_parse_and_compare_reverse_false(v6, v5, gt);
    test_parse_and_compare_reverse_false(v7, v6, gt);
    test_parse_and_compare_reverse_false(v8, v7, gt);

    constexpr std::string_view v9 = "1.0.0-alpha.5";
    constexpr std::string_view v10 = "1.0.0-alpha.10";
    test_parse_and_compare_reverse_false(v9, v10, lt);
    test_parse_and_compare_reverse_false(v10, v9, gt);
  }
}

TEST_CASE("build metadata equality") {
  // semver spec §10: build metadata MUST be ignored when determining precedence
  SUBCASE("versions differing only in build metadata are equal") {
    semver::version v1, v2, v3;
    REQUIRE(semver::parse("1.0.0+build.1", v1));
    REQUIRE(semver::parse("1.0.0+build.2", v2));
    REQUIRE(semver::parse("1.0.0", v3));
    REQUIRE(v1 == v2);
    REQUIRE(v1 == v3);
    REQUIRE_FALSE(v1 != v2);
  }
}

TEST_CASE("hash") {
  SUBCASE("same version, same hash") {
    semver::version v1, v2;
    REQUIRE(semver::parse("1.2.3", v1));
    REQUIRE(semver::parse("1.2.3", v2));
    REQUIRE(std::hash<semver::version<>>{}(v1) == std::hash<semver::version<>>{}(v2));
  }

  SUBCASE("build metadata ignored in hash") {
    semver::version v1, v2;
    REQUIRE(semver::parse("1.0.0+build.1", v1));
    REQUIRE(semver::parse("1.0.0+build.999", v2));
    REQUIRE(std::hash<semver::version<>>{}(v1) == std::hash<semver::version<>>{}(v2));
  }

  SUBCASE("different versions produce different hashes") {
    semver::version v1, v2, v3;
    REQUIRE(semver::parse("1.2.3", v1));
    REQUIRE(semver::parse("1.2.4", v2));
    REQUIRE(semver::parse("1.2.3-alpha", v3));
    REQUIRE(std::hash<semver::version<>>{}(v1) != std::hash<semver::version<>>{}(v2));
    REQUIRE(std::hash<semver::version<>>{}(v1) != std::hash<semver::version<>>{}(v3));
  }

  SUBCASE("usable in unordered_map") {
    std::unordered_map<semver::version<>, std::string> map;
    semver::version v;
    REQUIRE(semver::parse("1.0.0", v));
    map[v] = "stable";
    REQUIRE(map.at(v) == "stable");
  }
}

TEST_CASE("mixed-type comparisons") {
  // version<int,int,int> and version<unsigned,unsigned,unsigned> with the same
  // numeric values must be equal, and ordering must be consistent.
  semver::version<int, int, int>           a;
  semver::version<unsigned, unsigned, unsigned> b;
  semver::version<int64_t, int64_t, int64_t>    c;

  REQUIRE(semver::parse("1.2.3", a));
  REQUIRE(semver::parse("1.2.3", b));
  REQUIRE(semver::parse("1.2.3", c));

  SUBCASE("equality across types") {
    REQUIRE(a == b);
    REQUIRE(a == c);
    REQUIRE(b == c);
  }

  SUBCASE("ordering across types") {
    semver::version<int>      lo;
    semver::version<unsigned> hi;
    REQUIRE(semver::parse("1.2.2", lo));
    REQUIRE(semver::parse("1.2.3", hi));
    REQUIRE(lo < hi);
    REQUIRE(hi > lo);
    REQUIRE(lo <= hi);
    REQUIRE(hi >= lo);
  }

  SUBCASE("prerelease ordering across types") {
    semver::version<int>      pre;
    semver::version<unsigned> rel;
    REQUIRE(semver::parse("1.0.0-alpha", pre));
    REQUIRE(semver::parse("1.0.0",       rel));
    REQUIRE(pre < rel);
  }
}

TEST_CASE("build metadata does not affect ordering operators") {
  // spec \u00a710: build metadata MUST be ignored for all version precedence comparisons
  semver::version a, b;
  REQUIRE(semver::parse("1.0.0+aaa", a));
  REQUIRE(semver::parse("1.0.0+zzz", b));
  CHECK(a == b);
  CHECK_FALSE(a != b);
  CHECK_FALSE(a <  b);
  CHECK_FALSE(a >  b);
  CHECK(a <= b);
  CHECK(a >= b);
}

#if __cpp_impl_three_way_comparison >= 201907L
TEST_CASE("operator<=>") {
  semver::version<> a, b;

  SUBCASE("equal") {
    REQUIRE(semver::parse("1.2.3", a));
    REQUIRE(semver::parse("1.2.3", b));
    REQUIRE((a <=> b) == std::strong_ordering::equal);
    REQUIRE((a <=> a) == std::strong_ordering::equal);
  }

  SUBCASE("less") {
    REQUIRE(semver::parse("1.0.0-alpha", a));
    REQUIRE(semver::parse("1.0.0",       b));
    REQUIRE((a <=> b) == std::strong_ordering::less);

    REQUIRE(semver::parse("1.0.0", a));
    REQUIRE(semver::parse("2.0.0", b));
    REQUIRE((a <=> b) == std::strong_ordering::less);
  }

  SUBCASE("greater") {
    REQUIRE(semver::parse("2.0.0", a));
    REQUIRE(semver::parse("1.0.0", b));
    REQUIRE((a <=> b) == std::strong_ordering::greater);

    REQUIRE(semver::parse("1.0.0",       a));
    REQUIRE(semver::parse("1.0.0-alpha", b));
    REQUIRE((a <=> b) == std::strong_ordering::greater);
  }

  SUBCASE("build metadata ignored in <=>") {
    REQUIRE(semver::parse("1.0.0+build.1",   a));
    REQUIRE(semver::parse("1.0.0+build.999", b));
    REQUIRE((a <=> b) == std::strong_ordering::equal);
  }
}
#endif

TEST_CASE("std::sort compatibility") {
  // version<> is LessThanComparable; std::sort must produce strict ascending order.
  std::vector<semver::version<>> vs(5);
  REQUIRE(semver::parse("2.0.0",       vs[0]));
  REQUIRE(semver::parse("0.9.0",       vs[1]));
  REQUIRE(semver::parse("1.0.0-alpha", vs[2]));
  REQUIRE(semver::parse("1.0.0",       vs[3]));
  REQUIRE(semver::parse("1.0.0-beta",  vs[4]));

  std::sort(vs.begin(), vs.end());

  // Expected ascending order: 0.9.0 < 1.0.0-alpha < 1.0.0-beta < 1.0.0 < 2.0.0
  constexpr std::array<std::string_view, 5> expected_strs = {{
    "0.9.0", "1.0.0-alpha", "1.0.0-beta", "1.0.0", "2.0.0"
  }};
  for (std::size_t i = 0; i < expected_strs.size(); ++i) {
    semver::version<> expected;
    REQUIRE(semver::parse(expected_strs[i], expected));
    CHECK(vs[i] == expected);
  }
}

TEST_CASE("std::set deduplication") {
  std::set<semver::version<>> s;
  semver::version<> v1, v2, v3, v4;
  REQUIRE(semver::parse("1.0.0",       v1));
  REQUIRE(semver::parse("2.0.0",       v2));
  REQUIRE(semver::parse("1.0.0",       v3)); // exact duplicate of v1
  REQUIRE(semver::parse("1.0.0+build", v4)); // same precedence as v1 per spec \u00a710

  s.insert(v1);
  s.insert(v2);
  s.insert(v3);
  REQUIRE(s.size() == 2);

  s.insert(v4); // treated as equal to v1 \u2192 not inserted
  REQUIRE(s.size() == 2);
}

