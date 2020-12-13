#include "semver.hpp"

using namespace semver;

int main() {
    constexpr std::string_view r1 = ">=1.2.7 <1.3.0";
    static_assert(range::satisfies("1.2.7"_version, r1));
    static_assert(range::satisfies("1.2.8"_version, r1));
    static_assert(range::satisfies("1.2.99"_version, r1));
    static_assert(!range::satisfies("1.2.6"_version, r1));
    static_assert(!range::satisfies("1.3.0"_version, r1));
    static_assert(!range::satisfies("1.1.0"_version, r1));

    constexpr std::string_view r2 = "1.2.7 || >=1.2.9 <2.0.0";
    static_assert(range::satisfies(version{1, 2, 7}, r2));
    static_assert(range::satisfies({1, 2, 9}, r2));
    static_assert(!range::satisfies("1.2.8"_version, r2));
    static_assert(!range::satisfies("2.0.0"_version, r2));

    // By default, we exclude prerelease tag from comparison.
    constexpr std::string_view r3 = ">1.2.3-alpha.3";
    static_assert(range::satisfies("1.2.3-alpha.7"_version, r3));
    static_assert(!range::satisfies("3.4.5-alpha.9"_version, r3));

    // But we can suppress this behavior by passing semver::range::option::include_prerelease.
    // For details see: https://github.com/npm/node-semver#prerelease-tags
    static_assert(range::satisfies("3.4.5-alpha.9"_version, r3, range::satisfies_option::include_prerelease));

    return 0;
}
