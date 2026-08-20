// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2026 Daniil Goncharov <neargye@gmail.com>.

#include <string>
#include <system_error>
#include <semver.hpp>

static_assert(semver::max_input_length == 64, "SEMVER_CONFIG_FILE must override defaults before semver.hpp is configured");

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

  return 0;
}
