# Review report — Neargye/semver

Base `574883a9aa2182c9512f9e140979a7b812406ca0` ("clean-up"), review date 2026-10-09, review branch `claude/inspiring-albattani-ynu35i`.
Toolchains in the review container: g++-13.3 (libstdc++ 13), clang++-18.1 (libstdc++ 13; no sanitizer runtimes, no libc++, no -m32), CMake 3.28.3, Ninja 1.11.1.

```
git log --oneline 574883a..
3735575 [proposal] semver: make the _semver literal usable outside constant expressions
2655b75 semver: disable constexpr ranges with libstdc++ 13 debug mode
86b21ba semver: fix constant evaluation of range operations on clang
```

The owner takes the first two commits (fixes) and decides on the `[proposal]` commit. Everything else on this branch (`review-report.md`, `review/`, `.github/workflows/review.yml`) is review-only and not for merging.

Style kept: LF everywhere, UTF-8 BOM in `test/CMakeLists.txt` kept, 2-space indentation, one-line `//` comments, doctest `TEST_CASE`/`CHECK`, 3-line license header in the new test file. No existing workflow, README or doc file changed.

## Fixes

### 86b21ba — semver: fix constant evaluation of range operations on clang (N1)

- **Problem.** With Clang and `SEMVER_HAS_CONSTEXPR_RANGES == 1`, a range operation that is the first constant evaluation in a translation unit does not compile. `version_parser::parse_partial` (non-template) calls the member template `parse_number<std::uint64_t>` that is defined later in the class, so Clang defers its instantiation and the constant evaluation sees an undefined function. A second, toolchain-level part: `visit_range_candidates` (used by `min_version` and `intersects`) and `inc(prerelease)` built `std::string{string_view}`, which Clang 18 cannot constant-evaluate with libstdc++ 13 (`undefined function '_M_construct<const char *>'`). The test suite hid both: test_validation.cpp:63/77-78 instantiate `parse_number<uint64_t>` before the constexpr block, and its constexpr `min_version`/`intersects` check returns before the prerelease candidates.
- **Evidence.** 22 public operations, each the only constant evaluation in its TU: base header, clang++-18 c++20 → 9 fail (range parse, `satisfies`, `contains`, `min_version`, `intersects`, `max/min_satisfying`, `inc(prerelease)`), e.g. `semver.hpp:684:18: note: undefined function 'parse_number<unsigned long>' cannot be used in a constant expression`; with only the reorder, `min_version`/`intersects`/`inc` still fail with `_M_construct<const char *>`. After the fix: 22/22 on clang++-18 c++20/23/2c and g++-13 c++20/23.
- **Change.** `parse_number` moved to the top of `version_parser` (still private) with a one-line comment; two `std::string{...prerelease_tag()}` became `std::string x; x.assign(...)` (the form the `version` constructor already uses).
- **Test.** New `test/test_constexpr.cpp` in the main test sources (all standards): the first TEST_CASE holds `satisfies`, `try_parse_range<uint64_t>`, `min_version(">1.2.3-alpha") == 1.2.3-alpha.0`, `intersects` as `static_assert`s before any other semver code in the TU; `inc(alpha.9) == alpha.10` under `SEMVER_HAS_CONSTEXPR_OPTIONAL`; the same checks at run time. Fails on old: clang++-18 c++20/23/2c, 5 static assertions (`test_constexpr.cpp:14-17,23`). Passes on new: clang++-18 c++17/20/23/2c, g++-13 c++17/20/23.
- **Limitations.** GCC 13 is not affected, so the test can only fail on Clang (existing CI: Clang 17–22, AppleClang). The `assign()` workaround was verified only for Clang 18 + libstdc++ 13.

### 2655b75 — semver: disable constexpr ranges with libstdc++ 13 debug mode (P-2)

- **Problem.** libstdc++ 13 defines `__cpp_lib_constexpr_vector` under `_GLIBCXX_DEBUG`, but `std::__debug::vector` cannot be used in constant expressions; `SEMVER_HAS_CONSTEXPR_RANGES` was 1, so user code guarded by the flag and the project's own `semver-cpp20.t`/`semver-cpp23.t` failed to compile with `-D_GLIBCXX_DEBUG`.
- **Evidence.** g++-13 -std=c++20 -D_GLIBCXX_DEBUG: `error: variable 'v' of non-literal type 'std::__debug::vector<int>' in 'constexpr' function`; base header reports `RANGES=1`, new header `RANGES=0` (g++-13 and clang++-18); without `_GLIBCXX_DEBUG` still `RANGES=1`. Step 2: the CMake build with `-D_GLIBCXX_ASSERTIONS -D_GLIBCXX_DEBUG` failed (3/5 targets).
- **Change.** One more condition in the `SEMVER_HAS_CONSTEXPR_RANGES` guard: `!(defined(_GLIBCXX_DEBUG) && (!defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE < 14))`, with a comment. Run-time behaviour unchanged.
- **Test.** New CTest target `semver-glibcxx-debug-cpp20.t` (GCC/Clang frontends, C++20) builds `test.cpp` + `test_constexpr.cpp` with `_GLIBCXX_DEBUG`. Fails on old (header of 86b21ba): g++-13 c++20/23 `test_constexpr.cpp:14:26: error: non-constant condition for static assertion`, clang++-18 c++20/23/2c fail. Passes on new; whole suite with `-D_GLIBCXX_ASSERTIONS -D_GLIBCXX_DEBUG` now 6/6 on both compilers (was 3/5).
- **Limitations.** The `< 14` bound assumes the debug vector became constexpr in libstdc++ 14 (PR 109536); only libstdc++ 13 is in the container. Existing CI (GCC 14/15, Clang 17–22 on ubuntu-24.04) builds the new target; review-CI probe `R1b.cpp` on `gcc-latest` checks it directly. If they fail, the version bound must be dropped.

## Proposals

### 3735575 — [proposal] semver: make the _semver literal usable outside constant expressions (H1, P-1)

- **What changes.** `operator""_semver` is no longer `consteval version<>(const char*, std::size_t)` but a string literal operator template `template <detail::literal_string S> constexpr version<> operator""_semver()` (new `detail::literal_string<N>` with a deduction guide). The input is checked by `static_assert(valid(S.view()), "invalid semver literal")`, so an invalid literal is still a compile-time error at every use; the result is an ordinary `version<>`. The `SEMVER_HAS_CONSTEVAL_LITERAL` guard additionally requires class-type template parameters (`__cpp_nontype_template_args >= 201911L`, or Clang, which supports them but reports 201411L in clang++-18); the other conditions (GCC < 14, native MSVC) and the macro name are unchanged.
- **Why.** The consteval literal returns `std::string` members; an immediate invocation must be a constant expression, and a libstdc++ string always points into itself, so every use outside an enclosing constant evaluation was rejected, including the documented `constexpr auto v = "1.2.3-alpha"_semver;` (doc/reference.md:351) and plain run-time use. H1 probe, clang++-18 base: cases 1–4 fail on c++20/23/2c (`call to consteval function 'semver::literals::operator""_semver' is not a constant expression`); after: cases 1, 3, 4, 5, 6 compile; `"1.2.x"_semver` → `static assertion failed ... invalid semver literal`.
- **Test.** test_validation.cpp "_semver literal outside constant expressions": temporary `.major()`, run-time variable with a tag longer than any SSO buffer, and (libstdc++ only) the namespace-scope `constexpr` doc example. Base header: test_validation.cpp does not compile on clang++-18 c++20/23/2c; new: passes.
- **Risks.** (1) An explicit call `operator""_semver("1.2.3", 5)` no longer compiles. (2) The diagnostic for an invalid literal changes. (3) One template instantiation per distinct literal. (4) A `constexpr version<>` *variable* with a non-empty tag still cannot exist with libc++ (no short-string buffer during constant evaluation) and, at block scope, with libstdc++ (D21) — a `std::string` property, independent of the literal form; the namespace-scope check in the test is therefore limited to libstdc++ (added during acceptance). (5) GCC 14/15, AppleClang and clang-cl run the new test only in CI.
- **To reject.** `git revert 3735575`; the two fixes do not depend on it. Alternatives: keep consteval and document "usable only inside constant expressions" (fix the reference.md example), or disable the literal on libstdc++.

## Found but not fixed

