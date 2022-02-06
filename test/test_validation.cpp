#include <semver.hpp>
#include "test_dataset.hpp"
#include <catch.hpp>
#include <array>

using namespace semver;

TEST_CASE("validation") {
  for (auto version: dataset::valid_versions) {
//    REQUIRE(valid(version));
  }

  for (auto version: dataset::invalid_versions) {
//    REQUIRE_FALSE(valid(version));
  }
}
