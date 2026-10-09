// probe: H1 "..."_semver used outside a constant evaluation with GCC >= 14 + libstdc++ >= 14 (literal enabled by the guard)
// where: gcc-latest
// std: c++20 c++23
// expect: build-fail "not a constant expression"
// meaning: build-fail "not a constant expression" = H1 confirmed on this GCC/libstdc++ (the literal returns std::string members
//   that point into the result object, so the immediate invocation is never a constant expression);
//   build-fail "literal disabled" = the guard keeps the literal off on this GCC (H1 does not apply);
//   build-ok/run-ok = the literal is usable at run time on this GCC (H1 refuted here).
#include <semver.hpp>

#if !SEMVER_HAS_CONSTEVAL_LITERAL
#  error "literal disabled in this configuration"
#endif

using namespace semver::literals;

int main() {
  const auto v = "1.2.3"_semver; // ordinary run-time use, short tag
  return v.major() == 1 && v.minor() == 2 && v.patch() == 3 ? 0 : 1;
}