| item | reason | decision |
|---|---|---|
| H2 `-fno-exceptions` unsupported (header does not compile; GCC fails on inclusion via `library_version`) | design: exceptions are the documented error channel; support would be a new configuration feature | D9, Q1 |
| H9 `inc` with an explicit tag can go backwards/stay equal (`1.0.0-alpha.1` + `alpha` → `1.0.0-alpha`) | documented behaviour; changing it is API | D10, Q2 |
| H10 `diff(1.0.0-alpha, 1.0.0)` returns `prerelease`, contradicting doc/reference.md:206 | code or doc must change; API, low impact | D11, Q3 |
| H11 input cap applies to the whole buffer for `from_chars`/`coerce`/`clean` | pinned by tests (test_parse.cpp:693-705, test_config.cpp:52), D3 | D12, Q4 |
| H12 character types accepted as components (`version<>{'1','2','3'}` → `49.50.51`) | low impact, API | D13 |
| H13 quadratic `min_version`/`intersects`; constexpr budget exceeded at max input size | low impact (4 ms at run time with the default limit) | D14 |
| H15/H16 `clean`/`coerce` trim only spaces; `coerce` drops a prerelease before trailing text | documented / partly pinned, API, low | D15, Q5 |
| H17 `from_string` message holds the whole input | low, message format | D16 |
| H19 raw `SEMVER_MAX_INPUT_LENGTH` in comparisons | no warning-neutral fix (`max_input_length` breaks clang `-Wtype-limits -Werror` with `SIZE_MAX`) | D17, Q6 |
| H3 header-wide `-Wstringop-overflow` pragma | still needed (GCC 13 `-O3` false positive, ASan clean); formal only | D18 |
| H18 mixing standards/NDEBUG across TUs | formal ODR issue, no observable effect | D19 |
| Clang `-Weverything` style warnings | style only | D20 |
| Literal stays disabled on GCC 13 / native MSVC | enabling adds API; owner decision after the proposal | D22, Q7 |
| H5–H8 (old compilers, old libstdc++ `<compare>`, pre-LWG 3636 `<format>`, clang-cl) | not decidable in the container; review CI | D23, Q8 |

## Questions for the owner

- **Q1.** Should `-fno-exceptions` be supported (e.g. a `SEMVER_THROW` hook or `std::abort` fallback), or should the docs state that the header requires exceptions?
- **Q2.** `inc(1.0.0-alpha.1, prerelease, "alpha")` → `1.0.0-alpha` (lower than the input). Keep, return `nullopt` when the result is not greater, or continue the series like npm (`alpha.2`)?
- **Q3.** `diff(1.0.0-alpha, 1.0.0)` → `prerelease`. Change the code (npm: `major`/`minor`/`patch`) or the sentence in doc/reference.md:206?
- **Q4.** Should the input cap apply to the scanned prefix instead of the whole buffer for `from_chars`/`coerce`? Current behaviour is pinned by tests.
- **Q5.** Should `clean`/`coerce` trim all ASCII whitespace like range parsing, and should `coerce` keep a valid prerelease followed by trailing text?
- **Q6.** `SEMVER_MAX_INPUT_LENGTH`: document that it must be a parenthesized non-negative integer constant expression, or switch to `max_input_length` and accept a Clang warning for `SIZE_MAX`?
- **Q7.** Accept the literal proposal (3735575)? If yes: enable it on GCC 13 and native MSVC too, keep or rename `SEMVER_HAS_CONSTEVAL_LITERAL`, and adjust the reference.md example (a `constexpr` variable works only with libstdc++ at namespace scope)?
- **Q8.** README claims Clang ≥ 6, Apple Clang ≥ 10, GCC ≥ 7, VS ≥ 2019; CI covers GCC ≥ 13, Clang ≥ 17, VS 2022+. Test the older ones or lower the claim?

## Verification

On HEAD of the fix/proposal commits (step 3 matrix, re-run by the orchestrator after the acceptance change):

| configuration | result |
|---|---|
| T1 g++-13 Debug (CMake) | 6/6; doctest cpp17 92, cpp20 103, cpp23 103 test cases; examples rc=0 |
| T1 clang++-18 Debug (CMake) | 6/6; cpp17 92, cpp20 105, cpp23 105; examples rc=0 |
| T1 clang++-18 `-std=c++2c` Debug (manual; CMake 3.28 creates no cpp26 target) | 53441/53441 assertions |
| T1 g++-13 ASan+UBSan+LSan `-fno-sanitize-recover=all` | 6/6 |
| T1 clang++-18 UBSan trap (CMake + c++2c manual) | 6/6, 105/105 |
| T2 g++-13 / clang++-18 Release | 6/6 each |
| T2 `-D_GLIBCXX_ASSERTIONS -D_GLIBCXX_DEBUG`, both compilers | 6/6 each (base 3/5) |
| T2 g++-13 `-O3` c++17 / `-O2 -DNDEBUG` c++23 with ASan/UBSan; clang++-18 c++2c `-O2 -DNDEBUG` | 92/92, 103/103, 105/105 |
| Strict warnings on semver.hpp | identical to base |

Not verified in the container: GCC 14/15, other Clang versions, AppleClang/libc++, MSVC, ClangCL, -m32, Clang ASan/libFuzzer (runtimes missing). Existing CI after push and review CI cover these (see "Review CI" when present).

CI extension proposal (no workflow changed): a Linux Clang + libc++ job (`-stdlib=libc++`, apt.llvm.org `libc++-20-dev libc++abi-20-dev`), and a windows.yml ClangCL job (`-T ClangCL`, x64). Optionally a `-m32` GCC job.

## Decisions

Source: no earlier review branches found (`git log --all -- review-report.md` empty after fetching all heads); created in review 2026-10-09.
Rows D1–D8 are documented owner decisions taken from `doc/limitations.md` / `doc/reference.md` at 574883a, recorded so the review does not re-propose them.

| ID | decision | why | reference |
|---|---|---|---|
| D1 | Range grammar is a library-defined subset: no hyphen ranges, no `==`, commas, `v` prefixes, `~>`, build metadata in bounds, qualifiers on partials, wildcard/partial operands for some operators | documented limitation | doc/limitations.md "Range grammar" |
| D2 | `semver.hpp` `#undef`s system `major`/`minor` macros | avoid conflicts with `version::major()/minor()` | doc/limitations.md "System major and minor macros" |
| D3 | `SEMVER_MAX_INPUT_LENGTH` (default 512) counts prefixes and surrounding whitespace; longer input rejected with `value_too_large` before parsing; configuration must be identical in every TU | documented | doc/limitations.md "Configuration and input size" |
| D4 | Component types must be unsigned, non-cv, non-bool; constructors throw `out_of_range`, `bump_*` throw `overflow_error`, `inc` returns nullopt on overflow | documented | doc/limitations.md "Component types" |
| D5 | All range bounds incl. generated shorthand bounds must fit component types (`"255"` fails for `uint8_t`) | documented | doc/limitations.md "Component types" |
| D6 | Comparison, ranges and `std::hash` ignore build metadata; `compare_with_build` is a non-SemVer tie-breaker | SemVer §10 | doc/limitations.md "SemVer precedence" |
| D7 | `to_chars` does not append NUL | std::to_chars convention | doc/limitations.md "Formatting" |
| D8 | Default-constructed `range_set` contains nothing; `*`/`x`/`X` match all releases (all versions with `prerelease_policy::include`) | documented | doc/limitations.md "Range grammar" |
| D9 | Header requires exceptions (`-fno-exceptions` unsupported: `throw` in `component_cast`, ctors, `bump_*`, `from_string`; GCC fails on inclusion via `library_version`, Clang unless included as a system header) | awaiting owner: exceptions are part of the documented contract; supporting `-fno-exceptions` would be a new configuration hook (feature) | review step 3, report Q1; evidence.md H2, row 24 |
| D10 | `inc(v, prerelease/pre*, tag)` with an explicit tag uses the tag verbatim, so the result can be lower than or equal to `v` (`1.0.0-alpha.1` + `alpha` → `1.0.0-alpha`) | awaiting owner: follows doc/reference.md "accept a complete prerelease tag without adding a `.0` suffix"; any change is an API/behaviour change | report Q2; evidence.md H9 |
| D11 | `diff(1.0.0-alpha, 1.0.0)` returns `version_change::prerelease`, while doc/reference.md:206 says `pre*` values are used when the newer version is a prerelease | awaiting owner: either the code (npm returns `major`/`minor`/`patch`) or the sentence is wrong; API | report Q3; evidence.md H10 |
| D12 | `from_chars`/`coerce`/`clean` reject a whole buffer longer than `SEMVER_MAX_INPUT_LENGTH` even when the version prefix is short | left as is: pinned by test_parse.cpp:693-705 and test_config.cpp:52, consistent with D3; owner may revisit | report Q4; evidence.md H11 |
| D13 | Character types (`char`, `char8_t`, `char16_t`, `char32_t`, unsigned `wchar_t`) are accepted as component types and constructor sources (`version<>{'1','2','3'}` → `49.50.51`) | deferred: low impact, rejecting them changes the API | report; evidence.md H12 |
| D14 | `min_version`/`intersects` are quadratic in comparator count; a constant-evaluated `intersects` of two maximum-size (512-byte) ranges exceeds the default constexpr budgets of GCC and Clang | deferred: negligible at run time with the default limit (4 ms); documentation or algorithm change, low impact | report; evidence.md H13, O5 |
| D15 | `clean`/`coerce` trim only `' '`, ranges accept all ASCII whitespace; `coerce("1.2.3-alpha foo")` drops the valid prerelease | deferred / awaiting owner: documented ("removes outer spaces"), `"1.2.3-alpha+"` → `1.2.3` is pinned (test_api.cpp:477-483); API | report Q5; evidence.md H15, H16 |
| D16 | `from_string` copies the whole input (also over-limit input and control characters) into the `system_error` message | deferred: low impact; the message format is the owner's choice | report; evidence.md H17 |
| D17 | Length checks compare against the raw `SEMVER_MAX_INPUT_LENGTH` macro text, so an unparenthesized or signed macro value changes the comparison or warns inside the header | no warning-neutral fix: comparing with `max_input_length` or `static_cast<std::size_t>(SEMVER_MAX_INPUT_LENGTH)` fixes `true ? 64 : 0`, `0x100\|0x200`, `-1`, but makes clang++-18 `-Wtype-limits -Werror` fail for `SIZE_MAX` (`-Wtautological-type-limit-compare`), which builds today; low impact. Options: document "integer constant expression, parenthesized", or accept the Clang warning | report Q6; evidence.md H19; step 3 probe `/tmp/semver-step3/exp/h19b.sh` |
| D18 | Header-wide `#pragma GCC diagnostic ignored "-Wstringop-overflow"` without a comment | left as is: still needed (GCC 13 `-O3 -std=c++17` false positive in `to_chars`, ASan clean), push/pop scoped to the header; only a comment/narrowing would change | evidence.md H3, rows 13-15 |
| D19 | Mixing C++17/C++20 or NDEBUG/non-NDEBUG TUs gives different definitions of the same inline entities (formal ODR violation) | deferred: no diagnostic and identical behaviour with GCC LTO `-Wodr`; documentation-level | evidence.md H18 |
| D20 | Clang `-Weverything` style warnings in the header (`-Wswitch-default` 1385, `-Wcovered-switch-default` 1135/1616, C++17 `-Wexit-time-destructors` for `library_version`) | deferred: style only; the two switch warnings are mutually exclusive | evidence.md O3, H4 |
| D21 | A block-scope `constexpr` `version<>` (also from the literal) does not compile with libstdc++ (`pointer to subobject of 'v'`) | left as is: property of `std::string` in constant expressions; namespace-scope `constexpr`, `static constexpr` and run-time use work after the literal proposal | report, [proposal] commit for H1 |
| D22 | `SEMVER_HAS_CONSTEVAL_LITERAL` stays 0 on GCC < 14 and native MSVC; the name is kept although the proposed literal is a constexpr operator template | awaiting owner: the template literal compiles on g++-13, but enabling it there adds API where it is absent today | report Q7 |
| D23 | Hypotheses H4, H14, H20, H21, H22 rejected; H5-H8 unclear (old compilers, old libstdc++, pre-LWG 3636 `<format>`, clang-cl) | no change; H5-H8 are for review CI / the owner (README compatibility claim) | evidence.md "Hypothesis verdicts"; report Q8 |

