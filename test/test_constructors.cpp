#include <semver.hpp>
#include <catch.hpp>

using namespace semver;

TEST_CASE("constructors") {
  SECTION("default") {
    constexpr version v0;
    static_assert(v0.get_major() == 0 &&
    v0.get_minor() == 1 &&
    v0.get_patch() == 0 &&
    v0.get_prerelease().empty() &&
    v0.get_build_metadata().empty());
  }

  SECTION("constructor") {
    constexpr version v1{1, 2, 3, "rc.0"};
    static_assert(v1.get_major() == 1 &&
    v1.get_minor() == 2 &&
    v1.get_patch() == 3 &&
    detail::compare_equal(v1.get_prerelease(), "rc.0"));

    constexpr version v2{1, 2, 3, "rc.4"};
    static_assert(v2.get_major() == 1 &&
    v2.get_minor() == 2 &&
    v2.get_patch() == 3 &&
    detail::compare_equal(v2.get_prerelease(), "rc.4"));

    constexpr version v3{1, 2, 3};
    static_assert(v3.get_major() == 1 &&
    v3.get_minor() == 2 &&
    v3.get_patch() == 3 &&
    v3.get_prerelease().empty());

    constexpr version v4{1, 2, 3, ""};
    static_assert(v4.get_major() == 1 &&
    v4.get_minor() == 2 &&
    v4.get_patch() == 3 &&
    detail::compare_equal(v4.get_prerelease(), ""));

    constexpr version v5{v4};
    static_assert(v5.get_major() == 1 &&
    v5.get_minor() == 2 &&
    v5.get_patch() == 3 &&
    v5.get_prerelease().empty());

    constexpr version v6{v5};
    static_assert(v6.get_major() == 1 &&
    v6.get_minor() == 2 &&
    v6.get_patch() == 3 &&
    v6.get_prerelease().empty());

    constexpr version v7{"1.2.3-alpha.4"};
    static_assert(v7.get_major() == 1 &&
    v7.get_minor() == 2 &&
    v7.get_patch() == 3 &&
    detail::compare_equal(v7.get_prerelease(), "alpha.4"));

    std::string s = "1.1.1";
    version v10{s};
    REQUIRE(v10.get_major() == 1);
    REQUIRE(v10.get_minor() == 1);
    REQUIRE(v10.get_patch() == 1);
    REQUIRE(v10.get_prerelease().empty());
  }
}