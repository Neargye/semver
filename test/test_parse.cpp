#include <semver.hpp>
#include <catch.hpp>
#include <array>

using namespace semver;

TEST_CASE("parse") {
  SECTION("simple") {
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

  SECTION("negative") {
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

  SECTION("leading zero") {
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

  SECTION("incomplete") {
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

  SECTION("overflow") {
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

  SECTION("prerelease") {
    struct version {
      int major, minor, patch;
      std::string_view prerelease_tag;
    };

    constexpr std::array<std::pair<std::string_view, version>, 3> versions = {{
      {"0.0.1-alpha.128", {0, 0, 1, "alpha.128"}},
      {"1.2.3-alpha.beta.rc-45.42", {1, 2, 3, "alpha.beta.rc-45.42"}},
      {"0.0.1-alpha-beta", {0, 0, 1, "alpha-beta"}}
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

  SECTION("build-metadata") {
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

  SECTION("prerelease + build-metadata") {
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
}