#include <catch2/catch_test_macros.hpp>

#include "../src/htmldecode.h"

using namespace std::literals;

TEST_CASE("Text without mnemonics", "[HtmlDecode]") {
    CHECK(HtmlDecode("&amp;lt"sv) == "&lt"s);
    CHECK(HtmlDecode(""sv) == ""s);
    CHECK(HtmlDecode("hello"sv) == "hello"s);
    CHECK(HtmlDecode("Johnson&amp;Johnson"sv) == "Johnson&Johnson"s);
    CHECK(HtmlDecode("Johnson&APOSJohnson"sv) == "Johnson'Johnson"s);
    CHECK(HtmlDecode("Johnson&Quot;Johnson"sv) == "Johnson&Quot;Johnson"s);
    CHECK(HtmlDecode("&lt;Johnson&amp;Johnson&gt"sv) == "<Johnson&Johnson>"s);
    CHECK(HtmlDecode("&Johnson&Am"sv) == "&Johnson&Am"s);
}

// Напишите недостающие тесты самостоятельно
