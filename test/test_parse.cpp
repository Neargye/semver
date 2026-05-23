#include <semver.hpp>
#include <doctest.h>
#include <array>
#include <ostream>

using namespace semver;

TEST_CASE("parse") {
  SUBCASE("simple") {
    struct version {
      int major, minor, patch;
    };

    constexpr std::array<std::pair<std::string_view, version>, 8> versions = {{
      {"0.0.0", {0,0,0}},
      {"0.0.1", {0,0,1}},
      {"0.1.0", {0,1,0}},
      {"0.1.1", {0,1,1}},
      {"1.0.0", {1,0,0}},
      {"1.0.1", {1,0,1}},
      {"1.1.0", {1,1,0}},
      {"1.1.1", {1,1,1}}
    }};

    for (const auto& [version, expected]: versions) {
      semver::version result;
      REQUIRE(parse(version, result));
      REQUIRE(result.major() == expected.major);
      REQUIRE(result.minor() == expected.minor);
      REQUIRE(result.patch() == expected.patch);
    }
  }

  SUBCASE("negative") {
    constexpr std::array<std::string_view, 3> versions = {{
      {"0.0.-1"},
      {"0.-1.0"},
      {"-1.0.0"}
    }};

    for (auto version: versions) {
      semver::version result;
      REQUIRE_FALSE(parse(version, result));
    }
  }

  SUBCASE("leading zero") {
    constexpr std::array<std::string_view, 3> versions = {{
      {"0.0.01"},
      {"0.01.0"},
      {"01.0.0"}
    }};

    for (auto version: versions) {
      semver::version result;
      REQUIRE_FALSE(parse(version, result));
    }
  }

  SUBCASE("incomplete") {
    constexpr std::array<std::string_view, 7> versions = {{
      "",
      "1.",
      "1.*",
      "1.0",
      "1.0.",
      "1.0.*",
      "*"
    }};

    for (auto version: versions) {
      semver::version result;
      REQUIRE_FALSE(parse(version, result));
    }
  }

  SUBCASE("overflow") {
    constexpr std::string_view v = "0.0.128";

    semver::version<std::int8_t, std::int8_t, std::int8_t> result;
    REQUIRE_FALSE(parse(v, result));

    semver::version<std::int16_t, std::int16_t, std::int16_t> result2;
    REQUIRE(parse(v, result2));
    REQUIRE(result2.major() == 0);
    REQUIRE(result2.minor() == 0);
    REQUIRE(result2.patch() == 128);

    constexpr std::string_view v2 = "0.4294967296.0";
    semver::version<std::int32_t, std::int32_t, std::int32_t> result3;
    REQUIRE_FALSE(parse(v2, result3));

    semver::version<std::int64_t, std::int64_t, std::int64_t> result4;
    REQUIRE(parse(v2, result4));
    REQUIRE(result4.major() == 0);
    REQUIRE(result4.minor() == 4294967296);
    REQUIRE(result4.patch() == 0);
  }

  SUBCASE("prerelease") {
    struct version {
      int major, minor, patch;
      std::string_view prerelease_tag;
    };

    constexpr std::array<std::pair<std::string_view, version>, 4> versions = {{
      {"0.0.1-alpha.128", {0, 0, 1, "alpha.128"}},
      {"1.2.3-alpha.beta.rc-45.42", {1, 2, 3, "alpha.beta.rc-45.42"}},
      {"0.0.1-alpha-beta", {0, 0, 1, "alpha-beta"}},
      {"1.0.1-alpha.5-114-ga2f3905", {1, 0, 1, "alpha.5-114-ga2f3905"}}
    }};

    for (const auto& [version, expected]: versions) {
      semver::version result;
      REQUIRE(parse(version, result));
      REQUIRE(result.major() == expected.major);
      REQUIRE(result.minor() == expected.minor);
      REQUIRE(result.patch() == expected.patch);
      REQUIRE(result.prerelease_tag() == expected.prerelease_tag);
    }
  }

  SUBCASE("build-metadata") {
    struct version {
      int major, minor, patch;
      std::string_view build_metadata;
    };

    constexpr std::array<std::pair<std::string_view, version>, 3> versions = {{
      {"0.0.1+123", {0, 0, 1, "123"}},
      {"1.2.3+sha.42089", {1, 2, 3, "sha.42089"}},
      {"0.0.1+001-meta-info", {0, 0, 1, "001-meta-info"}}
    }};

    for (const auto& [version, expected]: versions) {
      semver::version result;
      REQUIRE(parse(version, result));
      REQUIRE(result.major() == expected.major);
      REQUIRE(result.minor() == expected.minor);
      REQUIRE(result.patch() == expected.patch);
      REQUIRE(result.prerelease_tag().empty());
      REQUIRE(result.build_metadata() == expected.build_metadata);
    }
  }

  SUBCASE("prerelease + build-metadata") {
    struct version {
      int major, minor, patch;
      std::string_view prerelease_tag, build_metadata;
    };

    constexpr std::array<std::pair<std::string_view, version>, 3> versions = {{
      {"0.0.1-alpha.128+123", {0, 0, 1, "alpha.128", "123"}},
      {"1.2.3-alpha.beta.rc-45.42+sha.42089", {1, 2, 3, "alpha.beta.rc-45.42", "sha.42089"}},
      {"0.0.1-alpha-beta+001-meta-info", {0, 0, 1, "alpha-beta", "001-meta-info"}}
    }};

    for (const auto& [version, expected]: versions) {
      semver::version result;
      REQUIRE(parse(version, result));
      REQUIRE(result.major() == expected.major);
      REQUIRE(result.minor() == expected.minor);
      REQUIRE(result.patch() == expected.patch);
      REQUIRE(result.prerelease_tag() == expected.prerelease_tag);
      REQUIRE(result.build_metadata() == expected.build_metadata);
    }
  }

  SUBCASE("alphanumeric prerelease with embedded digits") {
    // Regression: identifiers like "PullRequest0434" (letter-then-digit) must
    // NOT be rejected as leading-zero numeric identifiers (spec §9).
    semver::version v;
    REQUIRE(parse("0.16.411-PullRequest0434.25+45cf6d2", v));
    REQUIRE(v.major() == 0);
    REQUIRE(v.minor() == 16);
    REQUIRE(v.patch() == 411);
    REQUIRE(v.prerelease_tag() == "PullRequest0434.25");
    REQUIRE(v.build_metadata() == "45cf6d2");
  }
}

