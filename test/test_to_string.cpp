#include <semver.hpp>
#include <catch.hpp>
#include <sstream>
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

TEST_CASE("operator<<") {
  std::ostringstream os;

  SECTION("basic version") {
    version v;
    REQUIRE(parse("1.2.3", v));
    os << v;
    REQUIRE(os.str() == "1.2.3");
  }

  SECTION("with prerelease and build metadata") {
    version v;
    REQUIRE(parse("1.0.0-alpha.1+build.42", v));
    os << v;
    REQUIRE(os.str() == "1.0.0-alpha.1+build.42");
  }
}