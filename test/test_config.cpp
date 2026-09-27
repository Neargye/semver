// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2026 Daniil Goncharov <neargye@gmail.com>.

#if defined(SEMVER_TEST_LEGACY_OPTIONAL)
#include <optional>
#include <string>
#include <vector>
#if defined(__has_include) && __has_include(<version>)
#include <version>
#endif
#undef __cpp_lib_optional
#define __cpp_lib_optional 201606L
#endif

#include <string>
#include <system_error>
#include <semver.hpp>

static_assert(semver::max_input_length == 64, "SEMVER_CONFIG_FILE must override defaults before semver.hpp is configured");

#if defined(SEMVER_TEST_LEGACY_OPTIONAL)
static_assert(SEMVER_HAS_CONSTEXPR_OPTIONAL == 0);
static_assert(SEMVER_HAS_CONSTEXPR_RANGES == 0);
static_assert(SEMVER_HAS_CONSTEXPR == 0);
#if defined(__cpp_lib_constexpr_string) && __cpp_lib_constexpr_string >= 201907L && \
    !(defined(__clang__) && defined(__GLIBCXX__) && (!defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE < 13))
static_assert(SEMVER_HAS_CONSTEXPR_CORE == 1);
static_assert([] {
  semver::version<> v;
  return semver::parse("1.2.3-alpha+build", v) && v.to_string() == "1.2.3-alpha+build";
}());
#endif
#endif

int main() {
  std::string at_limit = "1.0.0+";
  at_limit.append(SEMVER_MAX_INPUT_LENGTH - at_limit.size(), 'a');

  semver::version<> parsed;
  if (!semver::parse(at_limit, parsed))
    return 1;

  auto over_limit = at_limit;
  over_limit.push_back('a');
  const auto result = semver::parse(over_limit, parsed);
  if (result || result.ec != std::errc::value_too_large || result.ptr != over_limit.data())
    return 2;

  if (semver::coerce(over_limit).has_value())
    return 3;
  if (semver::clean(over_limit).has_value())
    return 4;

  const auto v = semver::try_parse("1.2.3");
  const auto range = semver::try_parse_range("^1.2");
  if (!v || !range || !range->contains(*v))
    return 5;

  return 0;
}
