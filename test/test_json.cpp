// test_json.cpp — Unit tests for JSON extraction

#include "json.h"
#include <cassert>
#include <iostream>

static void test_extract_simple() {
    std::string json = R"({"response":"hello world"})";
    assert(json_extract_string(json, "response") == "hello world");
    std::cout << "PASS: test_extract_simple\n";
}

static void test_extract_escaped_newline() {
    std::string json = R"({"response":"line1\nline2"})";
    assert(json_extract_string(json, "response") == "line1\nline2");
    std::cout << "PASS: test_extract_escaped_newline\n";
}

static void test_extract_escaped_quote() {
    std::string json = R"({"response":"say \"hello\""})";
    assert(json_extract_string(json, "response") == "say \"hello\"");
    std::cout << "PASS: test_extract_escaped_quote\n";
}

static void test_extract_missing_key() {
    std::string json = R"({"response":"hello"})";
    assert(json_extract_string(json, "missing") == "");
    std::cout << "PASS: test_extract_missing_key\n";
}

static void test_extract_from_larger_json() {
    std::string json = R"({"model":"gemma4","response":"the answer","done":true})";
    assert(json_extract_string(json, "response") == "the answer");
    std::cout << "PASS: test_extract_from_larger_json\n";
}

int main() {
    test_extract_simple();
    test_extract_escaped_newline();
    test_extract_escaped_quote();
    test_extract_missing_key();
    test_extract_from_larger_json();
    std::cout << "\nAll JSON tests passed.\n";
    return 0;
}
