// probe: H1c "..."_semver with a prerelease longer than any SSO buffer used at run time on libc++ / MSVC STL
// where: clang-libcxx macos-appleclang clangcl
// std: c++20 c++23
// expect: build-fail "not a constant expression"
// meaning: build-fail "not a constant expression" = H1 confirmed on this library for tags longer than the SSO capacity
//   (the result would hold a pointer to heap memory allocated during constant evaluation);
//   build-fail "literal disabled" = the guard keeps the literal off here; run-ok = long literals work (H1 refuted here).
#include <semver.hpp>

#if !SEMVER_HAS_CONSTEVAL_LITERAL
#  error "literal disabled in this configuration"
#endif

using namespace semver::literals;

int main() {
  const auto v = "1.2.3-averyveryverylong.prerelease.tag.0123456789"_semver;
  return v.major() == 1 && v.is_prerelease() ? 0 : 1;
}
