// probe: H12 character types accepted as component types where wchar_t is unsigned (Windows)
// where: msvc-x64 msvc-x86 clangcl
// std: c++17 c++20
// expect: run-ok
// meaning: run-ok = version<wchar_t> and version<char16_t, char32_t> compile and char sources convert to code points
//   ("49.50.51"), i.e. H12 also holds on Windows; build-fail = wchar_t is rejected there; run-fail = different values.
#include <semver.hpp>
#include <cstdio>
#include <string>

int main() {
  const semver::version<wchar_t> w{1u, 2u, 3u};
  const semver::version<char16_t, char32_t> c{1u, 2u, 3u};
  const semver::version<> a{'1', '2', '3'};
  std::printf("%s %s %s\n", w.to_string().c_str(), c.to_string().c_str(), a.to_string().c_str());
  return w.to_string() == "1.2.3" && c.to_string() == "1.2.3" && a.to_string() == "49.50.51" ? 0 : 1;
}
