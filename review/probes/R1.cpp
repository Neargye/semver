// probe: R1 first constant-evaluated range parse at namespace scope on Clang (N1: parse_number<uint64_t> defined after its use)
// where: clang-libcxx macos-appleclang
// std: c++20 c++23
// expect: build-fail "undefined function"
// meaning: build-fail "undefined function 'parse_number" = N1 also reproduces with this Clang/libc++ (Clang does not
//   instantiate the member template in time, so SEMVER_HAS_CONSTEXPR_RANGES == 1 but the first constexpr range parse fails);
//   build-fail "SEMVER_HAS_CONSTEXPR_RANGES is 0" = the flag is off on this toolchain (probe not applicable);
//   build-ok = N1 does not reproduce on this compiler version.
#include <semver.hpp>

#if !SEMVER_HAS_CONSTEXPR_RANGES
#  error "SEMVER_HAS_CONSTEXPR_RANGES is 0"
#endif

static_assert(semver::try_parse_range("1").has_value());
static_assert(semver::satisfies(semver::version<>{1, 2, 3}, "^1.2"));

int main() { return 0; }
