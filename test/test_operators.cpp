#include "test_utils.hpp"
#include <semver.hpp>
#include <array>

using namespace semver;

TEST_CASE("operators") {
  constexpr std::array<version, 56> versions = {{
    version{0, 0, 0, "alpha.0"},
    version{0, 0, 0, "alpha.1"},
    version{0, 0, 0, "beta.0"},
    version{0, 0, 0, "beta.1"},
    version{0, 0, 0, "rc.0"},
    version{0, 0, 0, "rc.1"},
    version{0, 0, 0},

    version{0, 0, 1, "alpha.0"},
    version{0, 0, 1, "alpha.1"},
    version{0, 0, 1, "beta.0"},
    version{0, 0, 1, "beta.1"},
    version{0, 0, 1, "rc.0"},
    version{0, 0, 1, "rc.1"},
    version{0, 0, 1},

    version{0, 1, 0, "alpha.0"},
    version{0, 1, 0, "alpha.1"},
    version{0, 1, 0, "beta.0"},
    version{0, 1, 0, "beta.1"},
    version{0, 1, 0, "rc.0"},
    version{0, 1, 0, "rc.1"},
    version{0, 1, 0},

    version{0, 1, 1, "alpha.0"},
    version{0, 1, 1, "alpha.1"},
    version{0, 1, 1, "beta.0"},
    version{0, 1, 1, "beta.1"},
    version{0, 1, 1, "rc.0"},
    version{0, 1, 1, "rc.1"},
    version{0, 1, 1},

    version{1, 0, 0, "alpha.0"},
    version{1, 0, 0, "alpha.1"},
    version{1, 0, 0, "beta.0"},
    version{1, 0, 0, "beta.1"},
    version{1, 0, 0, "rc.0"},
    version{1, 0, 0, "rc.1"},
    version{1, 0, 0},

    version{1, 0, 1, "alpha.0"},
    version{1, 0, 1, "alpha.1"},
    version{1, 0, 1, "beta.0"},
    version{1, 0, 1, "beta.1"},
    version{1, 0, 1, "rc.0"},
    version{1, 0, 1, "rc.1"},
    version{1, 0, 1},

    version{1, 1, 0, "alpha.0"},
    version{1, 1, 0, "alpha.1"},
    version{1, 1, 0, "beta.0"},
    version{1, 1, 0, "beta.1"},
    version{1, 1, 0, "rc.0"},
    version{1, 1, 0, "rc.1"},
    version{1, 1, 0},

    version{1, 1, 1, "alpha.0"},
    version{1, 1, 1, "alpha.1"},
    version{1, 1, 1, "beta.0"},
    version{1, 1, 1, "beta.1"},
    version{1, 1, 1, "rc.0"},
    version{1, 1, 1, "rc.1"},
    version{1, 1, 1},
    }};

  SECTION("operator =") {
    constexpr version v1{1, 2, 3, "rc.4"};
    constexpr version v2 = v1;
    STATIC_CHECK_OP_AND_REVERSE(v1, ==, v2);
    static_assert(compare(v1, v2) == 0);
    static_assert(compare(v2, v1) == 0);

    for (std::size_t i = 0; i < versions.size(); ++i) {
      version v = versions[i];
      CHECK_OP_AND_REVERSE(v, ==, versions[i]);
    }
  }

  SECTION("operator ==") {
    constexpr version v1{1, 2, 3, "rc.4"};
    constexpr version v2{1, 2, 3, "rc.4"};
    STATIC_CHECK_OP_AND_REVERSE(v1, ==, v2);

    for (std::size_t i = 0; i < versions.size(); ++i) {
      version v = versions[i];
      CHECK_OP_AND_REVERSE(v, ==, versions[i]);

      REQUIRE(compare(v, versions[i], comparators_option::include_prerelease) == 0);
      REQUIRE(compare(versions[i], v, comparators_option::include_prerelease) == 0);
      REQUIRE(compare(v, versions[i], comparators_option::exclude_prerelease) == 0);
      REQUIRE(compare(versions[i], v, comparators_option::exclude_prerelease) == 0);
    }
  }

  SECTION("operator !=") {
    constexpr version v1{1, 2, 3, "rc.4"};
    constexpr version v2{1, 2, 3};
    STATIC_CHECK_OP_AND_REVERSE(v1, !=, v2);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        CHECK_OP_AND_REVERSE(versions[i], !=, versions[i - j]);

        REQUIRE(compare(versions[i], versions[i - j], comparators_option::include_prerelease) != 0);
        REQUIRE(compare(versions[i - j], versions[i], comparators_option::include_prerelease) != 0);
        if ((i - j) / 7 == i / 7) {
          REQUIRE(compare(versions[i], versions[i - j], comparators_option::exclude_prerelease) == 0);
          REQUIRE(compare(versions[i - j], versions[i], comparators_option::exclude_prerelease) == 0);
        } else {
          REQUIRE(compare(versions[i], versions[i - j], comparators_option::exclude_prerelease) != 0);
          REQUIRE(compare(versions[i - j], versions[i], comparators_option::exclude_prerelease) != 0);
        }
      }
    }
  }

  SECTION("operator >") {
    constexpr version v1{1, 2, 3, "rc.4"};
    constexpr version v2{1, 2, 3};
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v2, >, v1);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        CHECK_OP_AND_REVERSE_FALSE(versions[i], >, versions[i - j]);

        REQUIRE(greater(versions[i], versions[i - j], comparators_option::include_prerelease));
        REQUIRE_FALSE(greater(versions[i - j], versions[i], comparators_option::include_prerelease));
        if ((i - j) / 7 == i / 7) {
          REQUIRE_FALSE(greater(versions[i], versions[i - j], comparators_option::exclude_prerelease));
          REQUIRE_FALSE(greater(versions[i - j], versions[i], comparators_option::exclude_prerelease));
        } else {
          REQUIRE(greater(versions[i], versions[i - j], comparators_option::exclude_prerelease));
          REQUIRE_FALSE(greater(versions[i - j], versions[i], comparators_option::exclude_prerelease));
        }
      }
    }
  }

  SECTION("operator >=") {
    constexpr version v1{1, 2, 3, "rc.4"};
    constexpr version v2{1, 2, 3};
    constexpr version v3{1, 2, 3};
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v2, >=, v1);
    STATIC_CHECK_OP_AND_REVERSE(v2, >=, v3);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        version v = versions[i];
        CHECK_OP_AND_REVERSE_FALSE(versions[i], >=, versions[i - j]);
        CHECK_OP_AND_REVERSE(v, >=, versions[i]);

        REQUIRE(greater_equal(versions[i], versions[i - j], comparators_option::include_prerelease));
        REQUIRE_FALSE(greater_equal(versions[i - j], versions[i], comparators_option::include_prerelease));
        REQUIRE(greater_equal(versions[i], v, comparators_option::include_prerelease));
        REQUIRE(greater_equal(v, versions[i], comparators_option::include_prerelease));
        if ((i - j) / 7 == i / 7) {
          REQUIRE(greater_equal(versions[i], versions[i - j], comparators_option::exclude_prerelease));
          REQUIRE(greater_equal(versions[i - j], versions[i], comparators_option::exclude_prerelease));
          REQUIRE(greater_equal(versions[i], v, comparators_option::exclude_prerelease));
          REQUIRE(greater_equal(v, versions[i], comparators_option::exclude_prerelease));
        } else {
          REQUIRE(greater_equal(versions[i], versions[i - j], comparators_option::exclude_prerelease));
          REQUIRE_FALSE(greater_equal(versions[i - j], versions[i], comparators_option::exclude_prerelease));
        }
      }
    }
  }

  SECTION("operator <") {
    constexpr version v1{1, 2, 3, "rc.4"};
    constexpr version v2{1, 2, 3};
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v1, <, v2);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        CHECK_OP_AND_REVERSE_FALSE(versions[i - j], <, versions[i]);

        REQUIRE(less(versions[i - j], versions[i], comparators_option::include_prerelease));
        REQUIRE_FALSE(less(versions[i], versions[i - j], comparators_option::include_prerelease));
        if ((i - j) / 7 == i / 7) {
          REQUIRE_FALSE(less(versions[i - j], versions[i], comparators_option::exclude_prerelease));
          REQUIRE_FALSE(less(versions[i], versions[i - j], comparators_option::exclude_prerelease));
        } else {
          REQUIRE(less(versions[i - j], versions[i], comparators_option::exclude_prerelease));
          REQUIRE_FALSE(less(versions[i], versions[i - j], comparators_option::exclude_prerelease));
        }
      }
    }
  }

  SECTION("operator <=") {
    constexpr version v1{1, 2, 3, "rc.4"};
    constexpr version v2{1, 2, 3};
    constexpr version v3{1, 2, 3};
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v1, <=, v2);
    STATIC_CHECK_OP_AND_REVERSE(v2, <=, v3);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        version v = versions[i - j];
        CHECK_OP_AND_REVERSE_FALSE(versions[i - j], <=, versions[i]);
        CHECK_OP_AND_REVERSE(v, <=, versions[i - j]);

        REQUIRE(less_equal(versions[i - j], versions[i], comparators_option::include_prerelease));
        REQUIRE_FALSE(less_equal(versions[i], versions[i - j], comparators_option::include_prerelease));
        REQUIRE(less_equal(v, versions[i - j], comparators_option::include_prerelease));
        REQUIRE(less_equal(versions[i - j], v, comparators_option::include_prerelease));
        if ((i - j) / 7 == i / 7) {
          REQUIRE(less_equal(versions[i - j], versions[i], comparators_option::exclude_prerelease));
          REQUIRE(less_equal(versions[i], versions[i - j], comparators_option::exclude_prerelease));
        } else {
          REQUIRE(less_equal(versions[i - j], versions[i], comparators_option::exclude_prerelease));
          REQUIRE_FALSE(less_equal(versions[i], versions[i - j], comparators_option::exclude_prerelease));
        }
      }
    }
  }

  SECTION("operator _version") {
    constexpr version v = "1.2.3-rc.4"_version;
    static_assert(v == version{1, 2, 3, "rc.4"});
  }

  SECTION("prerelease compare") {
    //    1.0.0-alpha < 1.0.0-alpha.1 < 1.0.0-alpha.beta < 1.0.0-beta < 1.0.0-beta.2 < 1.0.0-beta.11 < 1.0.0-rc.1 < 1.0.0.
    constexpr auto v1 = "1.0.0-alpha"_version;
    constexpr auto v2 = "1.0.0-alpha.1"_version;
    constexpr auto v3 = "1.0.0-alpha.beta"_version;
    constexpr auto v4 = "1.0.0-beta"_version;
    constexpr auto v5 = "1.0.0-beta.2"_version;
    constexpr auto v6 = "1.0.0-beta.11"_version;
    constexpr auto v7 = "1.0.0-rc.1"_version;
    constexpr auto v8 = "1.0.0"_version;

    STATIC_CHECK_OP_AND_REVERSE_FALSE(v1, <, v2);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v2, <, v3);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v3, <, v4);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v4, <, v5);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v5, <, v6);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v6, <, v7);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v7, <, v8);

    STATIC_CHECK_OP_AND_REVERSE_FALSE(v2, >, v1);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v3, >, v2);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v4, >, v3);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v5, >, v4);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v6, >, v5);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v7, >, v6);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v8, >, v7);

    constexpr auto v9 = "1.0.0-alpha.5"_version;
    constexpr auto v10 = "1.0.0-alpha.10"_version;
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v9, <, v10);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v10, >, v9);
  }
}
