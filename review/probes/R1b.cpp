// probe: R1b P-2 on newer libstdc++: SEMVER_HAS_CONSTEXPR_RANGES under _GLIBCXX_DEBUG, range parsing in a constant expression
// where: gcc-latest
// std: c++20 c++23
// expect: build-fail "__debug::vector"
// meaning: build-fail mentioning std::__debug::vector = libstdc++ still advertises __cpp_lib_constexpr_vector in debug mode
//   while the debug vector is not constexpr, so the header's flag is wrong there (P-2 persists, as on libstdc++ 13);
//   build-fail "SEMVER_HAS_CONSTEXPR_RANGES is 0" = the flag is already off (no defect);
//   build-ok = the debug vector is usable in constant expressions on this libstdc++ (P-2 limited to older libstdc++).
#define _GLIBCXX_DEBUG 1
#include <semver.hpp>

#if !SEMVER_HAS_CONSTEXPR_RANGES
#  error "SEMVER_HAS_CONSTEXPR_RANGES is 0"
#endif

static_assert([] {
  semver::range_set<> rs;
  return semver::parse(">=1.0.0 <2.0.0", rs) && rs.contains(semver::version<>{1, 5, 0});
}());

int main() { return 0; }
