#include <semver.hpp>
#include <catch.hpp>
#include <array>

using namespace semver;

TEST_CASE("validation") {
  SECTION("constexpr prerelease tag validation") {
    constexpr std::string_view pre = "alpha";
    STATIC_REQUIRE(detail::is_prerelease_valid(pre));

    constexpr std::string_view pre2 = "alpha.1";
    STATIC_REQUIRE(detail::is_prerelease_valid(pre2));

    constexpr std::string_view pre3 = "alpha.beta";
    STATIC_REQUIRE(detail::is_prerelease_valid(pre3));

    constexpr std::string_view pre4 = "alpha.beta.1";
    STATIC_REQUIRE(detail::is_prerelease_valid(pre4));

    constexpr std::string_view pre5 = "alpha0.valid";
    STATIC_REQUIRE(detail::is_prerelease_valid(pre5));

    constexpr std::string_view pre6 = "alpha.0valid";
    STATIC_REQUIRE(detail::is_prerelease_valid(pre6));

    constexpr std::string_view pre7 = "somethinglong.1-aef.1-its-okay";
    STATIC_REQUIRE(detail::is_prerelease_valid(pre7));

    constexpr std::string_view pre8 = "---R-S.12.9.1--.12";
    STATIC_REQUIRE(detail::is_prerelease_valid(pre8));

    constexpr std::string_view v_str = "1.2.3-alpha.3";
    constexpr std::string_view pre9 = {v_str.data() + 6, 7};
    STATIC_REQUIRE(detail::is_prerelease_valid(pre9));

    constexpr std::string_view pre10 = "%alpha%";
    STATIC_REQUIRE(!detail::is_prerelease_valid(pre10));

    constexpr std::string_view pre11 = "rc*123";
    STATIC_REQUIRE(!detail::is_prerelease_valid(pre11));

    constexpr std::string_view pre12 = "tag_with_underscores";
    STATIC_REQUIRE(!detail::is_prerelease_valid(pre12));

    constexpr std::string_view pre13 = "dev/snapshot-1.2.3";
    STATIC_REQUIRE(!detail::is_prerelease_valid(pre13));

    constexpr std::string_view pre14 = "alpha.beta@146";
    STATIC_REQUIRE(!detail::is_prerelease_valid(pre14));
  }

  SECTION("prerelease tag validation") {
    constexpr std::array<std::string_view, 20> valid_prerelease_tags = {{
      "prerelease",
      "alpha",
      "beta",
      "alpha.beta",
      "alpha.beta.1",
      "alpha.1",
      "alpha0.valid",
      "alpha.0valid",
      "alpha-a.b-c-somethinglong",
      "rc.4",
      "1.2.3-beta",
      "DEV-SNAPSHOT",
      "SNAPSHOT-123",
      "1227",
      "---RC-SNAPSHOT.12.9.1--.12",
      "---R-S.12.9.1--.12",
      "----RC-SNAPSHOT.12.9.1--.12",
      "-rc.10000aaa-kk-0.1",
      "999999999.999999999.999999999",
      "1.0.0-0A.is.legal"
    }};

    for (const auto& prerelease: valid_prerelease_tags) {
      version v;
      REQUIRE(v.set_prerelease_noexcept(prerelease));
      REQUIRE(detail::compare_equal(prerelease, v.get_prerelease()));
    }

    constexpr std::array<std::string_view, 23> invalid_prerelease_tags = {{
      "%alpha%",
      "rc*123",
      "tag_with_underscores",
      "dev/snapshot-1.2.3",
      "alpha.beta@146",
      "--rc.1.best_release_ever",
      "___main.01",
      "!tag",
      "^caret^tag^",
      "{alpha.beta.rc.0}",
      "alpha;beta.1",
      "////|\\\\",
      "$dev.0001",
      "beta#123",
      "snapshot№1",
      "???",
      "beta&alpha&rc",
      "<some.tag>",
      ">_<",
      "(((ololo)))",
      ":prerelease:",
      ".",
      "alpha."
    }};

    for (const auto& prerelease: invalid_prerelease_tags) {
      version v;
      REQUIRE(!v.set_prerelease_noexcept(prerelease));
      REQUIRE(v.get_prerelease().empty());
    }
  }

  SECTION("constexpr build_metadata tag validation") {
    constexpr std::string_view bm = "build";
    STATIC_REQUIRE(detail::is_build_metadata_valid(bm));

    constexpr std::string_view bm2 = "meta-valid";
    STATIC_REQUIRE(detail::is_build_metadata_valid(bm2));

    constexpr std::string_view bm3 = "build.1-aef.1-its-okay";
    STATIC_REQUIRE(detail::is_build_metadata_valid(bm3));

    constexpr std::string_view bm4 = "build.1";
    STATIC_REQUIRE(detail::is_build_metadata_valid(bm4));

    constexpr std::string_view bm5 = "build.123";
    STATIC_REQUIRE(detail::is_build_metadata_valid(bm5));

    constexpr std::string_view bm6 = "-DEV-SNAPSHOT";
    STATIC_REQUIRE(detail::is_build_metadata_valid(bm6));

    constexpr std::string_view bm7 = "788";
    STATIC_REQUIRE(detail::is_build_metadata_valid(bm7));

    constexpr std::string_view bm8 = "999999999.999999999.999999999";
    STATIC_REQUIRE(detail::is_build_metadata_valid(bm8));

    constexpr std::string_view bm9 = "+build.1848";
    STATIC_REQUIRE(!detail::is_build_metadata_valid(bm9));

    constexpr std::string_view bm10 = "alpha+beta";
    STATIC_REQUIRE(!detail::is_build_metadata_valid(bm10));

    constexpr std::string_view bm11 = "build++";
    STATIC_REQUIRE(!detail::is_build_metadata_valid(bm11));

    constexpr std::string_view bm12 = "some.build!";
    STATIC_REQUIRE(!detail::is_build_metadata_valid(bm12));

    constexpr std::string_view bm13 = "some.build?";
    STATIC_REQUIRE(!detail::is_build_metadata_valid(bm13));

    constexpr std::string_view bm14 = "//thebestbuild";
    STATIC_REQUIRE(!detail::is_build_metadata_valid(bm14));

    constexpr std::string_view bm15 = "meta_2021_10_26";
    STATIC_REQUIRE(!detail::is_build_metadata_valid(bm15));
  }

  SECTION("build_metadata tag validation") {
    constexpr std::array<std::string_view, 19> valid_build_metadata = {{
      "build",
      "meta-valid",
      "build.1-aef.1-its-okay",
      "build.1",
      "build.123",
      "-DEV-SNAPSHOT",
      "build.1848",
      "alpha.beta",
      "788",
      "999999999.999999999.999999999",
      "exp.sha.5114f85",
      "-",
      "build-meta-data",
      "my.cat.is.very.cute",
      "000001",
      "we.cant.use.plus.sign",
      "--and-hyphen-too-",
      "-123-456--",
      "21AF26D3--117B344092BD"
    }};

    for (const auto& bm: valid_build_metadata) {
      version v;
      REQUIRE(v.set_build_metadata_noexcept(bm));
      REQUIRE(detail::compare_equal(bm, v.get_build_metadata()));
    }

    constexpr std::array<std::string_view, 24> invalid_build_metadata = {{
      "some.build!",
      "some.build?",
      "//thebestbuild",
      "meta_2021_10_26",
      "%meta_dAtA%",
      "мета",
      "we&have&some&build",
      "cat*has*generated*this*meta",
      "_notvalidbuildmeta",
      "#",
      "#123",
      "no/",
      "you-shall-not-pass!",
      "(this-meta-invalid-too)",
      "[some-data-001]",
      "<and.this.invalid>",
      ">>>",
      "build@1937",
      "build^extrainfo",
      "additional;info;",
      "...",
      "build.",
      ".build",
      "build.."
    }};

    for (const auto& bm: invalid_build_metadata) {
      version v;
      REQUIRE(!v.set_build_metadata_noexcept(bm));
      REQUIRE(v.get_build_metadata().empty());
    }
  }
}