## Evidence (empirical checks and hypothesis verdicts)

Logs and scripts referenced below were in the review container (`/tmp/semver-step*/`) and are not part of the branch.

### Results

| # | check | configuration | result | command |
|---|---|---|---|---|
| 1 | T1 baseline | g++-13 Debug, CMake: cpp17, cpp20, cpp23, config-cpp17, config-cpp20 | PASS 5/5 (doctest: cpp17 91/91, cpp20 100/100, cpp23 100/100) | `CM(gcc-debug, -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_BUILD_TYPE=Debug)` |
| 2 | T1 baseline | clang++-18 Debug, CMake: same 5 targets | PASS 5/5 (cpp17 91, cpp20 101, cpp23 101 — +1 = consteval literal test) | `CM(clang-debug, -DCMAKE_CXX_COMPILER=clang++-18 -DCMAKE_BUILD_TYPE=Debug)` |
| 3 | T1 baseline, C++26 | clang++-18 `-std=c++2c` Debug, manual (CMake 3.28.3 lists no `cxx_std_26`, so `semver-cpp26.t` is never created, see O9) | PASS 101/101 | `BT(clang-2c-debug, clang++-18, -std=c++2c W -g)` |
| 4 | T1 sanitizers | g++-13 Debug + `SAN` (ASan+UBSan+LSan), all 5 CMake tests | PASS 5/5 | `CM(gcc-asan, ... "-DCMAKE_CXX_FLAGS=SAN")` |
| 5 | T1 sanitizers | clang++-18 Debug UBSan trap (`-fsanitize=undefined -fsanitize-trap=undefined`), all 5 tests | PASS 5/5 | `CM(clang-ubtrap, ...)` |
| 6 | sanitizers, C++26 | clang++-18 `-std=c++2c` UBSan trap | PASS 101/101 | `BT(clang-2c-ubtrap, clang++-18, -std=c++2c W -g -fsanitize=undefined -fsanitize-trap=undefined)` |
| 7 | sanitizers, optimised | g++-13 `-std=c++17 -O3` + SAN; g++-13 `-std=c++23 -O2 -DNDEBUG` + SAN | PASS 91/91, 100/100 | `BT(gcc-asan-O3-17, ...)`, `BT(gcc-asan-O2-23, ...)` |
| 8 | T2 Release (NDEBUG) | g++-13 Release (`-O3 -DNDEBUG`, from CMakeCache) | PASS 5/5 | `CM(gcc-release, -DCMAKE_BUILD_TYPE=Release)` |
| 9 | T2 Release (NDEBUG) | clang++-18 Release | PASS 5/5 | `CM(clang-release, ...)` |
| 10 | T2 stdlib checks | `-D_GLIBCXX_ASSERTIONS`, Debug, g++-13 and clang++-18 | PASS 5/5 each | `CM(gcc-glibcxxassert, ... "-DCMAKE_CXX_FLAGS=-D_GLIBCXX_ASSERTIONS")`, same for clang |
| 11 | T2 stdlib checks | `-D_GLIBCXX_ASSERTIONS -D_GLIBCXX_DEBUG`, Debug, g++-13 and clang++-18 | **FAIL**: `semver-cpp20.t` and `semver-cpp23.t` do not compile (`test_validation.cpp` lines 394–471, only the `#if SEMVER_HAS_CONSTEXPR_RANGES` block 392–474); cpp17, config-cpp17, config-cpp20 pass (3/5). See P-2 | `CM(gcc-glibcxxdebug, ...)`; then `ninja -C b-gcc-glibcxxdebug -k 0` → `logs/gcc-glibcxxdebug.build-k0.log`, `logs/clang-glibcxxdebug.build-k0.log`; `logs/glibcxx-debug-vector.log` |
| 12 | P6/P8 config tests | `test_config.cpp` with `SEMVER_CONFIG_FILE`, g++-13 c++17/20/23, clang++-18 c++17/20/23/2c, with and without `SEMVER_TEST_LEGACY_OPTIONAL` (12 configurations) | PASS 12/12 (rc=0) | `logs/config-tests-allstd.log` |
| 13 | Unmask `-Wstringop-overflow` pragma (header copy without line 79) | g++-13, all 8 test sources × `-O1 -O2 -O3 -Os` × c++17/20/23, `-Wstringop-overflow=4`; examples at -O2/-O3 | 48 warnings, **all** in one configuration: `-O3 -std=c++17`, `test_to_string.cpp:205-206` (`to_chars` into `char buf[5]` for `1.2.3`), reported at `semver.hpp:950/954` (prerelease/build copy loops). 0 warnings in the other 95 compilations and in examples. False positive: the same code runs clean under ASan at -O3 (row 7). See O2 | `logs/unmask-stringop.log` |
| 14 | Unmask, with `_GLIBCXX_ASSERTIONS` | same header copy, g++-13 `-O2/-O3` × c++17/20 + `-D_GLIBCXX_ASSERTIONS` | 0 warnings | `logs/unmask-stringop-glibcxxassert.log` (empty) |
| 15 | Unmask doctest `SYSTEM` include | doctest via `-I` instead of `-isystem`, project warning flags, g++-13/clang++-18, c++17/23, `test.cpp test_parse.cpp test_ranges.cpp` | 0 warnings — the suppression hides nothing under the project flags | `logs/unmask-doctest-nonsystem.log` (empty) |
| 16 | Strict warnings, GCC | g++-13 `-O2 -Wall -Wextra -pedantic -Wconversion -Wsign-conversion -Wshadow -Wold-style-cast -Wuseless-cast -Wcast-qual -Wzero-as-null-pointer-constant -Wextra-semi`, 8 test sources × c++17/23 | 0 diagnostics in project paths (log empty) | `logs/strict-gcc.log` |
| 17 | Strict warnings, Clang | clang++-18 `-O2 -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-c++20-compat -Wno-padded -Wno-pre-c++17-compat -Wno-pre-c++20-compat-pedantic -Wno-unsafe-buffer-usage -Wno-c++20-extensions`, same sources | header: `-Wswitch-default` 1385; `-Wcovered-switch-default` 1135, 1616; `-Wglobal-constructors` ("requires a global destructor") + `-Wexit-time-destructors` 1684 (`library_version`, C++17 only; none at C++20, re-checked with `probes/selfcontained.cpp`). Tests: 22× `-Wctad-maybe-unsupported` (`range_set` CTAD in test_ranges.cpp). No conversion/sign/shadow warnings. See O3 | `logs/strict-clang.log` |
| 18 | Header self-contained, include guard | `#include <semver.hpp>` twice, nothing else; `W -Wshadow`; g++-13 c++17/20/23, clang++-18 c++17/20/23/2c | PASS 7/7 | `logs/self-contained.log` |
| 19 | Feature flags | print `SEMVER_HAS_*` per compiler/standard | g++-13: c++17 all 0; c++20/23 CORE=OPTIONAL=RANGES=1, CONSTEVAL=0. clang++-18: c++17 all 0; c++20/23/2c all 1. Internal `SEMVER_CONSTEXPR_*` are undefined after the header (no "LEAK" line) | `probes/flags.cpp`, `logs/feature-flags.log` |
| 20 | Attribute check: GCC<14 consteval guard | literal body copied into a probe, g++-13 c++20/23 | guard is justified: GCC 13 rejects the result ("... `_M_local_buf` ... is not a constant expression") | `probes/consteval_gcc13.cpp`, `logs/consteval-gcc13.log` |
| 21 | Attribute check: `_semver` literal where `SEMVER_HAS_CONSTEVAL_LITERAL == 1` | clang++-18 + libstdc++ 13, c++20/23/2c, six usage forms | **FAIL**: every use outside an enclosing constant evaluation is rejected, including the documented example `constexpr auto v = "1.2.3-alpha"_semver;` (doc/reference.md:351). See P-1 | `probes/lit_{A..F}.cpp`, `probes/consteval_long.cpp`, `logs/consteval-literal-forms.log`, `logs/consteval-long.log` |
| 22 | Attribute check: `[[nodiscard]]` | 10 discarded calls (parse ×2, bump_major, try_parse, coerce, valid, compare, to_chars, min_version, inc), `-Wall`, g++-13/clang++-18, c++17/20 | PASS: 10/10 warnings in each of 4 configurations | `probes/nodiscard.cpp`, `logs/nodiscard.log` |
| 23 | Comparison with std counterparts | g++-13 c++23 and clang++-18 c++2c, `W -O1` | PASS: `from_chars_result`/`to_chars_result` member types equal std; explicit bool like C++26 std; `detail::cmp_less`/`number_in_range`/`compare_numbers` equal `std::cmp_less`/`std::in_range` on all extreme pairs of 8 integer types (static_assert) and on 6·10⁶ random pairs; `detail::length` equals `std::to_chars` length; `<=>` yields `std::weak_ordering`; `std::hash` nothrow; `std::format("{}")` == `to_string()`, string_view specs work (`{:>14}`, `{:*<14}`, `{:.5}` → `1.2.3`) | `p7std/stdcmp.cpp`, `logs/std-counterparts.log` |
| 24 | P6 `-fno-exceptions` | consumer TU (parse version + range, to_chars), g++-13/clang++-18 × c++17/20/23 | does not compile in any configuration: `semver.hpp:331` (`component_cast` `throw`) — g++ "exception handling disabled", clang "cannot use 'throw' with exceptions disabled". See O4 | `probes/consumer.cpp`, `logs/p6-noexcept-nortti.log` |
| 25 | P6 `-fno-rtti` | same TU and matrix | PASS 6/6 (compiles, prints `1.2.3-rc.1+b 1`) | `logs/p6-noexcept-nortti.log` |
| 26 | P6 NDEBUG vs asserts | Debug (asserts on) rows 1–6, Release rows 8–9; differential harness (rows 31–32) built without NDEBUG | PASS; `detail::cursor` asserts never fired on ~1.3·10⁶ random inputs | — |
| 27 | P1 install + find_package | `cmake --install` into `p1/prefix`; `test/cmake-consumer` against install tree and build tree, g++-13 and clang++-18 | PASS 4/4. Installed: `include/semver.hpp`, `share/semver/LICENSE`, `lib/cmake/semver/semverConfig{,Version}.cmake`. Version check: request 1.0.0 → found 1.0.1; 1.1.0, 2.0.0, 0.9.0 → not found (SameMajorVersion as declared) | `logs/p1-packaging.log` |
| 28 | P1 add_subdirectory | parent project, g++-13/clang++-18; plus `-DSEMVER_OPT_INSTALL=ON` | PASS: options default OFF in a subproject, no test/example targets (`ninja -t targets` count 0), consumer builds and runs; install from subproject works | `logs/p1-add_subdirectory.log` |
| 29 | P1 pkg-config | — | n/a: the project ships no `.pc` file | — |
| 30 | P7 system macros | emulated `<windows.h>` macros (`min`, `max`, `small`, `near`, `far`, `interface`, `IN`, `OUT`, `OPTIONAL`, `CONST`, `VOID`, `DELETE`, `ERROR`, `TRUE`, `FALSE`, `CALLBACK`, …) before `semver.hpp`, instantiating parse/ranges/min_version/intersects/coerce/inc/hash for `uint8_t`; `<sys/sysmacros.h>` before semver.hpp; g++-13/clang++-18 × c++17/20 | PASS 8/8 (rc=0). `<sys/sysmacros.h>` included *after* semver.hpp breaks `v.major()` (expected, D2; O6) | `probes/p7_*.cpp`, `logs/p7-macros.log` |
| 31 | P5 differential vs independent reference | harness (`p5/harness.cpp`) under g++-13 Debug+SAN (seeds 1–9) and clang++-18 -O2 UBSan trap (seeds 10–13); Python reference = SemVer 2.0.0 regex + §11 precedence + range semantics from doc/reference.md | PASS: 283 000 strict-parse/valid/try_parse/from_chars/to_string cases (≈60 % invalid, 16 % overflow, 2 % over-length), 283 000 compare/operators/hash/compare_with_build pairs, 283 000 ranges with 649 836 `contains` checks (exclude and include policy): **0 mismatches**, no sanitizer report. Negative control (reference deliberately broken) reports 4/26/53 mismatches | `python3 p5/diff.py <harness> <seed> <n>`, `p5/negctl.py`, `logs/p5-differential.log` |
| 32 | P5 differential vs npm semver 7.7.2 | ranges accepted by the library without `!=` (npm has none); 100 000 ranges × 8 versions, both policies; `min_version` vs `semver.minVersion` | `contains`: **0 mismatches** in 800 000 checks when npm evaluates each `||` branch separately; 8 differences against npm's whole-range evaluation, all caused by npm collapsing a range to an "any" branch (`>=0`). `min_version`: 166 differences, all classified: 145 lib result lower than npm's and satisfies the range, 16 npm candidate violates the documented semantics (npm drops `>=0.0.0`), 3 npm returns null while the lib result satisfies, 2 npm result exceeds `uint32` (documented). 0 unexplained. See O7 | `p5/npm_diff.py`, `p5/npm_ref.js`, `p5/classify_min.py`, `logs/p5-npm-differential.log`, `logs/p5-npm-minversion-classified.log` |
| 33 | P5 edge-case table | 41 inputs through parse/from_chars/coerce/clean/parse<u64>, null/inverted pointers, to_chars with short/null buffer, SAN | consistent with doc/reference.md and D3/D7; see table below and O8 | `p5/edge.cpp`, `logs/p5-edge-table.log` |
| 34 | Limits (item 8) | g++-13 -O2, `-DSEMVER_MAX_INPUT_LENGTH=100000000`, N = 10⁴, 10⁵, 10⁶, `timeout 120` | PASS (all linear operations ≤ 0.37 s at N = 10⁶; no crash/overflow). `intersects` on K×K disjoint `||` branches: 0.003 s (K=100), 0.50 s (K=1 000), 31.8 s (K=10 000) — quadratic in branch count; see O5 | `limits/limits.cpp`, `logs/limits.log` |
| 35 | Examples executed (T2) | `basic_example`, `range_example` from gcc-debug, clang-debug, gcc-release, clang-release, gcc-asan (LSan on), clang-ubtrap | PASS 12/12 rc=0, identical output gcc-debug vs clang-release | `logs/examples-run.log`, `logs/examples-{basic,range}.out` |
| 36 | P9 build systems | CMake only; configure-time check CMake version == header macros | PASS (every configure above succeeded) | `logs/*.configure.log` |

