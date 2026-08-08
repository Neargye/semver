// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2026 Daniil Goncharov <neargye@gmail.com>.

// Verify that semver.hpp is safe to include after the Windows min/max macros.
#include <windows.h>

#ifndef min
#  define min(a, b) unexpected_min_macro
#endif

#ifndef max
#  define max(a, b) unexpected_max_macro
#endif

#include <semver.hpp>

#undef min
#undef max

bool semver_windows_minmax_macro_compile_test() {
  return semver::try_parse("1.2.3").has_value();
}
