#include <semver.hpp>
#include <catch.hpp>
#include "test_utils.hpp"

using namespace semver;

TEST_CASE("to string") {
  version version;
  CHECK(version.to_string() == "0.1.0");

  for (auto str : valid_versions) {
    REQUIRE(parse(str, version));
    CHECK(version.to_string() == str);
  }
}