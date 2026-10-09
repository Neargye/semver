// probe: H6 operator<=> guarded by the compiler macro only: Clang (C++20) with a libstdc++ that has no <compare> (GCC 9)
// where: clang-oldlibstdcxx
// std: c++20
// expect: build-fail "compare"
// meaning: build-fail "'compare' file not found" = H6 confirmed (header includes <compare> because the compiler defines
//   __cpp_impl_three_way_comparison although the library lacks it); build-ok = refuted.
//   Job clang-oldlibstdcxx (not in the standard list): ubuntu-22.04, apt g++-9 clang-14, run.py --job clang-oldlibstdcxx
//   --cxx clang++-14 --flags "-nostdinc++ -isystem /usr/include/c++/9 -isystem /usr/include/x86_64-linux-gnu/c++/9".
#include <semver.hpp>

int main() {
  const semver::version<> a{1, 2, 3};
  const semver::version<> b{1, 2, 4};
  return a < b ? 0 : 1;
}
