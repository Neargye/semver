// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2026 Daniil Goncharov <neargye@gmail.com>.

#include <semver.hpp>
#include <doctest.h>
#include <algorithm>
#include <array>
#include <ostream>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <vector>

using namespace semver;

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

    const auto lt = [](const semver::version<>& a, const semver::version<>& b) { return a < b; };
    const auto gt = [](const semver::version<>& a, const semver::version<>& b) { return a > b; };

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
  // versions with the same numeric values but different unsigned widths must compare equal.
  semver::version<std::uint32_t, std::uint32_t, std::uint32_t> a;
  semver::version<unsigned, unsigned, unsigned>                 b;
  semver::version<std::uint64_t, std::uint64_t, std::uint64_t> c;

  REQUIRE(semver::parse("1.2.3", a));
  REQUIRE(semver::parse("1.2.3", b));
  REQUIRE(semver::parse("1.2.3", c));

  SUBCASE("equality across types") {
    REQUIRE(a == b);
    REQUIRE(a == c);
    REQUIRE(b == c);
  }

  SUBCASE("ordering across types") {
    semver::version<std::uint8_t>  lo;
    semver::version<unsigned>      hi;
    REQUIRE(semver::parse("1.2.2", lo));
    REQUIRE(semver::parse("1.2.3", hi));
    REQUIRE(lo < hi);
    REQUIRE(hi > lo);
    REQUIRE(lo <= hi);
    REQUIRE(hi >= lo);
  }

  SUBCASE("prerelease ordering across types") {
    semver::version<std::uint16_t> pre;
    semver::version<unsigned>      rel;
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

#if defined(__cpp_impl_three_way_comparison) && __cpp_impl_three_way_comparison >= 201907L
TEST_CASE("operator<=>") {
  semver::version<> a, b;
  static_assert(std::is_same_v<decltype(a <=> b), std::weak_ordering>);

  SUBCASE("equal") {
    REQUIRE(semver::parse("1.2.3", a));
    REQUIRE(semver::parse("1.2.3", b));
    REQUIRE((a <=> b) == std::weak_ordering::equivalent);
    REQUIRE((a <=> a) == std::weak_ordering::equivalent);
  }

  SUBCASE("less") {
    REQUIRE(semver::parse("1.0.0-alpha", a));
    REQUIRE(semver::parse("1.0.0",       b));
    REQUIRE((a <=> b) == std::weak_ordering::less);

    REQUIRE(semver::parse("1.0.0", a));
    REQUIRE(semver::parse("2.0.0", b));
    REQUIRE((a <=> b) == std::weak_ordering::less);
  }

  SUBCASE("greater") {
    REQUIRE(semver::parse("2.0.0", a));
    REQUIRE(semver::parse("1.0.0", b));
    REQUIRE((a <=> b) == std::weak_ordering::greater);

    REQUIRE(semver::parse("1.0.0",       a));
    REQUIRE(semver::parse("1.0.0-alpha", b));
    REQUIRE((a <=> b) == std::weak_ordering::greater);
  }

  SUBCASE("build metadata ignored in <=>") {
    REQUIRE(semver::parse("1.0.0+build.1",   a));
    REQUIRE(semver::parse("1.0.0+build.999", b));
    REQUIRE((a <=> b) == std::weak_ordering::equivalent);
  }

  SUBCASE("mixed component types") {
    const semver::version<std::uint8_t> narrow{1, 2, 3, "alpha"};
    const semver::version<std::uint64_t> wide{1, 2, 3, "beta"};
    static_assert(std::is_same_v<decltype(narrow <=> wide), std::weak_ordering>);
    REQUIRE((narrow <=> wide) == std::weak_ordering::less);
    REQUIRE((wide <=> narrow) == std::weak_ordering::greater);
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
TEST_CASE("bump versions") {
  SUBCASE("bump_major increments major, resets minor and patch to 0") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    const auto b = v.bump_major();
    REQUIRE(b.major() == 2);
    REQUIRE(b.minor() == 0);
    REQUIRE(b.patch() == 0);
  }

  SUBCASE("bump_minor increments minor, resets patch to 0, keeps major") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    const auto b = v.bump_minor();
    REQUIRE(b.major() == 1);
    REQUIRE(b.minor() == 3);
    REQUIRE(b.patch() == 0);
  }

  SUBCASE("bump_patch increments patch only") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    const auto b = v.bump_patch();
    REQUIRE(b.major() == 1);
    REQUIRE(b.minor() == 2);
    REQUIRE(b.patch() == 4);
  }

  SUBCASE("bump_major clears pre-release tag") {
    version<> v;
    REQUIRE(parse("1.2.3-alpha.1", v));
    REQUIRE(v.bump_major().prerelease_tag().empty());
  }

  SUBCASE("bump_minor clears pre-release tag and resets patch") {
    version<> v;
    REQUIRE(parse("1.2.3-rc.2", v));
    const auto b = v.bump_minor();
    REQUIRE(b.prerelease_tag().empty());
    REQUIRE(b.minor() == 3);
    REQUIRE(b.patch() == 0);
  }

  SUBCASE("bump_patch clears pre-release tag") {
    version<> v;
    REQUIRE(parse("1.2.3-beta", v));
    const auto b = v.bump_patch();
    REQUIRE(b.prerelease_tag().empty());
    REQUIRE(b.patch() == 4);
  }

  SUBCASE("bump_major clears build metadata") {
    version<> v;
    REQUIRE(parse("1.2.3+build.42", v));
    REQUIRE(v.bump_major().build_metadata().empty());
  }

  SUBCASE("bump_patch clears both prerelease and build metadata") {
    version<> v;
    REQUIRE(parse("1.2.3-alpha+build", v));
    const auto b = v.bump_patch();
    REQUIRE(b.prerelease_tag().empty());
    REQUIRE(b.build_metadata().empty());
    REQUIRE(b.patch() == 4);
  }

  SUBCASE("without_prerelease preserves the core and build metadata") {
    version<> v;
    REQUIRE(parse("1.2.3-rc.2+build.7", v));
    const auto release = v.without_prerelease();
    REQUIRE(release.to_string() == "1.2.3+build.7");
    REQUIRE(v.to_string() == "1.2.3-rc.2+build.7");
  }

  SUBCASE("without_prerelease leaves a release unchanged") {
    version<> v;
    REQUIRE(parse("1.2.3+build.7", v));
    REQUIRE(v.without_prerelease().to_string() == "1.2.3+build.7");
  }

  SUBCASE("without_build_metadata preserves the core and prerelease tag") {
    version<> v;
    REQUIRE(parse("1.2.3-rc.2+build.7", v));
    const auto reproducible = v.without_build_metadata();
    REQUIRE(reproducible.to_string() == "1.2.3-rc.2");
    REQUIRE(v.to_string() == "1.2.3-rc.2+build.7");
  }

  SUBCASE("without_build_metadata leaves a version without metadata unchanged") {
    version<> v;
    REQUIRE(parse("1.2.3-rc.2", v));
    REQUIRE(v.without_build_metadata().to_string() == "1.2.3-rc.2");
  }

  SUBCASE("original version is not modified (bump is const)") {
    version<> v;
    REQUIRE(parse("1.2.3-alpha", v));
    (void)v.bump_major();
    REQUIRE(v.major() == 1);
    REQUIRE(v.prerelease_tag() == "alpha");
  }

  SUBCASE("bump from default version 0.1.0") {
    const version<> v;
    REQUIRE(v.bump_major().major() == 1);
    REQUIRE(v.bump_major().minor() == 0);
    REQUIRE(v.bump_minor().minor() == 2);
    REQUIRE(v.bump_patch().patch() == 1);
  }

  SUBCASE("chaining: bump_major then bump_minor") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    const auto chain = v.bump_major().bump_minor();
    REQUIRE(chain.major() == 2);
    REQUIRE(chain.minor() == 1);
    REQUIRE(chain.patch() == 0);
  }

  SUBCASE("chaining: bump_minor then bump_patch") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    const auto chain = v.bump_minor().bump_patch();
    REQUIRE(chain.major() == 1);
    REQUIRE(chain.minor() == 3);
    REQUIRE(chain.patch() == 1);
  }

  SUBCASE("uint8_t type parameter works with bump") {
    version<uint8_t> v;
    REQUIRE(parse("1.2.3", v));
    const auto bm = v.bump_major();
    REQUIRE(bm.major() == 2);
    REQUIRE(bm.minor() == 0);
    REQUIRE(bm.patch() == 0);
  }

  SUBCASE("overflow throws instead of wrapping in release builds") {
    const version<uint8_t> max_major{uint8_t{255}, uint8_t{0}, uint8_t{0}};
    const version<uint8_t> max_minor{uint8_t{1}, uint8_t{255}, uint8_t{0}};
    const version<uint8_t> max_patch{uint8_t{1}, uint8_t{2}, uint8_t{255}};

    CHECK_THROWS_AS((void)max_major.bump_major(), std::overflow_error);
    CHECK_THROWS_AS((void)max_minor.bump_minor(), std::overflow_error);
    CHECK_THROWS_AS((void)max_patch.bump_patch(), std::overflow_error);
  }

  SUBCASE("bump_major on 0.x.x") {
    version<> v;
    REQUIRE(parse("0.9.0", v));
    const auto b = v.bump_major();
    REQUIRE(b.major() == 1);
    REQUIRE(b.minor() == 0);
    REQUIRE(b.patch() == 0);
  }
}