Totals: 36 rows — 30 PASS, 3 FAIL (rows 11, 21, 24; row 24 is a design question rather than a defect),
1 n/a (row 29), 2 informational with findings recorded as observations (rows 13, 17).

#### Edge-case table (excerpt of `logs/p5-edge-table.log`, `uint32_t` unless stated)

| input | parse | from_chars | coerce | clean |
|---|---|---|---|---|
| `` | EINVAL | EINVAL@0 | nullopt | nullopt |
| `1.2.3-00` | EINVAL | ok@7 `1.2.3-0` | `1.2.3` | nullopt |
| `1.2.03` | EINVAL | ok@5 `1.2.0` | `1.2.3` | nullopt |
| `1.2.3-` / `1.2.3+` | EINVAL | ok@5 `1.2.3` | `1.2.3` | nullopt |
| `1.2.3-a..b` | EINVAL | ok@7 `1.2.3-a` | `1.2.3` | nullopt |
| ` 1.2.3 ` | EINVAL | EINVAL@0 | `1.2.3` | `1.2.3` |
| `\t1.2.3` | EINVAL | EINVAL@0 | nullopt | nullopt |
| `1.2.3\n` | EINVAL | ok@5 | `1.2.3` | nullopt |
| `4294967295.4294967295.4294967295` | ok | ok@32 | same | same |
| `4294967296.0.0` | ERANGE (u64: ok) | ERANGE@14 | nullopt | nullopt |
| `18446744073709551616.0.0` | ERANGE (u64: ERANGE) | ERANGE@24 | nullopt | nullopt |
| `1.2.3-99999999999999999999999` | ok (prerelease numbers unbounded) | ok | same | same |
| `1.2.3-alpha foo` | EINVAL | ok@11 `1.2.3-alpha` | `1.2.3` | nullopt |
| `1.2.3\0junk` (10 bytes) | EINVAL | ok@5 | `1.2.3` | nullopt |
| 512-byte `1.0.0+aaa…` | ok | ok@512 | ok | ok |
| 513-byte `1.0.0+aaa…` / 510 spaces + `1.2.3` | EOVERFLOW | EOVERFLOW@0 | nullopt | nullopt |
| `from_chars(nullptr, nullptr)`, `from_chars(last, first)` | — | EINVAL | — | — |
| `to_chars(buf, buf+4, 1.2.3)`, `to_chars(nullptr, nullptr, …)` | — | EOVERFLOW, ptr == last | — | — |

