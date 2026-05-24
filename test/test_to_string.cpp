#include <semver.hpp>
#include <doctest.h>
#include <array>
#include <iterator>
#include <ostream>
#include <sstream>
#include <string>
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

  SUBCASE("uint64_t large patch value") {
    semver::version<uint64_t> v;
    REQUIRE(semver::parse("999999999999.1.0", v));
    REQUIRE(v.to_string() == "999999999999.1.0");
  }

  SUBCASE("complex prerelease and build metadata preserved exactly") {
    semver::version<> v;
    REQUIRE(semver::parse("1.2.3----RC-SNAPSHOT.12.9.1--.12+788", v));
    REQUIRE(v.to_string() == "1.2.3----RC-SNAPSHOT.12.9.1--.12+788");
  }
}

#if __cpp_lib_format >= 202110L
#include <format>
TEST_CASE("std::formatter") {
  semver::version<> v;
  REQUIRE(semver::parse("1.2.3-alpha.1+build.42", v));

  SUBCASE("std::format basic") {
    REQUIRE(std::format("{}", v) == "1.2.3-alpha.1+build.42");
  }

  SUBCASE("std::format_to") {
    std::string out;
    std::format_to(std::back_inserter(out), "{}", v);
    REQUIRE(out == "1.2.3-alpha.1+build.42");
  }

  SUBCASE("default version") {
    semver::version<> def;
    REQUIRE(std::format("{}", def) == "0.1.0");
  }

  SUBCASE("format spec throws format_error") {
    // Use vformat to bypass compile-time format string validation (P2216)
    CHECK_THROWS_AS((void)std::vformat("{:>20}", std::make_format_args(v)), std::format_error);
  }
}
#endif

TEST_CASE("to_chars — zero-allocation serialization") {
  SUBCASE("basic version") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    char buf[32];
    const auto r = semver::to_chars(buf, buf + sizeof(buf), v);
    REQUIRE(r);
    REQUIRE(std::string_view(buf, static_cast<std::size_t>(r.ptr - buf)) == "1.2.3");
  }

  SUBCASE("version with prerelease") {
    version<> v;
    REQUIRE(parse("1.0.0-alpha.1", v));
    char buf[32];
    const auto r = semver::to_chars(buf, buf + sizeof(buf), v);
    REQUIRE(r);
    REQUIRE(std::string_view(buf, static_cast<std::size_t>(r.ptr - buf)) == "1.0.0-alpha.1");
  }

  SUBCASE("version with build metadata") {
    version<> v;
    REQUIRE(parse("2.3.4+build.42", v));
    char buf[32];
    const auto r = semver::to_chars(buf, buf + sizeof(buf), v);
    REQUIRE(r);
    REQUIRE(std::string_view(buf, static_cast<std::size_t>(r.ptr - buf)) == "2.3.4+build.42");
  }

  SUBCASE("version with prerelease and build metadata") {
    version<> v;
    REQUIRE(parse("1.0.0-alpha.1+build.42", v));
    char buf[64];
    const auto r = semver::to_chars(buf, buf + sizeof(buf), v);
    REQUIRE(r);
    REQUIRE(std::string_view(buf, static_cast<std::size_t>(r.ptr - buf)) == "1.0.0-alpha.1+build.42");
  }

  SUBCASE("buffer exactly the right size succeeds") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    char buf[5];
    const auto r = semver::to_chars(buf, buf + 5, v);
    REQUIRE(r);
    REQUIRE(r.ptr == buf + 5);
  }

  SUBCASE("buffer one byte too small fails") {
    version<> v;
    REQUIRE(parse("1.2.3", v));
    char buf[4];
    const auto r = semver::to_chars(buf, buf + 4, v);
    REQUIRE_FALSE(r);
    REQUIRE(r.ec == std::errc::value_too_large);
    REQUIRE(r.ptr == buf + 4);
  }

  SUBCASE("empty buffer fails") {
    version<> v;
    REQUIRE(parse("0.0.0", v));
    char buf[1];
    const auto r = semver::to_chars(buf, buf, v);
    REQUIRE_FALSE(r);
    REQUIRE(r.ec == std::errc::value_too_large);
  }

  SUBCASE("round-trip: to_chars output matches to_string") {
    constexpr std::array<std::string_view, 6> cases = {{
      "0.0.0", "1.2.3", "1.0.0-alpha.1+build", "10.20.30",
      "999999999.999999999.999999999", "1.2.3----RC-SNAPSHOT.12.9.1--.12+788"
    }};
    for (auto sv : cases) {
      version<> v;
      REQUIRE(parse(sv, v));
      char buf[256];
      const auto r = semver::to_chars(buf, buf + sizeof(buf), v);
      REQUIRE(r);
      REQUIRE(std::string_view(buf, static_cast<std::size_t>(r.ptr - buf)) == v.to_string());
    }
  }

  SUBCASE("uint8_t version serializes correctly") {
    version<uint8_t> v;
    REQUIRE(parse("255.0.1", v));
    char buf[16];
    const auto r = semver::to_chars(buf, buf + sizeof(buf), v);
    REQUIRE(r);
    REQUIRE(std::string_view(buf, static_cast<std::size_t>(r.ptr - buf)) == "255.0.1");
  }

  SUBCASE("ptr advances by exactly the written length") {
    version<> v;
    REQUIRE(parse("10.200.3000", v));
    char buf[32] = {};
    const auto r = semver::to_chars(buf, buf + sizeof(buf), v);
    REQUIRE(r);
    const auto written = static_cast<std::size_t>(r.ptr - buf);
    REQUIRE(written == std::string_view("10.200.3000").size());
  }
}
