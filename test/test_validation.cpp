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