### Confirmed problems

#### P-1. `"…"_semver` literal is unusable outside an enclosing constant evaluation (Clang + libstdc++), including the documented example

Configuration: clang++-18 + libstdc++ 13, `-std=c++20`, `c++23`, `c++2c` (`SEMVER_HAS_CONSTEVAL_LITERAL == 1`, row 19).
Matches step-1 hypothesis H1 (not re-verified here beyond these probes; step 2b owns it).

```cpp
// /tmp/semver-step2/probes/lit_A.cpp — the example from doc/reference.md:351
#include <semver.hpp>
using namespace semver::literals;
constexpr auto v = "1.2.3-alpha"_semver;
int main() { return v.major() == 1 ? 0 : 1; }
```
`clang++-18 -std=c++20 -DFORM_A -I/home/user/semver/include lit_A.cpp` →
`error: call to consteval function 'semver::literals::operator""_semver' is not a constant expression`
`note: pointer to subobject of temporary is not a constant expression`.

All six forms fail on c++20 (`logs/consteval-literal-forms.log`): A namespace-scope `constexpr` (doc example),
B block-scope `constexpr` (`pointer to subobject of 'v' is not a constant expression`), C `auto v = "1.2.3"_semver;`,
D `static const auto v = …`, E `"1.2.3"_semver.major()` in `main`, F `x == "1.2.3"_semver`. A and C also fail on c++23
and c++2c. A prerelease longer than the SSO buffer fails with `pointer to subobject of heap-allocated object`
(`logs/consteval-long.log`). The test suite only uses the literal inside `static_assert([] { … }())`
(test_validation.cpp:503-520), which is why rows 2/3 pass. GCC 13 has the literal disabled (row 20, guard justified).
Boundary: every Clang 18 standard tested fails; GCC 14/15 (where the guard enables it), libc++ (AppleClang) and
MSVC STL / clang-cl are not in the container — UNCLEAR, review CI.

#### P-2. `SEMVER_HAS_CONSTEXPR_RANGES == 1` under `_GLIBCXX_DEBUG`, but range operations cannot be constant-evaluated; the project's C++20/23 test targets fail to build

Configuration: g++-13 and clang++-18 with libstdc++ 13, `-std=c++20`/`c++23`, `-D_GLIBCXX_DEBUG`.
Cause (`logs/glibcxx-debug-vector.log`): libstdc++ 13 still defines `__cpp_lib_constexpr_vector == 201907` in debug
mode, while `std::__debug::vector` is not usable in constant expressions; the header's guard (semver.hpp:104) relies
only on the feature macro.

```cpp
// /tmp/semver-step2/probes/dbgvec2.cpp
#include <vector>
constexpr int f(){ std::vector<int> v; v.push_back(1); return v[0]; }
static_assert(f()==1);
int main(){}
```
`g++-13 -std=c++20 -D_GLIBCXX_DEBUG -fsyntax-only dbgvec2.cpp` → `error: variable 'v' of non-literal type 'std::__debug::vector<int>' in 'constexpr' function only available with '-std=c++2b'`
(clang++-18: `variable of non-literal type 'std::vector<int>' cannot be defined in a constexpr function before C++23`),
while `probes/dbgvec.cpp` prints `cpp_lib_constexpr_vector=201907 RANGES=1 OPTIONAL=1 CORE=1` under the same flags.
Project impact: `cmake -DCMAKE_CXX_FLAGS="-D_GLIBCXX_ASSERTIONS -D_GLIBCXX_DEBUG"` → `FAILED: test/CMakeFiles/semver-cpp20.t.dir/test_validation.cpp.o`
and `semver-cpp23.t.dir/test_validation.cpp.o`, errors only on lines 394–471 (the `SEMVER_HAS_CONSTEXPR_RANGES` block);
`ctest`: cpp17 Passed, config-cpp17 Passed, cpp20 Not Run, config-cpp20 Passed, cpp23 Not Run.
A user `static_assert` on a range operation guarded by `SEMVER_HAS_CONSTEXPR_RANGES` fails the same way. Runtime use is
unaffected (cpp17 under `_GLIBCXX_DEBUG` passes). The defect originates in libstdc++ (macro advertised in debug mode);
whether libstdc++ 14/15 still advertises it is UNCLEAR (not in the container, review CI). `-D_GLIBCXX_ASSERTIONS` alone is fine (row 10).

### Observations

- **O1. Test runtime under `_GLIBCXX_DEBUG`.** `semver-cpp17.t` takes 19.7 s with `_GLIBCXX_DEBUG` vs 2.6 s in plain Debug and 12 s under ASan (`logs/*.test.log`) — not a problem, just the heaviest configuration.
- **O2. `-Wstringop-overflow` pragma (semver.hpp:77-80, 1741-1743).** Without it, GCC 13 warns only at `-O3 -std=c++17`, inside `to_chars` inlined into an exactly-sized `char[5]` buffer (row 13): GCC cannot prove the prerelease/build loops are not executed. ASan at -O3 confirms no overflow (row 7). A user calling `to_chars` with a small fixed buffer at -O3 `-Werror` would hit this false positive without the pragma, so the suppression is effective, but it has no explanatory comment. GCC 14/15: not in container (review CI). Clang does not need it.
- **O3. Clang `-Weverything` findings in the header** (row 17): `switch` without `default` at 1385 (`term_kind` — all enumerators handled, `expanded` initialised to `false`), `default` in fully-covered switches at 1135 and 1616, and `library_version` (1684, `inline const version<>` with `std::string` members) needs a global (exit-time) destructor in C++17; at C++20 the warning disappears (`clang++-18 -std=c++20 -Wglobal-constructors -Wexit-time-destructors` on `probes/selfcontained.cpp`: no output). Style-level; step 3 decides.
- **O4. `-fno-exceptions`** (row 24): the header cannot be included at all (`throw` in uninstantiated templates is rejected by both compilers at 331 first). Not documented in README/doc (no mention of "exception" in docs). Owner question (R4/H2).
- **O5. `intersects` cost is O(B₁·B₂·comparators²)** (row 34): with the default `SEMVER_MAX_INPUT_LENGTH` 512 at most ~170 branches per side are possible, so the worst case stays small; only with a raised limit (K = 10 000 branches) it took 31.8 s.
- **O6. `<sys/sysmacros.h>` after `semver.hpp`** re-defines `major`/`minor`, then `v.major()` expands to `gnu_dev_major` (row 30). Consistent with D2 (the header can only undef what exists at its inclusion point).
- **O7. Differences from npm semver are npm-side or documented** (row 32): npm collapses a range with an "any" branch (`>=0`, `*`) to that branch, so e.g. `<2.2.3 >0.2.3-alpha.1 ||>=0` rejects `0.2.3-rc.1` in npm but accepts it in semver.hpp (per-branch npm agrees with semver.hpp); npm strips `>=0.0.0`, admitting `0.0.0-x` prereleases that the documented expansion (`0` → `>=0.0.0 <1.0.0-0`) excludes; npm `minVersion` misses lower prerelease candidates (e.g. `<= 2.1.3-beta >2.1.2`: semver.hpp `2.1.3-0`, npm `null`; `>=0.0.0-1`: semver.hpp `0.0.0-1`, npm `0.0.0`). semver.hpp min results were checked to satisfy the range under the independent reference.
- **O8. Parser conventions visible in the edge table:** `from_chars` returns the longest valid prefix, so `1.2.03` → `1.2.0`@5 and `1.2.3-00` → `1.2.3-0`@7 (std::from_chars-like, as documented). On overflow, `from_chars` points past the whole syntactic prefix (`4294967296.0.0` → ERANGE@14). `coerce` accepts leading spaces but not a leading tab (`\t1.2.3` → nullopt) while ranges accept all ASCII whitespace; `clean` trims only spaces (`1.2.3\n` → nullopt); `coerce("1.2.3-alpha foo")` → `1.2.3` (prerelease dropped). These touch hypotheses H15/H16 — left to step 2b.
- **O9. CMake 3.28.3 reports no `cxx_std_26`** for g++-13 or clang++-18 (`CMakeCXXCompiler.cmake` lists up to `cxx_std_23`), so `semver-cpp26.t` is only built by CI runners whose CMake knows C++26; covered manually here (rows 3, 6).
- **O10. Literal coverage gap in tests:** the consteval literal is tested only inside constexpr lambdas (P-1), so CI on GCC 14/15 cannot catch P-1.