TEST_CASE("is_prerelease and has_build_metadata") {
  SUBCASE("plain version has neither") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    REQUIRE_FALSE(v.is_prerelease());
    REQUIRE_FALSE(v.has_build_metadata());
  }

  SUBCASE("version with prerelease tag") {
    version<> v;
    REQUIRE(parse("1.2.3-alpha.1", v));
    REQUIRE(v.is_prerelease());
    REQUIRE_FALSE(v.has_build_metadata());
  }

  SUBCASE("version with build metadata") {
    version<> v;
    REQUIRE(parse("1.2.3+build.1", v));
    REQUIRE_FALSE(v.is_prerelease());
    REQUIRE(v.has_build_metadata());
  }

  SUBCASE("version with both") {
    version<> v;
    REQUIRE(parse("1.2.3-rc.1+sha.abc", v));
    REQUIRE(v.is_prerelease());
    REQUIRE(v.has_build_metadata());
  }
}

TEST_CASE("compare_with_build") {
  SUBCASE("identical versions with same build return 0") {
    version<> a, b;
    REQUIRE(parse("1.0.0+build.1", a));
    REQUIRE(parse("1.0.0+build.1", b));
    REQUIRE(compare_with_build(a, b) == 0);
  }

  SUBCASE("identical versions with different build are not equal") {
    version<> a, b;
    REQUIRE(parse("1.0.0+build.1", a));
    REQUIRE(parse("1.0.0+build.2", b));
    REQUIRE(compare_with_build(a, b) != 0);
  }

  SUBCASE("lower version is less even with large build metadata") {
    version<> a, b;
    REQUIRE(parse("1.0.0+build.99", a));
    REQUIRE(parse("2.0.0+build.1", b));
    REQUIRE(compare_with_build(a, b) < 0);
  }

  SUBCASE("no build metadata sorts before build metadata") {
    version<> a, b;
    REQUIRE(parse("1.0.0", a));
    REQUIRE(parse("1.0.0+build", b));
    REQUIRE(compare_with_build(a, b) < 0);
  }
}

