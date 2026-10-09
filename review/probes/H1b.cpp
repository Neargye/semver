// probe: H1b "..."_semver with a short (SSO) prerelease used at run time on libc++ / MSVC STL
// where: clang-libcxx macos-appleclang clangcl
// std: c++20 c++23
// expect: run-ok
// meaning: run-ok = short literals work on this standard library (H1 limited to libstdc++ and long tags);
//   build-fail "not a constant expression" = the literal is unusable outside constant evaluation here as well (H1 wider);
//   build-fail "literal disabled" = the guard keeps the literal off on this toolchain; run-fail = wrong value.
#include <semver.hpp>

#if !SEMVER_HAS_CONSTEVAL_LITERAL
#  error "literal disabled in this configuration"
#endif

using namespace semver::literals;

int main() {
  const auto v = "1.2.3-rc.1"_semver;
  return v.major() == 1 && v.patch() == 3 && v.prerelease_tag() == "rc.1" ? 0 : 1;
}