### Not done / could not

- **Clang ASan/LSan** — `libclang_rt.asan*-x86_64.a` missing (`clang++-18 -fsanitize=address` link error); replaced by UBSan trap on Clang and full ASan+UBSan+LSan on GCC.
- **libFuzzer** — `libclang_rt.fuzzer-x86_64.a` missing; replaced by randomized differential tests (rows 31–32) per P5.
- **libc++** (`-stdlib=libc++`: `cannot find -lc++`), **-m32** (`cannot find Scrt1.o`) — not in container; review CI.
- **GCC 14/15, Clang 17/19–22, old compilers from README (GCC ≥ 7, Clang ≥ 6), MSVC/ClangCL/AppleClang** — not in container. Needed in review CI in particular for: P-1 on GCC 14/15 (literal enabled there), libc++ and MSVC STL; P-2 with libstdc++ 14/15; O2 (pragma) on GCC 14/15.
- **Existing CI after push** — step 2 does not push (orchestrator only); no CI results consulted.
- **T3 boundaries** — P-1: all available standards fail on the only available Clang; P-2: only libstdc++ 13 available. Version boundaries need CI.
- **P2, P3, P4, P10, P11, P12** — not marked in profile section 10 (no compile-fail tests, no compiler strings, no heavy metaprogramming, no modules, no downloads, no C API); examples were still run under ASan+LSan (row 35).
- **pkg-config consumer** — project ships no `.pc` file.
- **Hypotheses** — `.review/hypotheses.md` appeared during the step and was skimmed; its in-container probes (H3 with `_GLIBCXX_ASSERTIONS`, done as row 14) needed no extra T2 configuration; all other H* need either step 2b probes or review CI (GCC 9–12/14/15, ClangCL, libc++). Hypotheses were not verified here (step 2b). Exhaustive `min_version`/`intersects` brute force (H14) left to step 2b.

### Hypothesis verdicts

Step 2b. Revision `574883a9aa2182c9512f9e140979a7b812406ca0`; repository files not modified (`git status --short` empty).
Every "How to check" probe from `.review/hypotheses.md` was built against `/home/user/semver/include/semver.hpp` with
g++-13 (13.3.0) and clang++-18 (18.1.3, libstdc++ 13) on the standards each hypothesis names. Work directory
`/tmp/semver-step2b/` (probe sources there, logs in `/tmp/semver-step2b/logs/<id>.log`). Probes that were inaccurate were
fixed, not their output: H17 (embedded NUL truncated `what()`, control-character case rewritten), H19 (`"1.2.3"+5` triggered
Clang `-Wstring-plus-int` in the probe itself), H20 (alphabet generator produced only 7 valid strings in 300 000; a token-based
generator `h20b.cpp` was added, with a uint64 overflow filter based on the regex groups), H13 compile-time (`std::string` copies
in the probe hit N1, see below). `h4.cpp`, `h14.cpp`, `h20.cpp` were copied unchanged from `/tmp/semver-step1/`.