TEST_CASE("diff") {
  SUBCASE("same version returns none") {
    version<> a, b;
    REQUIRE(parse("1.2.3", a));
    REQUIRE(parse("1.2.3", b));
    REQUIRE(diff(a, b) == version_change::none);
  }

  SUBCASE("major differs") {
    version<> a, b;
    REQUIRE(parse("1.0.0", a));
    REQUIRE(parse("2.0.0", b));
    REQUIRE(diff(a, b) == version_change::major);
    REQUIRE(diff(b, a) == version_change::major);
  }

  SUBCASE("minor differs") {
    version<> a, b;
    REQUIRE(parse("1.1.0", a));
    REQUIRE(parse("1.2.0", b));
    REQUIRE(diff(a, b) == version_change::minor);
  }

  SUBCASE("patch differs") {
    version<> a, b;
    REQUIRE(parse("1.0.0", a));
    REQUIRE(parse("1.0.1", b));
    REQUIRE(diff(a, b) == version_change::patch);
  }

  SUBCASE("major differs, newer has prerelease yields premajor") {
    version<> a, b;
    REQUIRE(parse("1.0.0", a));
    REQUIRE(parse("2.0.0-alpha", b));
    REQUIRE(diff(a, b) == version_change::premajor);
  }

  SUBCASE("minor differs, newer has prerelease yields preminor") {
    version<> a, b;
    REQUIRE(parse("1.0.0", a));
    REQUIRE(parse("1.1.0-beta", b));
    REQUIRE(diff(a, b) == version_change::preminor);
  }

  SUBCASE("patch differs, newer has prerelease yields prepatch") {
    version<> a, b;
    REQUIRE(parse("1.0.0", a));
    REQUIRE(parse("1.0.1-rc.1", b));
    REQUIRE(diff(a, b) == version_change::prepatch);
  }

  SUBCASE("same release, only prerelease differs yields prerelease") {
    version<> a, b;
    REQUIRE(parse("1.0.0-alpha", a));
    REQUIRE(parse("1.0.0-beta", b));
    REQUIRE(diff(a, b) == version_change::prerelease);
  }

  SUBCASE("same version including prerelease returns none") {
    version<> a, b;
    REQUIRE(parse("1.0.0-alpha.1", a));
    REQUIRE(parse("1.0.0-alpha.1", b));
    REQUIRE(diff(a, b) == version_change::none);
  }

  SUBCASE("build metadata is ignored") {
    version<> a, b;
    REQUIRE(parse("1.0.0+build.1", a));
    REQUIRE(parse("1.0.0+build.2", b));
    REQUIRE(diff(a, b) == version_change::none);
  }

  SUBCASE("mixed component widths compare without narrowing") {
    const version<std::uint8_t> narrow{255, 1, 2};
    const version<std::uint64_t> equal{255, 1, 2};
    const version<std::uint64_t> greater{256, 1, 2};

    REQUIRE(diff(narrow, equal) == version_change::none);
    REQUIRE(diff(narrow, greater) == version_change::major);
  }
}

