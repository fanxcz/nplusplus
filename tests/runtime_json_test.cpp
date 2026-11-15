#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

extern "C" bool npp_json_valid(const char* json);
extern "C" char* npp_json_get(const char* json, const char* key);
extern "C" char* npp_json_type(const char* json, const char* key);
extern "C" int64_t npp_json_array_len(const char* json, const char* key);
extern "C" char* npp_json_array_get(const char* json, const char* key, int64_t index);
extern "C" void npp_free(void* pointer);

namespace {
    int failures = 0;

    std::string take(char* value) {
        if (!value)
            return {};
        std::string result(value);
        npp_free(value);
        return result;
    }

    void expect(bool condition, const char* label) {
        if (condition)
            return;
        std::fprintf(stderr, "JSON runtime test failed: %s\n", label);
        ++failures;
    }

    void expectString(const char* label, char* actual, const char* expected) {
        const std::string result = take(actual);
        if (result == expected)
            return;
        std::fprintf(stderr,
                     "JSON runtime test failed: %s (got [%s], expected [%s])\n",
                     label,
                     result.c_str(),
                     expected);
        ++failures;
    }
}

int main() {
    const char* json = R"json({
        "name": "N++",
        "version": -1.25e+3,
        "active": true,
        "nothing": null,
        "nested": {"version": 99},
        "array": [1, "two", {"ok": true}, null],
        "quoted": "a \"key\": b",
        "escaped": "line\nnext",
        "unicode": "\uD83D\uDE42",
        "a\"b": 7
    })json";

    expect(npp_json_valid(json), "valid JSON object");
    expect(npp_json_valid(" [true, null, -0.5e2] "), "valid non-object JSON");
    expect(npp_json_valid("null"), "valid JSON null");
    expect(!npp_json_valid("{\"n\":01}"), "leading zero is rejected");
    expect(!npp_json_valid("{\"n\":1,}"), "trailing object comma is rejected");
    expect(!npp_json_valid("[1,]"), "trailing array comma is rejected");
    expect(!npp_json_valid("true false"), "trailing non-whitespace is rejected");
    expect(!npp_json_valid(R"json({"bad":"\uD800"})json"), "unpaired high surrogate is rejected");

    std::string tooDeep(129, '[');
    tooDeep += "0";
    tooDeep.append(129, ']');
    expect(!npp_json_valid(tooDeep.c_str()), "excessive nesting is rejected");

    std::string tooManyValues = "[";
    for (int i = 0; i < 100001; ++i) {
        if (i != 0)
            tooManyValues += ',';
        tooManyValues += '0';
    }
    tooManyValues += ']';
    expect(!npp_json_valid(tooManyValues.c_str()), "excessive node count is rejected");

    expectString("top-level key", npp_json_get(json, "version"), "-1.25e+3");
    expectString("nested key is not mistaken for a root key",
                 npp_json_get(R"json({"nested":{"version":99}})json", "version"),
                 "");
    expectString("key-like text inside a string is ignored",
                 npp_json_get(R"json({"note":"\"version\":99","version":42})json", "version"),
                 "42");
    expectString("string escapes are decoded", npp_json_get(json, "quoted"), "a \"key\": b");
    expectString("newline escape is decoded", npp_json_get(json, "escaped"), "line\nnext");
    expectString("UTF-16 surrogate pair becomes UTF-8",
                 npp_json_get(json, "unicode"),
                 "\xF0\x9F\x99\x82");
    expectString("escaped object key is decoded",
                 npp_json_get(R"json({"a\"b":7})json", "a\"b"),
                 "7");
    expectString("booleans retain JSON text", npp_json_get(json, "active"), "true");
    expectString("null retains JSON text", npp_json_get(json, "nothing"), "null");
    expectString("compound value returns its JSON source",
                 npp_json_get(json, "array"),
                 "[1, \"two\", {\"ok\": true}, null]");
    expectString("missing key returns empty string", npp_json_get(json, "absent"), "");
    expectString("malformed input does not return a false match",
                 npp_json_get(R"json({"version": 42,)json", "version"),
                 "");

    expectString("string type", npp_json_type(json, "name"), "string");
    expectString("number type", npp_json_type(json, "version"), "number");
    expectString("boolean type", npp_json_type(json, "active"), "boolean");
    expectString("null type", npp_json_type(json, "nothing"), "null");
    expectString("object type", npp_json_type(json, "nested"), "object");
    expectString("array type", npp_json_type(json, "array"), "array");
    expectString("missing type", npp_json_type(json, "absent"), "missing");
    expectString("invalid type", npp_json_type("{broken", "x"), "invalid");
    expectString("non-object root has no properties", npp_json_type("[]", "x"), "missing");

    expect(npp_json_array_len(json, "array") == 4, "array length");
    expect(npp_json_array_len(json, "nested") == -1, "non-array length sentinel");
    expect(npp_json_array_len(json, "absent") == -1, "missing array sentinel");
    expectString("array string element is decoded", npp_json_array_get(json, "array", 1), "two");
    expectString("array object element returns JSON",
                 npp_json_array_get(json, "array", 2),
                 "{\"ok\": true}");
    expectString("array null element", npp_json_array_get(json, "array", 3), "null");
    expectString("negative array index", npp_json_array_get(json, "array", -1), "");
    expectString("out-of-range array index", npp_json_array_get(json, "array", 4), "");
    expectString("invalid array input", npp_json_array_get("{broken", "array", 0), "");

    if (failures != 0) {
        std::fprintf(stderr, "%d JSON runtime test(s) failed\n", failures);
        return 1;
    }
    std::puts("N++ JSON runtime tests: PASS");
    return 0;
}