| H | verdict | command | key output line | comment |
|---|---|---|---|---|
| H1 | CONFIRMED | `clang++-18 -std={c++20,c++23,c++2c} -I/home/user/semver/include -DCASE={1..6} -c h1.cpp` (probe from hypotheses.md H1) | CASE 1, 3, 4: `error: call to consteval function 'semver::literals::operator""_semver' is not a constant expression` / `note: pointer to subobject of temporary is not a constant expression`; CASE 2: `note: pointer to subobject of 'v' is not a constant expression`; CASE 5, 6 `ok` on all three standards | Same result as P-1 (row 21). g++-13: `#error "literal disabled in this configuration"` for every case (guard keeps it off, row 20). GCC 14/15, libc++ (short and long tags) and clang-cl: probes `H1.cpp`, `H1b.cpp`, `H1c.cpp`. Log `logs/h1.log` |
| H2 | CONFIRMED | `for cxx in g++-13 clang++-18; for std in c++17 c++20; for d in "" -DRANGES -DCTOR; for inc in -I -isystem: $cxx -std=$std -fno-exceptions $d $inc/home/user/semver/include h2.cpp -o h2.bin`; same with `-fno-rtti` | g++-13, all 12: `semver.hpp:331:11: error: exception handling disabled, use '-fexceptions' to enable` (also with `-isystem`); clang++-18 `-I`: `semver.hpp:331:11: error: cannot use 'throw' with exceptions disabled` (8–11 errors); clang++-18 `-isystem`: builds, `rc=0` (6/6); `-fno-rtti`: 24/24 ok rc=0 | Correction to the hypothesis: GCC fails on *inclusion*, not only on use — `library_version` (semver.hpp:1684) is built from `int` macros and instantiates the throwing `component_cast<unsigned, int>` (`required from semver.hpp:1684:104`). Clang accepts the header only as a system header. Owner question (R4, O4): no documented `-fno-exceptions` support. Log `logs/h2.log` |
| H3 | FORMAL | header copy without lines 77–80/1741–1743 (`grep -c stringop` → 0); `g++-13 -std={c++17,c++20} {-O2,-O3} -Wall -Wextra -Wundef -I nopragma ... -c test/test_to_string.cpp`; user-code control `h3user*.cpp` with/without the header | only `-O3 -std=c++17`: `semver.hpp:947:29: warning: writing 1 byte into a region of size 0 [-Wstringop-overflow=]` (14×) and `:951:30` (lines shifted by −3 in the copy = 950/954 in the original); 0 at `-O2` and at C++20 | Confirms rows 13–15: the pragma suppresses a GCC 13 false positive (ASan clean at -O3, row 7) and is still needed; it is location-scoped (push/pop), user-code warnings after the header are unchanged (`h3user.cpp`: the same `-Warray-bounds` warning with and without the header). Only formal issues: no comment, header-wide scope. Log `logs/h3.log` |
| H4 | REJECTED | `g++-13 -std={c++17,c++20,c++23} -O2 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wold-style-cast -Wuseless-cast -Wcast-qual -Wnull-dereference -Wduplicated-cond -Wlogical-op -Wundef -c h4.cpp`; `clang++-18 -std={c++17,c++20,c++2c} -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-pre-c++17-compat -Wno-c++20-compat -Wno-unsafe-buffer-usage -Wno-ctad-maybe-unsupported -c h4.cpp` | GCC: no diagnostics on any standard. Clang: only `semver.hpp:1135 / 1616 [-Wcovered-switch-default]`, `semver.hpp:1385 [-Wswitch-default]`, and at C++17 `semver.hpp:1684:24 [-Wglobal-constructors] / [-Wexit-time-destructors]` | No conversion, sign, shadow, old-style-cast or useless-cast warning for `uint8_t`, mixed `uint16/32/64`, `version<>` and `unsigned char/short/long long`. The remaining `-Weverything` diagnostics are the style items already recorded in O3 (the two switch warnings are mutually exclusive by design of `-Weverything`). Log `logs/h4.log` |
| H5 | UNCLEAR | — | — | Needs GCC 9–12 / Clang 11–15 (ubuntu-22.04 official repositories); GCC 7/8, Clang 6–10, VS 2019, AppleClang 10 are not on current GitHub images (owner: test or lower the README claim). Probe `H5.cpp` (jobs `gcc-old`, `clang-old`) |
| H6 | UNCLEAR | `grep -n "three_way" include/semver.hpp` | `82:#if defined(__cpp_impl_three_way_comparison) ...` and `848:` — the guard uses the compiler macro only; libstdc++ 13 has `<compare>` (`/usr/include/c++/13/compare`) | Needs Clang ≥ 10 with libstdc++ ≤ 9 headers (only `/usr/include/c++/13` in the container). Probe `H6.cpp` (job `clang-oldlibstdcxx`: ubuntu-22.04, `g++-9` + `clang-14`, `-nostdinc++ -isystem /usr/include/c++/9 ...`) |
| H7 | UNCLEAR | `{g++-13,clang++-18} -std={c++20,c++23} -Wall -Wextra -pedantic-errors -Werror h7.cpp && ./h7.bin` (`std::format("[{:>12}]", version<>{1,2,3})`) | `[       1.2.3] rc=0` on all four; `__cpp_lib_format 202110L` | Works on libstdc++ 13 (row 23 too). The failure mode needs a pre-LWG 3636 library (VS 2019 16.10/16.11, early libc++) — not in the container and not on current CI images. Probe `H7.cpp` checks the current CI libraries only. Log `logs/h7.log` |
| H8 | UNCLEAR | — | — | Needs clang-cl + MSVC STL (windows runners). Probe `H8.cpp` (job `clangcl`); the full test suite under ClangCL additionally needs a CMake job (`-G "Visual Studio 17 2022" -T ClangCL -A x64`, Debug and Release). Note: if clang-cl ends up with `SEMVER_HAS_CONSTEXPR_RANGES == 1`, N1 below applies there too |
| H9 | CONFIRMED | `{g++-13,clang++-18} -std=c++17 h9.cpp -o h9 && ./h9` (probe from hypotheses.md H9) | `1.0.0-alpha.1 +prerelease(alpha) -> 1.0.0-alpha (LESS than input)`, `1.0.0-beta.1 +prerelease(alpha) -> 1.0.0-alpha (LESS than input)`, `1.0.0-alpha +prerelease(alpha) -> 1.0.0-alpha (EQUAL to input)`, `1.0.0-rc.1 +prerelease(rc) -> 1.0.0-rc (LESS than input)` | npm semver 7.7.2 `inc(v, "prerelease", id)`: `1.0.0-alpha.2`, `1.0.0-alpha.0` (also lower than the input), `1.0.0-alpha.0`, `1.0.0-rc.2`. The code follows the doc sentence "accept a complete prerelease tag", so this is a design question (result not greater than the input), API if changed. Log `logs/h9.log` |
| H10 | CONFIRMED | `g++-13 -std=c++17 h10.cpp -o h10 && ./h10`; `h10b.cpp` (round trip through `inc`) | `1.0.0-alpha -> 1.0.0 : 7`, `1.2.0-rc.1 -> 1.2.0 : 7`, `1.2.3-rc.1 -> 1.2.3 : 7`; `diff=7 inc(1.0.0-alpha, diff)=1.0.0-alpha.0`; none of `inc(1.0.0-alpha, 1..7)` yields `1.0.0` | Contradicts doc/reference.md:206 ("a `pre*` value when the newer version is a prerelease") and its "use the same enum with `inc`"; consistent with the enum comment at semver.hpp:476. npm `diff`: `major`, `minor`, `patch`. Design question, API. Log `logs/h10.log` |
| H11 | CONFIRMED | `g++-13 -std=c++17 h11.cpp -o h11 && ./h11` | `from_chars ok=0 ec_too_large=1 consumed=0`, `coerce has_value=0`; the same buffer cut to 512 bytes: `from_chars first 512 bytes ok=1 consumed=5 v=1.2.3` | Behaviour is pinned by tests (test_parse.cpp:693-705, test_config.cpp:52) and recorded in D3; the new argument (prefix-oriented APIs reject because of bytes they never read) is for the owner, not a bug fix. Log `logs/h11.log` |
| H12 | CONFIRMED | `{g++-13,clang++-18} -std={c++17,c++20} -Wall -Wextra -pedantic-errors -Werror h12.cpp -o h12 && ./h12`; `version<wchar_t>` TU | `49.50.51 49.50.51 1`, `1.2.3` (`version<char16_t, char32_t>`), `0.1.0` (`version<char8_t>`, C++20); `wchar_t accepted as constructor source: 1`; `version<wchar_t>` rejected on x86-64 Linux (`wchar_t` signed): `constraints not satisfied for class template 'version'` | No warning even with `-Wall -Wextra -pedantic-errors -Werror`. Windows (`wchar_t` unsigned): probe `H12.cpp`. API if changed. Log `logs/h12.log` |
| H13 | CONFIRMED | `g++-13 -std=c++17 -O2 -DSEMVER_MAX_INPUT_LENGTH=$N h13.cpp` for N = 512, 4096, 16384, 65536; compile time: `h13c.cpp`, `h13c2.cpp` (N1 workaround), `h13c4.cpp` with `-DSEMVER_MAX_INPUT_LENGTH=16…512` | run time: `limit=512 terms=256 intersects=0 0.004s`, `limit=4096 ... 0.165s`, `limit=16384 ... 3.393s`, `limit=65536 terms=32768 intersects=0 64.138s min_version=1.0.0 6.540s` (×4 input → ×19–21 time); compile time, default limit 512: g++-13 `'constexpr' evaluation operation count exceeds limit of 33554432` (also at 256; 64 and 128 pass), clang++-18 `semver.hpp:547:5: note: constexpr evaluation hit maximum step limit` | Quadratic in comparator count. With the default limit the run-time cost is negligible (4 ms); a constant-evaluated `intersects` of two maximum-size ranges does not fit the default constexpr budgets of either compiler. Quality/documentation, not API. Logs `logs/h13.log`, `logs/h13c*.log` |
| H14 | REJECTED | `g++-13 -std=c++17 -O2 -fsanitize=address,undefined -fno-sanitize-recover=all h14.cpp -o h14 && ./h14 {1..5} 3000`; `./h14_fast {6..25} 20000` (no sanitizers) | `checked=6000 bugs=0 gaps=0` ×5 (rc=0); `20 checked=40000 bugs=0 gaps=0` | 830 000 `min_version`/`intersects` checks against `contains` over the closed domain, both prerelease policies, no sanitizer report. Negative control (header copy without the `bound-0` candidate, `negctl/semver.hpp`): `checked=206 bugs=20`, e.g. `BUG min_version not minimal \| pol=include \| r1=">=1.3" ... ref_min=1.3.0-0 got_min=1.3.0`. Log `logs/h14.log` |
| H15 | CONFIRMED | `g++-13 -std=c++17 h15.cpp -o h15 && ./h15` | `0 0 0 1` (`clean("\t1.2.3\n")`, `clean("1.2.3\r\n")`, `coerce("\t1.2.3")` fail; `satisfies(v, "\t1.2.3\n")` true) | npm 7.7.2: `1.2.3 1.2.3 1.2.3`. Documented ("removes outer spaces", reference.md:158) → inconsistency/design question, API if changed. Log `logs/h15.log` |
| H16 | CONFIRMED | `g++-13 -std=c++17 h16.cpp -o h16 && ./h16` | `[1.2.3-alpha foo] -> 1.2.3`, `[1.2.3-alpha] -> 1.2.3-alpha`, `[1.2.3 foo] -> 1.2.3`, `[1.2.3-alpha+build foo] -> 1.2.3`, `[1.2.3-alpha+] -> 1.2.3` | npm `coerce(…, {includePrerelease:true})` keeps `1.2.3-alpha` for `"1.2.3-alpha foo"` and `"1.2.3-alpha+build foo"`. The last case is pinned (test_api.cpp:477-483). Design question. Log `logs/h16.log` |
| H17 | CONFIRMED | `g++-13 -std=c++17 -O2 h17.cpp -o h17 && ./h17` | `what_len=16777255 code=75` (16 MiB input rejected with `EOVERFLOW` before parsing, message still holds the whole input); `ctrl_in_what=1` (ESC copied into `what()`) | semver.hpp:974 `throw std::system_error(..., std::string{str})`. Low severity; fix changes only the `what()` text. Log `logs/h17.log` |
| H18 | FORMAL | `{g++-13,clang++-18} -O2 -flto -Wodr -std=c++17 -c tu1.cpp; ... -std=c++20 -DNDEBUG -c tu2.cpp; link with -flto -Wodr` (g++ also `-Wlto-type-mismatch`; and c++17 + c++23) | no `-Wodr` / `-Wlto-type-mismatch` output; `1.2.3-a0 1.2.3-a0`, `rc=0` in all three combinations | Mixing standards/NDEBUG gives different definitions of the same inline entities (formal ODR violation), with no diagnostic and identical behaviour. Documentation-level at most. Log `logs/h18.log` |
| H19 | CONFIRMED | `{g++-13,clang++-18} -std=c++17 -Wall -Wextra -Wsign-compare -Wtype-limits -Werror "-DSEMVER_MAX_INPUT_LENGTH=$L" h19.cpp` for 10 values | `true?1024:0`: `max_input_length=1024 accepted=1/5` (both compilers: `str.size() > true?1024:0` parses as `(str.size() > true) ? 1024 : 0`); `0x100\|0x200`: `semver.hpp:982:18: error: suggest parentheses around comparison in operand of '\|' [-Werror=parentheses]` (clang: `semver.hpp:824:20: error: \| has lower precedence than >`); `-1`: `semver.hpp:982:18: error: comparison of integer expressions of different signedness` (clang `semver.hpp:824:18`); `0, SIZE_MAX, 18446744073709551615ULL, 1024*1024, (int)512, 512u, 100+412`: build ok, consistent | The public constant `max_input_length` and the checks at semver.hpp:824/893/982/1675 can disagree because the checks expand the raw macro. Unusual macro values only; low severity. Log `logs/h19.log` |
| H20 | REJECTED | `g++-13 -std=c++17 -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all h20.cpp && ./h20 {1,2,3} 300000`; `clang++-18 -std=c++20 -O1 -fsanitize=undefined -fsanitize-trap=undefined h20.cpp && ./h20c 4 300000`; strengthened `h20b.cpp` (same sanitizers) `./h20b {1,2,3} 200000` | `mismatches=0` ×4; `h20b`: `mismatches=0 valid=6884 ranges=8933`, `valid=7005 ranges=9019`, `valid=7106 ranges=8937`, rc=0 | No regex/`valid` disagreement, `from_chars` always returned the longest valid prefix, no sanitizer report in coerce/clean/range parsing/`min_version`/`intersects`. The original generator was too weak (`valid=7` in 300 000, `h20s.cpp`), hence `h20b`; the first `h20b` run reported mismatches that were all uint64 overflow from concatenated digits (probe filter fixed, not the output). Also covered by row 31 (283 000 differential cases). Log `logs/h20.log` |
| H21 | REJECTED | `/tmp/semver-step2b/sub`: `add_subdirectory(/home/user/semver semver-build)`, `app` = `h4.cpp` with `-Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -Werror`; `cmake -S . -B b-$cxx -G Ninja -DCMAKE_CXX_COMPILER=$cxx && cmake --build b-$cxx` | `[2/2] Linking CXX executable app`, rc=0 for g++-13 and clang++-18; include flag `-I/home/user/semver/include` (non-SYSTEM); no semver test/example targets in `ninja -t targets all` | The non-SYSTEM include produces no warning under these flags (see H4). The plain target name clashes as expected for any CMake target name: a parent with `add_library(semver INTERFACE)` gets `CMake Error at /home/user/semver/CMakeLists.txt:25 (add_library): add_library cannot create target "semver" because another target with the same name already exists` — generic CMake behaviour, not a defect. Row 28 already covers options/consumer. Log `logs/h21.log` |
| H22 | REJECTED | `g++-13 -std=c++17 -Wall -Wextra -Werror -c test/test_windows_minmax.cpp`; `clang++-18 -std=c++20 ...` | `test_windows_minmax.cpp:6:10: fatal error: windows.h: No such file or directory` (clang: `'windows.h' file not found`); with an empty stub `windows.h`: both ok | The hypothesis premise is wrong: the test includes `<windows.h>` unconditionally, so it cannot be enabled for GCC/Clang unchanged. The header itself is clean: no unparenthesized `numeric_limits<…>::max()/min()` (`grep` empty), and row 30 already runs the emulated Windows macros with GCC/Clang. Log `logs/h22.log` |

