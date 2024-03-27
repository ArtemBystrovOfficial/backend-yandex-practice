#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(UrlDecode_tests) {
    using namespace std::literals;

    BOOST_TEST(UrlDecode(""sv) == ""s);
    BOOST_TEST(UrlDecode("/example/string/without/percent/encoding"sv) == "/example/string/without/percent/encoding"s);
    BOOST_TEST(UrlDecode("%6d%6D"sv) == "mm"s);

    BOOST_CHECK_THROW (UrlDecode("/invalid/%2zsequence/%2Ghere"sv),std::exception);
    BOOST_CHECK_THROW (UrlDecode("/%+"sv),std::exception);
    BOOST_CHECK_THROW (UrlDecode("%%%%%%%%"sv),std::exception);

    BOOST_TEST(UrlDecode("/path/with/plus+symbol"sv) == "/path/with/plus symbol"s);
}