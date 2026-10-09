// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2026 Daniil Goncharov <neargye@gmail.com>.

#include <doctest.h>
#include <cstdint>
#include <semver.hpp>

using namespace semver;

// Keep this test first in the file: range parsing must work as the first constant evaluation in a translation unit.
#if SEMVER_HAS_CONSTEXPR_RANGES
TEST_CASE("constexpr range operations as first use") {
  static_assert(satisfies(version<>{1, 2, 3}, ">=1.0.0 <2.0.0"));
  static_assert(try_parse_range<std::uint64_t>("1.x").has_value());
  static_assert(min_version(*try_parse_range(">1.2.3-alpha")) == version<>{1, 2, 3, "alpha.0"});
  static_assert(intersects(*try_parse_range("^1"), *try_parse_range(">=1.5.0-rc.1 <1.6")));
}
#endif

#if SEMVER_HAS_CONSTEXPR_OPTIONAL
TEST_CASE("constexpr inc of numeric prerelease") {
  static_assert(inc(version<>{1, 2, 3, "alpha.9"}, version_change::prerelease) == version<>{1, 2, 3, "alpha.10"});
}
#endif

TEST_CASE("range operations used in constexpr tests") {
  CHECK(satisfies(version<>{1, 2, 3}, ">=1.0.0 <2.0.0"));
  CHECK(try_parse_range<std::uint64_t>("1.x").has_value());
  CHECK(min_version(*try_parse_range(">1.2.3-alpha")) == version<>{1, 2, 3, "alpha.0"});
  CHECK(intersects(*try_parse_range("^1"), *try_parse_range(">=1.5.0-rc.1 <1.6")));
  CHECK(inc(version<>{1, 2, 3, "alpha.9"}, version_change::prerelease) == version<>{1, 2, 3, "alpha.10"});
}