TEST_CASE("inc") {
  SUBCASE("none returns nullopt") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    REQUIRE_FALSE(inc(v, version_change::none).has_value());
  }

  SUBCASE("major bumps major, resets minor and patch") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    const auto r = inc(v, version_change::major);
    REQUIRE(r.has_value());
    REQUIRE(r->major() == 2);
    REQUIRE(r->minor() == 0);
    REQUIRE(r->patch() == 0);
    REQUIRE(r->prerelease_tag().empty());
  }

  SUBCASE("minor bumps minor, resets patch") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    const auto r = inc(v, version_change::minor);
    REQUIRE(r.has_value());
    REQUIRE(r->major() == 1);
    REQUIRE(r->minor() == 3);
    REQUIRE(r->patch() == 0);
  }

  SUBCASE("patch bumps patch") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    const auto r = inc(v, version_change::patch);
    REQUIRE(r.has_value());
    REQUIRE(r->major() == 1);
    REQUIRE(r->minor() == 2);
    REQUIRE(r->patch() == 4);
  }

  SUBCASE("premajor with explicit pre tag") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    const auto r = inc(v, version_change::premajor, "alpha");
    REQUIRE(r.has_value());
    REQUIRE(r->major() == 2);
    REQUIRE(r->minor() == 0);
    REQUIRE(r->patch() == 0);
    REQUIRE(r->prerelease_tag() == "alpha");
  }

  SUBCASE("premajor default pre is 0") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    const auto r = inc(v, version_change::premajor);
    REQUIRE(r.has_value());
    REQUIRE(r->prerelease_tag() == "0");
  }

  SUBCASE("prerelease bumps last numeric identifier") {
    version<> v;
    REQUIRE(parse("1.2.3-alpha.1", v));
    const auto r = inc(v, version_change::prerelease);
    REQUIRE(r.has_value());
    REQUIRE(r->major() == 1);
    REQUIRE(r->minor() == 2);
    REQUIRE(r->patch() == 3);
    REQUIRE(r->prerelease_tag() == "alpha.2");
  }

  SUBCASE("prerelease: non-numeric last identifier appends .0") {
    version<> v;
    REQUIRE(parse("1.2.3-beta", v));
    const auto r = inc(v, version_change::prerelease);
    REQUIRE(r.has_value());
    REQUIRE(r->prerelease_tag() == "beta.0");
  }

  SUBCASE("prerelease on release version bumps patch and sets -0") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    const auto r = inc(v, version_change::prerelease);
    REQUIRE(r.has_value());
    REQUIRE(r->patch() == 4);
    REQUIRE(r->prerelease_tag() == "0");
  }

  SUBCASE("invalid pre tag returns nullopt") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    REQUIRE_FALSE(inc(v, version_change::premajor, "bad pre!").has_value());
  }

  SUBCASE("pre tag is rejected for stable increments") {
    version<> v;
    REQUIRE(parse("1.2.3", v));

    CHECK_FALSE(inc(v, version_change::major, "beta").has_value());
    CHECK_FALSE(inc(v, version_change::minor, "rc.1").has_value());
    CHECK_FALSE(inc(v, version_change::patch, "0").has_value());
  }

  SUBCASE("stable increments are arithmetic for prerelease versions") {
    version<> v;
    REQUIRE(parse("1.2.3-rc.1+build", v));

    const auto major = inc(v, version_change::major);
    const auto minor = inc(v, version_change::minor);
    const auto patch = inc(v, version_change::patch);
    REQUIRE(major.has_value());
    REQUIRE(minor.has_value());
    REQUIRE(patch.has_value());
    CHECK(major->to_string() == "2.0.0");
    CHECK(minor->to_string() == "1.3.0");
    CHECK(patch->to_string() == "1.2.4");
  }
}

