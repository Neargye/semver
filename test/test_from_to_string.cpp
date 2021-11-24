#include <semver.hpp>
#include <catch.hpp>
#include <array>

using namespace semver;

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

  constexpr std::array<std::pair<std::string_view, std::ptrdiff_t>, 45> invalid_versions = {{
    {"1", 1},
    {"1.2", 3},
    {"1.2.3-0123", 6},
    {"1.2.3-0123.0123", 6},
    {"1.0.0-", 6},
    {"1.0.0+", 6},
    {"1.0.0-.", 6},
    {"1.0.0+.", 6},
    {"1.0.0-.+.", 6},
    {"1.1.2+.123", 6},
    {"+invalid", 0},
    {"-invalid", 0},
    {"-invalid+invalid", 0},
    {"-invalid.01", 0},
    {"alpha", 0},
    {"alpha.beta", 0},
    {"alpha.beta.1", 0},
    {"alpha.1", 0},
    {"alpha+beta", 0},
    {"alpha_beta", 0},
    {"alpha.", 0},
    {"alpha..", 0},
    {"beta", 0},
    {"1.0.0-alpha_beta", 11},
    {"-alpha.", 0},
    {"1.0.0-alpha..", 12},
    {"1.0.0-alpha..1", 12},
    {"1.0.0-alpha...1", 12},
    {"1.0.0-alpha....1", 12},
    {"1.0.0-alpha.....1", 12},
    {"1.0.0-alpha......1", 12},
    {"1.0.0-alpha.......1", 12},
    {"01.1.1", 0},
    {"1.01.1", 2},
    {"1.1.01", 4},
    {"1.2.", 4},
    {"1.2.3.DEV", 5},
    {"1.2-SNAPSHOT", 3},
    {"1.2.31.2.3----RC-SNAPSHOT.12.09.1--..12+788", 6},
    {"1.2-RC-SNAPSHOT", 3},
    {"-1.0.3-gamma+b7718", 0},
    {"+justmeta", 0},
    {"9.8.7+meta+meta", 10},
    {"9.8.7-whatever+meta+meta", 19},
    {"99999999999999999999999.999999999999999999.99999999999999999----RC-SNAPSHOT.12.09.1--------------------------------..12", 0}
  }};

  SECTION("from chars") {
    for (const auto& [ver, str] : valid_versions) {
      version v;
      const auto result = v.from_chars(str.data(), str.data() + str.size());
      REQUIRE(result);
      REQUIRE(result.ptr != nullptr);
      REQUIRE(result.ptr == str.data() + str.size());
      REQUIRE(v == ver);
    }

    for (const auto& [str, inv] : invalid_versions) {
      version v;
      const auto result = v.from_chars(str.data(), str.data() + str.size());
      REQUIRE_FALSE(result);
      REQUIRE(result.ptr != nullptr);
      REQUIRE(result.ptr == str.data() + inv);
    }
  }

  SECTION("to chars") {
    for (const auto& [ver, str] : valid_versions) {
      std::array<char, 255> m = {'\0'};
      const auto result = ver.to_chars(m.data(), m.data() + m.size());
      const auto s = std::string_view{m.data()};
      REQUIRE(result);
      REQUIRE(result.ptr != nullptr);
      REQUIRE(result.ptr == s.data() + s.size());
      REQUIRE(s == str);
    }
  }

  SECTION("from string") {
    for (const auto& [ver, str] : valid_versions) {
      version v;
      v.from_string(str);
      REQUIRE(v == ver);
    }

    for (const auto& [str, inv] : invalid_versions) {
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

    for (const auto& [str, inv] : invalid_versions) {
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