TEST_CASE("construct") {
  SUBCASE("version(major, minor, patch) constructor") {
    const semver::version v{1, 2, 3};
    REQUIRE(v.major() == 1);
    REQUIRE(v.minor() == 2);
    REQUIRE(v.patch() == 3);
    REQUIRE(v.prerelease_tag().empty());
    REQUIRE(v.build_metadata().empty());

    // equals a parsed version
    semver::version parsed;
    REQUIRE(semver::parse("1.2.3", parsed));
    REQUIRE(v == parsed);
  }

  SUBCASE("single-type template version<uint32_t>") {
    // Issue: version<uint32_t> should work without spelling out all three params
    semver::version<uint32_t> v{1u, 0u, 0u};
    REQUIRE(v.major() == 1u);
    REQUIRE(v.minor() == 0u);
    REQUIRE(v.patch() == 0u);

    semver::version<uint32_t> parsed;
    REQUIRE(semver::parse("1.0.0", parsed));
    REQUIRE(v == parsed);
  }
}

TEST_CASE("parse number overflow") {
  // Regression: a 20+-digit component must NOT silently wrap around.
  // Before the fix, uint64_t overflow produced a valid-looking but wrong value.
  semver::version<> v;

  SUBCASE("20-digit major wraps uint64_t — must be rejected") {
    // 18446744073709551616 == 2^64, overflows uint64_t to 0
    auto result = semver::parse("18446744073709551616.0.0", v);
    REQUIRE_FALSE(result);
    REQUIRE(result.ec == std::errc::result_out_of_range);
  }

  SUBCASE("all-nines 20-digit component") {
    auto result = semver::parse("99999999999999999999.0.0", v);
    REQUIRE_FALSE(result);
    REQUIRE(result.ec == std::errc::result_out_of_range);
  }

  SUBCASE("uint64_t max is still accepted for uint64_t version") {
    // 18446744073709551615 == UINT64_MAX — valid for uint64_t components
    semver::version<uint64_t> big;
    REQUIRE(semver::parse("18446744073709551615.0.0", big));
    REQUIRE(big.major() == std::numeric_limits<uint64_t>::max());
  }
}