TEST_CASE("compare") {
  version<> v100, v110, v200, v100pre;
  REQUIRE(parse("1.0.0",       v100));
  REQUIRE(parse("1.1.0",       v110));
  REQUIRE(parse("2.0.0",       v200));
  REQUIRE(parse("1.0.0-alpha", v100pre));

  SUBCASE("compare: equal returns 0") {
    REQUIRE(compare(v100, v100) == 0);
  }

  SUBCASE("compare: lower < higher returns -1") {
    REQUIRE(compare(v100, v110) == -1);
    REQUIRE(compare(v110, v200) == -1);
  }

  SUBCASE("compare: higher > lower returns 1") {
    REQUIRE(compare(v200, v100) == 1);
  }

  SUBCASE("compare: pre-release < release") {
    REQUIRE(compare(v100pre, v100) == -1);
    REQUIRE(compare(v100, v100pre) == 1);
  }

}

TEST_CASE("comparison and hash laws hold for representative versions") {
  constexpr std::array<std::string_view, 16> texts{{
    "0.0.0-0", "0.0.0-alpha", "0.0.0", "1.0.0-alpha",
    "1.0.0-alpha.1", "1.0.0-alpha.beta", "1.0.0-beta",
    "1.0.0-beta.2", "1.0.0-beta.11", "1.0.0-rc.1", "1.0.0",
    "1.0.0+build.1", "1.0.0+build.2", "1.0.1", "1.1.0", "2.0.0"
  }};

  std::array<semver::version<>, texts.size()> versions;
  for (std::size_t i = 0; i < texts.size(); ++i)
    REQUIRE(semver::parse(texts[i], versions[i]));

  for (std::size_t i = 0; i < versions.size(); ++i) {
    for (std::size_t j = 0; j < versions.size(); ++j) {
      const auto ij = semver::compare(versions[i], versions[j]);
      const auto ji = semver::compare(versions[j], versions[i]);
      CAPTURE(texts[i]);
      CAPTURE(texts[j]);
      CHECK(ij == -ji);
      CHECK((ij == 0) == (versions[i] == versions[j]));
      if (versions[i] == versions[j])
        CHECK(std::hash<semver::version<>>{}(versions[i]) == std::hash<semver::version<>>{}(versions[j]));

      for (std::size_t k = 0; k < versions.size(); ++k) {
        if (ij <= 0 && semver::compare(versions[j], versions[k]) <= 0) {
          CAPTURE(texts[k]);
          CHECK(semver::compare(versions[i], versions[k]) <= 0);
        }
      }
    }
  }
}
