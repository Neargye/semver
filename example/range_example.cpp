#include "semver.hpp"

using namespace semver;

int main() {
    constexpr auto r1 = range(">=1.2.7 <1.3.0");
    static_assert(r1.satisfies("1.2.7"_version));
    static_assert(r1.satisfies("1.2.8"_version));
    static_assert(r1.satisfies("1.2.99"_version));
    static_assert(!r1.satisfies("1.2.6"_version));
    static_assert(!r1.satisfies("1.3.0"_version));
    static_assert(!r1.satisfies("1.1.0"_version));

    constexpr auto r2 = range("1.2.7 || >=1.2.9 <2.0.0");
    static_assert(r2.satisfies(version{1, 2, 7}));
    static_assert(r2.satisfies({1, 2, 9}));
    static_assert(!r2.satisfies("1.2.8"_version));
    static_assert(!r2.satisfies("2.0.0"_version));

    // By default, we exclude prerelease tag from comparison.
    constexpr auto r3 = range(">1.2.3-alpha.3");
    static_assert(r3.satisfies("1.2.3-alpha.7"_version));
    static_assert(!r3.satisfies("3.4.5-alpha.9"_version));

    // But we can suppress this behavior by passing semver::range::option::include_prerelease.
    // For details see: https://github.com/npm/node-semver#prerelease-tags
    static_assert(r3.satisfies("3.4.5-alpha.9"_version, range::option::include_prerelease));

    return 0;
} 