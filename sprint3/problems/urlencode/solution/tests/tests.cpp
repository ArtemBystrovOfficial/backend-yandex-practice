#include <gtest/gtest.h>
#include <algorithm>
#include <iostream>
#include "../src/urlencode.h"

using namespace std::literals;

static std::string ToUpperCase(const std::string& input) {
    std::string result = input;

    auto percentPos = std::find(result.begin(), result.end(), '%');

    while (percentPos != result.end()) {
        if (std::distance(percentPos, result.end()) >= 3) {
            std::transform(percentPos + 1, percentPos + 3, percentPos + 1, [](char c) {
                return std::toupper(c);
            });
        }
        percentPos = std::find(percentPos + 1, result.end(), '%');
    }

    return result;
}

TEST(UrlEncodeTestSuite, OrdinaryCharsAreNotEncoded) {
    //A
    bool is_valid = true;
    //A
    auto str = UrlEncode(char(15)+"кириллица"s);
    for (size_t i = 0; i < str.length(); i += 3) 
        if (str[i] != '%') 
            is_valid = false;
    std::cout <<str;
    //A
    EXPECT_EQ(UrlEncode(""sv), ""s);
    EXPECT_EQ(UrlEncode("hello"sv), "hello"s);
    EXPECT_EQ(ToUpperCase(UrlEncode("Hello!#$&'()*+,/:;=?@[]World"sv)), "Hello%21%23%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5DWorld"s);
    EXPECT_EQ(ToUpperCase(UrlEncode("sum is 352 + 241"sv)), "sum+is+352+%2B+241"s);
    EXPECT_TRUE(is_valid);
}

/* Напишите остальные тесты самостоятельно */
