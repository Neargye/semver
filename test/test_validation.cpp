#include <semver.hpp>
#include <catch.hpp>
#include "test_utils.hpp"

using namespace semver;

TEST_CASE("validation") {
#ifdef SEMVER_CONSTEXPR_SUPPORT
  SECTION("constexpr valid") {
    constexpr std::string_view v1 = "0.0.1";
    static_assert(valid(v1));

    constexpr std::string_view v2 = "1.2.3-rc.4";
    static_assert(valid(v2));

    constexpr std::string_view v3 = "1.1.2-prerelease+meta";
    static_assert(valid(v3));
  }

  SECTION("constexpr invalid") {
    constexpr std::string_view v1 = "";
    static_assert(!valid(v1));

    constexpr std::string_view v2 = "1.01.*";
    static_assert(!valid(v2));

    constexpr std::string_view v3 = "1.1.2-prerelease_meta";
    static_assert(!valid(v3));
  }
#endif

  SECTION("runtime valid") {
    for (auto version: valid_versions) {
      REQUIRE(valid(version));
    }
  }

  SECTION("runtime invalid") {
    for (auto version: invalid_versions) {
      REQUIRE_FALSE(valid(version));
    }
  }
}

TEST_CASE("integral type constraint") {
  // version<T> requires integral T; non-integral types are caught at compile time
  // via static_assert. We verify integral types work for a range of widths.
  SECTION("various integral widths parse correctly") {
    semver::version<uint8_t>  v8;
    semver::version<uint16_t> v16;
    semver::version<uint32_t> v32;
    semver::version<uint64_t> v64;
    semver::version<int64_t>  vi64;
    REQUIRE(semver::parse("1.2.3", v8));
    REQUIRE(semver::parse("1.2.3", v16));
    REQUIRE(semver::parse("1.2.3", v32));
    REQUIRE(semver::parse("1.2.3", v64));
    REQUIRE(semver::parse("1.2.3", vi64));
    REQUIRE(v8.major() == 1);
    REQUIRE(v64.patch() == 3);
    // static_assert(false, "version<float> must not compile");  // uncomment to test
  }
}
