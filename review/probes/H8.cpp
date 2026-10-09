// probe: H8 ClangCL guard paths (MSVC STL + Clang): feature flags, the literal and the run-time API
// where: clangcl
// std: c++20 c++23
// expect: run-ok
// meaning: H8 predicts CORE=1 OPTIONAL=0 RANGES=0 CONSTEVAL_LITERAL=1 under clang-cl; the line printed shows the actual values.
//   run-ok = every flag that is 1 is usable (constant evaluation below) and the run-time API works;
//   build-fail = a guard enables a path clang-cl cannot compile (H8 defect; if the note says "undefined function" and RANGES=1,
//   it is the Clang first-use problem N1 instead); run-fail = a run-time check failed ("FAIL <n>").
#include <semver.hpp>
#include <cstdio>
#include <functional>
#include <sstream>
#include <string>

#if SEMVER_HAS_CONSTEXPR_CORE
static_assert(semver::valid("1.2.3-alpha.1+build.5"));
static_assert([] {
  semver::version<> v;
  return semver::parse("1.2.3-rc.1", v) && v.prerelease_tag() == "rc.1";
}());
#endif
#if SEMVER_HAS_CONSTEXPR_OPTIONAL
static_assert(semver::try_parse("1.2.3")->minor() == 2);
#endif
#if SEMVER_HAS_CONSTEXPR_RANGES
static_assert([] { return semver::satisfies(semver::version<>{1, 2, 3}, "^1.2"); }());
#endif
#if SEMVER_HAS_CONSTEVAL_LITERAL
using namespace semver::literals;
static_assert([] { return "1.2.3-rc.1"_semver.patch() == 3; }());
#endif

static int failures = 0;
static void check(bool ok, int n) {
  if (!ok) {
    std::printf("FAIL %d\n", n);
    ++failures;
  }
}

int main() {
  std::printf("CORE=%d OPTIONAL=%d RANGES=%d CONSTEVAL_LITERAL=%d\n", SEMVER_HAS_CONSTEXPR_CORE, SEMVER_HAS_CONSTEXPR_OPTIONAL,
              SEMVER_HAS_CONSTEXPR_RANGES, SEMVER_HAS_CONSTEVAL_LITERAL);
  semver::version<> v;
  check(static_cast<bool>(semver::parse("1.2.3-rc.1+b", v)) && v.to_string() == "1.2.3-rc.1+b", 1);
  check(semver::try_parse("1.2.3").has_value() && semver::coerce("v1.2").has_value() && semver::clean(" v1.2.3 ").has_value(), 2);
  check(semver::satisfies(v, ">=1.2.3-0 <2") && !semver::satisfies(v, "^1.2.3"), 3);
  const auto rs = semver::try_parse_range(">=1.0.0 <2.0.0 || ^3");
  check(rs && semver::min_version(*rs) == semver::version<>{1, 0, 0} && semver::intersects(*rs, *semver::try_parse_range("2.x || 3.1")), 4);
  check(semver::inc(v, semver::version_change::prerelease) == semver::version<>{1, 2, 3, "rc.2"}, 5);
  std::ostringstream os;
  os << v;
  check(os.str() == "1.2.3-rc.1+b" && std::hash<semver::version<>>{}(v) == std::hash<semver::version<>>{}(semver::version<>{1, 2, 3, "rc.1"}), 6);
  return failures == 0 ? 0 : 1;
}
