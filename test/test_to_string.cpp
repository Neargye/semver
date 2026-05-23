#include <semver.hpp>
#include <doctest.h>
#include <ostream>
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

  SUBCASE("basic version") {
    version v;
    REQUIRE(parse("1.2.3", v));
    os << v;
    REQUIRE(os.str() == "1.2.3");
  }

  SUBCASE("with prerelease and build metadata") {
    version v;
    REQUIRE(parse("1.0.0-alpha.1+build.42", v));
    os << v;
    REQUIRE(os.str() == "1.0.0-alpha.1+build.42");
  }
}

TEST_CASE("to_string with various integer widths") {
  SUBCASE("uint8_t boundary value") {
    semver::version<uint8_t> v;
    REQUIRE(semver::parse("255.0.0", v));
    REQUIRE(v.to_string() == "255.0.0");
  }

  SUBCASE("uint16_t boundary value") {
    semver::version<uint16_t> v;
    REQUIRE(semver::parse("65535.0.0", v));
    REQUIRE(v.to_string() == "65535.0.0");
  }

  SUBCASE("uint32_t boundary value") {
    semver::version<uint32_t> v;
    REQUIRE(semver::parse("4294967295.0.0", v));
    REQUIRE(v.to_string() == "4294967295.0.0");
  }

  SUBCASE("int64_t large patch value") {
    semver::version<int64_t> v;
    REQUIRE(semver::parse("999999999999.1.0", v));
    REQUIRE(v.to_string() == "999999999999.1.0");
  }

  SUBCASE("complex prerelease and build metadata preserved exactly") {
    semver::version<> v;
    REQUIRE(semver::parse("1.2.3----RC-SNAPSHOT.12.9.1--.12+788", v));
    REQUIRE(v.to_string() == "1.2.3----RC-SNAPSHOT.12.9.1--.12+788");
  }
}
