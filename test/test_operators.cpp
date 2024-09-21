#include "test_utils.hpp"
#include <semver.hpp>
#include <array>

using namespace semver;

TEST_CASE("operators") {
  constexpr std::array<std::string_view, 56> versions = {{
    std::string_view{"0.0.0-alpha.0"},
    std::string_view{"0.0.0-alpha.1"},
    std::string_view{"0.0.0-beta.0"},
    std::string_view{"0.0.0-beta.1"},
    std::string_view{"0.0.0-rc.0"},
    std::string_view{"0.0.0-rc.1"},
    std::string_view{"0.0.0"},

    std::string_view{"0.0.1-alpha.0"},
    std::string_view{"0.0.1-alpha.1"},
    std::string_view{"0.0.1-beta.0"},
    std::string_view{"0.0.1-beta.1"},
    std::string_view{"0.0.1-rc.0"},
    std::string_view{"0.0.1-rc.1"},
    std::string_view{"0.0.1"},

    std::string_view{"0.1.0-alpha.0"},
    std::string_view{"0.1.0-alpha.1"},
    std::string_view{"0.1.0-beta.0"},
    std::string_view{"0.1.0-beta.1"},
    std::string_view{"0.1.0-rc.0"},
    std::string_view{"0.1.0-rc.1"},
    std::string_view{"0.1.0"},

    std::string_view{"0.1.1-alpha.0"},
    std::string_view{"0.1.1-alpha.1"},
    std::string_view{"0.1.1-beta.0"},
    std::string_view{"0.1.1-beta.1"},
    std::string_view{"0.1.1-rc.0"},
    std::string_view{"0.1.1-rc.1"},
    std::string_view{"0.1.1"},

    std::string_view{"1.0.0-alpha.0"},
    std::string_view{"1.0.0-alpha.1"},
    std::string_view{"1.0.0-beta.0"},
    std::string_view{"1.0.0-beta.1"},
    std::string_view{"1.0.0-rc.0"},
    std::string_view{"1.0.0-rc.1"},
    std::string_view{"1.0.0"},

    std::string_view{"1.0.1-alpha.0"},
    std::string_view{"1.0.1-alpha.1"},
    std::string_view{"1.0.1-beta.0"},
    std::string_view{"1.0.1-beta.1"},
    std::string_view{"1.0.1-rc.0"},
    std::string_view{"1.0.1-rc.1"},
    std::string_view{"1.0.1"},

    std::string_view{"1.1.0-alpha.0"},
    std::string_view{"1.1.0-alpha.1"},
    std::string_view{"1.1.0-beta.0"},
    std::string_view{"1.1.0-beta.1"},
    std::string_view{"1.1.0-rc.0"},
    std::string_view{"1.1.0-rc.1"},
    std::string_view{"1.1.0"},

    std::string_view{"1.1.1-alpha.0"},
    std::string_view{"1.1.1-alpha.1"},
    std::string_view{"1.1.1-beta.0"},
    std::string_view{"1.1.1-beta.1"},
    std::string_view{"1.1.1-rc.0"},
    std::string_view{"1.1.1-rc.1"},
    std::string_view{"1.1.1"},
  }};

  SECTION("operator ==") {
    constexpr std::string_view v1 = "1.2.3-rc.4";
    constexpr std::string_view v2 = "1.2.3-rc.4";
    STATIC_CHECK_OP_AND_REVERSE(v1, equal, v2);

    for (auto version : versions) {
      std::string_view v = version;
      CHECK_OP_AND_REVERSE(v, equal, version);
    }
  }

  SECTION("operator !=") {
    constexpr std::string_view v1{"1.2.3-rc.4"};
    constexpr std::string_view v2{"1.2.3"};
    STATIC_CHECK_OP_AND_REVERSE(v1, not_equal, v2);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        CHECK_OP_AND_REVERSE(versions[i], not_equal, versions[i - j]);
      }
    }
  }

  SECTION("operator >") {
    constexpr std::string_view v1{"1.2.3-rc.4"};
    constexpr std::string_view v2{"1.2.3"};
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v2, greater, v1);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        CHECK_OP_AND_REVERSE_FALSE(versions[i], greater, versions[i - j]);
      }
    }
  }

  SECTION("operator >=") {
    constexpr std::string_view v1{"1.2.3-rc.4"};
    constexpr std::string_view v2{"1.2.3"};
    constexpr std::string_view v3{"1.2.3"};
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v2, greater_equal, v1);
    STATIC_CHECK_OP_AND_REVERSE(v2, greater_equal, v3);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        std::string_view v = versions[i];
        CHECK_OP_AND_REVERSE_FALSE(versions[i], greater_equal, versions[i - j]);
        CHECK_OP_AND_REVERSE(v, greater_equal, versions[i]);
      }
    }
  }

  SECTION("operator <") {
    constexpr std::string_view v1{"1.2.3-rc.4"};
    constexpr std::string_view v2{"1.2.3"};
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v1, less, v2);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        CHECK_OP_AND_REVERSE_FALSE(versions[i - j], less, versions[i]);
      }
    }
  }

  SECTION("operator <=") {
    constexpr std::string_view v1{"1.2.3-rc.4"};
    constexpr std::string_view v2{"1.2.3"};
    constexpr std::string_view v3{"1.2.3"};
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v1, less_equal, v2);
    STATIC_CHECK_OP_AND_REVERSE(v2, less_equal, v3);

    for (std::size_t i = 1; i < versions.size(); ++i) {
      for (std::size_t j = 1; j < i; ++j) {
        std::string_view v = versions[i - j];
        CHECK_OP_AND_REVERSE_FALSE(versions[i - j], less_equal, versions[i]);
        CHECK_OP_AND_REVERSE(v, less_equal, versions[i - j]);
      }
    }
  }

  SECTION("prerelease compare") {
    //    1.0.0-alpha < 1.0.0-alpha.1 < 1.0.0-alpha.beta < 1.0.0-beta < 1.0.0-beta.2 < 1.0.0-beta.11 < 1.0.0-rc.1 < 1.0.0.
    constexpr std::string_view v1 = "1.0.0-alpha";
    constexpr std::string_view v2 = "1.0.0-alpha.1";
    constexpr std::string_view v3 = "1.0.0-alpha.beta";
    constexpr std::string_view v4 = "1.0.0-beta";
    constexpr std::string_view v5 = "1.0.0-beta.2";
    constexpr std::string_view v6 = "1.0.0-beta.11";
    constexpr std::string_view v7 = "1.0.0-rc.1";
    constexpr std::string_view v8 = "1.0.0";

    STATIC_CHECK_OP_AND_REVERSE_FALSE(v1, less, v2);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v2, less, v3);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v3, less, v4);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v4, less, v5);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v5, less, v6);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v6, less, v7);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v7, less, v8);

    STATIC_CHECK_OP_AND_REVERSE_FALSE(v2, greater, v1);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v3, greater, v2);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v4, greater, v3);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v5, greater, v4);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v6, greater, v5);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v7, greater, v6);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v8, greater, v7);

    constexpr std::string_view v9 = "1.0.0-alpha.5";
    constexpr std::string_view v10 = "1.0.0-alpha.10";
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v9, less, v10);
    STATIC_CHECK_OP_AND_REVERSE_FALSE(v10, greater, v9);
  }
}
