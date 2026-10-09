// probe: R1c every SEMVER_HAS_CONSTEXPR_* flag that is 1 is usable in constant expressions at namespace scope (first use)
// where: gcc-latest gcc-m32 msvc-x64 msvc-x86
// std: c++20 c++23
// expect: run-ok
// meaning: run-ok = the feature flags of this toolchain match what it can constant-evaluate (R1 guards correct here);
//   build-fail = a flag is 1 but the corresponding API cannot be constant-evaluated on this compiler/library (wrong guard,
//   or the Clang first-use problem N1 on a non-Clang compiler); the output line lists the flag values.
#include <semver.hpp>
#include <cstdio>
#include <string_view>

#if SEMVER_HAS_CONSTEXPR_CORE
static_assert(semver::valid("1.2.3-alpha.1+build.5"));
static_assert(semver::version<>{1, 2, 3, "alpha"} < semver::version<>{1, 2, 3});
static_assert([] {
  semver::version<> v;
  return semver::parse("1.2.3-rc.1", v) && v.patch() == 3 && v.prerelease_tag() == "rc.1";
}());
static_assert([] {
  char buf[32]{};
  const auto r = semver::to_chars(buf, buf + sizeof buf, semver::version<>{1, 2, 3, "a"});
  return r && std::string_view(buf, static_cast<std::size_t>(r.ptr - buf)) == "1.2.3-a";
}());
#endif

#if SEMVER_HAS_CONSTEXPR_OPTIONAL
static_assert(semver::try_parse("1.2.3")->minor() == 2);
static_assert(semver::coerce("v1.2")->minor() == 2);
static_assert(semver::clean(" =v1.2.3 ").has_value());
static_assert(semver::inc(semver::version<>{1, 2, 3}, semver::version_change::minor)->minor() == 3);
#endif

#if SEMVER_HAS_CONSTEXPR_RANGES
static_assert(semver::satisfies(semver::version<>{1, 2, 3}, "^1.2"));
static_assert(semver::min_version(*semver::try_parse_range(">=1.2.3-0 <2"))->prerelease_tag() == "0");
static_assert(semver::intersects(*semver::try_parse_range("^1"), *semver::try_parse_range(">=1.5 <3")));
#endif

int main() {
  std::printf("CORE=%d OPTIONAL=%d RANGES=%d CONSTEVAL_LITERAL=%d\n", SEMVER_HAS_CONSTEXPR_CORE, SEMVER_HAS_CONSTEXPR_OPTIONAL,
              SEMVER_HAS_CONSTEXPR_RANGES, SEMVER_HAS_CONSTEVAL_LITERAL);
  return 0;
}
