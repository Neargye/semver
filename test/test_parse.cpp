#include <semver.hpp>
#include <catch.hpp>
#include <array>
#include <cstring>

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
      int major = 0, minor = 0, patch = 0;
      REQUIRE(parse(version, major, minor, patch));
      REQUIRE(major == expected.major);
      REQUIRE(minor == expected.minor);
      REQUIRE(patch == expected.patch);
    }
  }

  SECTION("negative") {
    constexpr std::array<std::string_view, 3> versions = {{
      {"0.0.-1"},
      {"0.-1.0"},
      {"-1.0.0"}
    }};

    for (auto version: versions) {
      int major = 0, minor = 0, patch = 0;
      REQUIRE_FALSE(parse(version, major, minor, patch));
    }
  }

  SECTION("leading zero") {
    constexpr std::array<std::string_view, 3> versions = {{
      {"0.0.01"},
      {"0.01.0"},
      {"01.0.0"}
    }};

    for (auto version: versions) {
      int major = 0, minor = 0, patch = 0;
      REQUIRE_FALSE(parse(version, major, minor, patch));
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
      int major = 0, minor = 0, patch = 0;
      REQUIRE_FALSE(parse(version, major, minor, patch));
    }
  }

  SECTION("overflow") {
    constexpr std::string_view v = "128.0.0";
    std::int8_t major = 0, minor = 0, patch = 0;
    REQUIRE_FALSE(parse(v, major, minor, patch));
  }

  SECTION("prerelease") {
    struct version {
      int major, minor, patch;
      std::string_view prerelease;
    };

    constexpr std::array<std::pair<std::string_view, version>, 3> versions = {{
      {"0.0.1-alpha.128", {0, 0, 1, "alpha.128"}},
      {"1.2.3-alpha.beta.rc-45.42", {1, 2, 3, "alpha.beta.rc-45.42"}},
      {"0.0.1-alpha-beta", {0, 0, 1, "alpha-beta"}}
    }};

    for (const auto& [version, expected]: versions) {
      int major = 0, minor = 0, patch = 0;
      std::string prerelease;
      prerelease.resize(expected.prerelease.size());
      REQUIRE(parse(version, major, minor, patch, prerelease.data(), prerelease.data() + prerelease.size()));
      REQUIRE(major == expected.major);
      REQUIRE(minor == expected.minor);
      REQUIRE(patch == expected.patch);
      REQUIRE(prerelease == expected.prerelease);
    }
  }

  SECTION("build-metadata") {
    struct version {
      int major, minor, patch;
      std::string_view build_meta;
    };

    constexpr std::array<std::pair<std::string_view, version>, 3> versions = {{
      {"0.0.1+123", {0, 0, 1, "123"}},
      {"1.2.3+sha.42089", {1, 2, 3, "sha.42089"}},
      {"0.0.1+001-meta-info", {0, 0, 1, "001-meta-info"}}
    }};

    for (const auto& [version, expected]: versions) {
      int major = 0, minor = 0, patch = 0;
      std::string build_meta;
      build_meta.resize(expected.build_meta.size());
      REQUIRE(parse(version, major, minor, patch, nullptr, nullptr,
                    build_meta.data(), build_meta.data() + build_meta.size()));
      REQUIRE(major == expected.major);
      REQUIRE(minor == expected.minor);
      REQUIRE(patch == expected.patch);
      REQUIRE(build_meta == expected.build_meta);
    }
  }

  SECTION("prerelease + build-metadata") {
    struct version {
      int major, minor, patch;
      std::string_view prerelease, build_meta;
    };

    constexpr std::array<std::pair<std::string_view, version>, 3> versions = {{
      {"0.0.1-alpha.128+123", {0, 0, 1, "alpha.128", "123"}},
      {"1.2.3-alpha.beta.rc-45.42+sha.42089", {1, 2, 3, "alpha.beta.rc-45.42", "sha.42089"}},
      {"0.0.1-alpha-beta+001-meta-info", {0, 0, 1, "alpha-beta", "001-meta-info"}}
    }};

    for (const auto& [version, expected]: versions) {
      int major = 0, minor = 0, patch = 0;
      std::string prerelease, build_meta;
      prerelease.resize(expected.prerelease.size());
      build_meta.resize(expected.build_meta.size());
      REQUIRE(parse(version, major, minor, patch,
                    prerelease.data(), prerelease.data() + prerelease.size(),
                    build_meta.data(), build_meta.data() + build_meta.size()));
      REQUIRE(major == expected.major);
      REQUIRE(minor == expected.minor);
      REQUIRE(patch == expected.patch);
      REQUIRE(prerelease == expected.prerelease);
      REQUIRE(build_meta == expected.build_meta);
    }
  }
}