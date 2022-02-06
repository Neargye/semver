#include <semver.hpp>
#include "test_dataset.hpp"
#include <catch.hpp>
#include <array>
#include <cstring>

using namespace semver;

TEST_CASE("parse") {
  for (const auto& version: semver::dataset::valid_versions) {
    integer_type major = 0, minor = 0, patch = 0;
    char* prerelease = nullptr, *build_meta = nullptr;
    bool result = parse(version.second, major, minor, patch, prerelease, build_meta);

    REQUIRE(result);
    REQUIRE(major == version.first.major);
    REQUIRE(minor == version.first.minor);
    REQUIRE(patch == version.first.patch);

    if (prerelease) {
      REQUIRE(strcmp(prerelease, version.first.prerelease.data()) == 0);
    }

    if (build_meta) {
      REQUIRE(strcmp(build_meta, version.first.build_metadata.data()) == 0);
    }
  }
}