TEST_CASE("from_chars_result contract") {
  SUBCASE("ptr points past last consumed byte on success") {
    semver::version v;
    constexpr std::string_view sv = "1.2.3-alpha.1+build";
    const auto [ptr, ec] = semver::parse(sv, v);
    REQUIRE(ec == std::errc{});
    REQUIRE(ptr == sv.data() + sv.size());
  }

  SUBCASE("trailing garbage fails parse") {
    semver::version v;
    REQUIRE_FALSE(semver::parse("1.2.3garbage", v));
    REQUIRE_FALSE(semver::parse("1.2.3 ",        v)); // trailing space
    REQUIRE_FALSE(semver::parse("1.2.3.4",        v)); // extra component
  }

  SUBCASE("errc::invalid_argument for structurally malformed input") {
    semver::version v;
    auto result = semver::parse("not-a-version", v);
    REQUIRE_FALSE(result);
    REQUIRE(result.ec == std::errc::invalid_argument);
  }

  SUBCASE("errc::result_out_of_range for typed component overflow") {
    semver::version<int8_t> v;
    auto result = semver::parse("128.0.0", v); // int8_t max is 127
    REQUIRE_FALSE(result);
    REQUIRE(result.ec == std::errc::result_out_of_range);
  }
}

TEST_CASE("prerelease precedence \u2014 semver spec \u00a711") {
  // \u00a711.4: numeric-only identifiers compared as integers; alphanumeric compared
  // lexically; numeric always has lower precedence than alphanumeric;
  // more fields > fewer fields when the prefix is equal.

  SUBCASE("numeric identifiers compared as integers, not strings") {
    // \"9\" > \"10\" lexicographically, but 9 < 10 as integers
    semver::version v9, v10;
    REQUIRE(semver::parse("1.0.0-9",  v9));
    REQUIRE(semver::parse("1.0.0-10", v10));
    REQUIRE(v9 < v10);
  }

  SUBCASE("numeric identifier always less than alphanumeric (\u00a711.4.1)") {
    semver::version vnum, valpha;
    REQUIRE(semver::parse("1.0.0-1",     vnum));
    REQUIRE(semver::parse("1.0.0-alpha", valpha));
    REQUIRE(vnum < valpha);
  }

  SUBCASE("fewer fields < more fields when prefix equal (\u00a711.4.4)") {
    semver::version vfew, vmore;
    REQUIRE(semver::parse("1.0.0-alpha",   vfew));
    REQUIRE(semver::parse("1.0.0-alpha.1", vmore));
    REQUIRE(vfew < vmore);
  }

  SUBCASE("all-numeric prerelease fields valid and ordered numerically") {
    semver::version v0, v1, v10;
    REQUIRE(semver::parse("1.0.0-0",  v0));
    REQUIRE(semver::parse("1.0.0-1",  v1));
    REQUIRE(semver::parse("1.0.0-10", v10));
    REQUIRE(v0 < v1);
    REQUIRE(v1 < v10);
  }

  SUBCASE("alphanumeric identifiers compared lexically") {
    semver::version va, vb, vz;
    REQUIRE(semver::parse("1.0.0-alpha", va));
    REQUIRE(semver::parse("1.0.0-beta",  vb));
    REQUIRE(semver::parse("1.0.0-zeta",  vz));
    REQUIRE(va < vb);
    REQUIRE(vb < vz);
  }

  SUBCASE("spec example chain: alpha < alpha.1 < alpha.beta < \u2026 < 1.0.0") {
    // The ordering example from https://semver.org/#spec-item-11
    constexpr std::array<std::string_view, 8> chain = {{
      "1.0.0-alpha", "1.0.0-alpha.1", "1.0.0-alpha.beta",
      "1.0.0-beta",  "1.0.0-beta.2",  "1.0.0-beta.11",
      "1.0.0-rc.1",  "1.0.0"
    }};
    for (std::size_t i = 1; i < chain.size(); ++i) {
      semver::version lo, hi;
      REQUIRE(semver::parse(chain[i - 1], lo));
      REQUIRE(semver::parse(chain[i],     hi));
      REQUIRE(lo < hi);
      REQUIRE(hi > lo);
    }
  }
}