Totals: 22 hypotheses — 11 CONFIRMED (H1, H2, H9, H10, H11, H12, H13, H15, H16, H17, H19), 2 FORMAL (H3, H18),
5 REJECTED (H4, H14, H20, H21, H22), 4 UNCLEAR (H5, H6, H7, H8). Of the CONFIRMED ones H9, H10, H11, H15, H16 are
design/owner questions (behaviour documented or pinned by tests), H2 is an owner question (R4).

#### N1 (new, found while running H13): first constant-evaluated range operation fails on Clang

Configuration: clang++-18 + libstdc++ 13, `-std=c++20`, `c++23`, `c++2c` (`SEMVER_HAS_CONSTEXPR_RANGES == 1`, row 19).
Not covered by any hypothesis; recorded here because it changes what `SEMVER_HAS_CONSTEXPR_RANGES` promises.

```cpp
// /tmp/semver-step2b/n1.cpp (CASE 6), nothing else in the TU
#include <semver.hpp>
static_assert(semver::satisfies(semver::version<>{1, 2, 3}, ">=1.0.0 <2.0.0"));
int main() {}
```
`clang++-18 -std=c++20 -I/home/user/semver/include -DCASE=6 -c n1.cpp` →
`error: static assertion expression is not an integral constant expression`,
`semver.hpp:684:18: note: undefined function 'parse_number<unsigned long>' cannot be used in a constant expression`,
`semver.hpp:729:43: note: declared here`. g++-13: ok.

- Scope (`logs/n1.log`): every range parse fails as the first constant evaluation in a TU (`try_parse_range("1")`,
  `">=1.2.3"`, `"^1.2"`, `satisfies(...)`, `try_parse_range<uint64_t>`), also inside a function or lambda
  (`h13e.cpp` CASE 4, the test-suite pattern). `valid`, `try_parse<uint8_t>`, `coerce`, `clean`, `inc`, `to_string`
  are fine.
- Cause: `version_parser::parse_partial` (non-template, line ~673) calls the member template `parse_number` (defined
  later in the class, line 729) with `std::uint64_t`; Clang cannot instantiate the definition at that point and defers it
  to the end of the TU, so a constant evaluation before that sees an undefined function. Any earlier use that instantiates
  `parse_number<uint64_t>` from a template context hides it: `static_assert(semver::valid<std::uint64_t>("1.2.3"));` or
  even a run-time `semver::valid<std::uint64_t>(...)` before the `static_assert` makes it pass (`h13e.cpp` CASE 1,
  `n1b.cpp` CASE 1/2).
- Why CI is green: `test_validation.cpp:63` (`parse("1.2.3", v64)`) and `:77-78` (`valid<uint64_t>`) run before the
  constexpr block at line 392. With those three lines commented out (`n1t/test_validation.cpp`):
  `clang++-18 -std=c++20 ... -fsyntax-only n1t/test_validation.cpp` → `n1t/test_validation.cpp:394:17: error: static
  assertion expression is not an integral constant expression` / `undefined function 'parse_number<unsigned long>'`
  (`logs/n1-testsuite.log`); the unmodified file passes.
- Second, toolchain-level part: once `parse_number` is instantiated, `min_version`/`intersects` as first constant
  evaluation fail with `undefined function '_M_construct<const char *>'` from `std::string{bound.prerelease_tag()}`
  (semver.hpp:1456). That one is Clang + libstdc++ 13 itself: `static_assert(std::string{std::string_view{"x"}}.size() == 1);`
  alone fails the same way with clang++-18 (g++-13 ok, `logs/n1d.log`).
- Other compilers: GCC 13 not affected; libc++ / AppleClang: probe `R1.cpp`; MSVC/GCC latest: `R1c.cpp`.

#### Probes for review CI (`.review/probes/`)

`run.py` is the Appendix A runner, unchanged. All probes were run with it locally (stand-in compilers g++-13 /
clang++-18 for every job id, gcc style), `logs/probes-local.log`; they parse, and where the local toolchain matches the
job they behave as predicted (H1c, R1, R1b, R1c, R5, H7 "held"); stand-in rows for msvc/libc++/old-compiler jobs are not
evidence.

| file | where | std | expect | decides |
|---|---|---|---|---|
| H1.cpp | gcc-latest | c++20 c++23 | build-fail "not a constant expression" | H1 on GCC ≥ 14 + libstdc++ ≥ 14 (literal enabled there) |
| H1b.cpp | clang-libcxx macos-appleclang clangcl | c++20 c++23 | run-ok | H1 with short tags on libc++ / MSVC STL |
| H1c.cpp | clang-libcxx macos-appleclang clangcl | c++20 c++23 | build-fail "not a constant expression" | H1 with tags longer than the SSO buffer |
| H5.cpp | gcc-old clang-old (new job ids) | c++17 c++20 | build-fail "error" | H5/R6 README compatibility claim on GCC 9–12, Clang 11–15 |
| H6.cpp | clang-oldlibstdcxx (new job id) | c++20 | build-fail "compare" | H6 `<compare>` guard |
| H7.cpp | gcc-latest clang-libcxx macos-appleclang msvc-x64 msvc-x86 clangcl | c++20 c++23 | run-ok | H7 on current libraries (old ones stay UNCLEAR) |
| H8.cpp | clangcl | c++20 c++23 | run-ok | H8 clang-cl flag values and usability |
| H12.cpp | msvc-x64 msvc-x86 clangcl | c++17 c++20 | run-ok | H12 with unsigned `wchar_t` |
| R1.cpp | clang-libcxx macos-appleclang | c++20 c++23 | build-fail "undefined function" | R1 / N1 on other Clang versions and libc++ |
| R1b.cpp | gcc-latest | c++20 c++23 | build-fail "__debug::vector" | R1 / P-2 on libstdc++ 14/15 |
| R1c.cpp | gcc-latest gcc-m32 msvc-x64 msvc-x86 | c++20 c++23 | run-ok | R1 flags usable at namespace scope (first use) on non-Clang toolchains |
| R5.cpp | gcc-m32 msvc-x86 msvc-x64 | c++17 c++20 | run-ok | R5 32-bit run-time behaviour (msvc-x64 = control) |

Risks checked in the container instead of a probe: R2 (H20, rows 31–33), R3 (H14), R4 (H2, row 24), R7 (H3, rows 13–15).
R6 is H5. Job ids `gcc-old`, `clang-old`, `clang-oldlibstdcxx` are not in the standard list and need jobs in review.yml
(ubuntu-22.04 official packages: `g++-9 … g++-12`, `clang-11 … clang-15`; for `clang-oldlibstdcxx` `g++-9 clang-14` and
`--flags "-nostdinc++ -isystem /usr/include/c++/9 -isystem /usr/include/x86_64-linux-gnu/c++/9"`); without them H5/H6 stay UNCLEAR.
