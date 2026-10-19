#include "json.h"

#include <cstdio>
#include <cstring>

namespace app {

Json& Json::obj(const char* k) { return open(k, '{'); }
Json& Json::arr(const char* k) { return open(k, '['); }
Json& Json::end_obj() { return close('}'); }
Json& Json::end_arr() { return close(']'); }

Json& Json::str(const char* k, const char* v) {
    key(k);
    quote(v, std::strlen(v));
    return *this;
}

Json& Json::str_n(const char* k, const char* v, size_t n) {
    key(k);
    quote(v, n);
    return *this;
}

Json& Json::num(const char* k, double v, int decimals) {
    key(k);
    char b[32];
    std::snprintf(b, sizeof(b), "%.*f", decimals, v);
    s_ += b;
    return *this;
}

Json& Json::boolean(const char* k, bool v) {
    key(k);
    s_ += v ? "true" : "false";
    return *this;
}

Json& Json::null(const char* k) {
    key(k);
    s_ += "null";
    return *this;
}

Json& Json::raw(const char* k, const std::string& json) {
    key(k);
    s_ += json;
    return *this;
}

void Json::escape(std::string& out, const char* v, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        auto c = static_cast<unsigned char>(v[i]);
        if (c == '"' || c == '\\') {
            out += '\\';
            out += static_cast<char>(c);
        } else if (c == '\n') {
            out += "\\n";
        } else if (c < 0x20 || c == 0x7f) {
            char b[8];
            std::snprintf(b, sizeof(b), "\\u%04x", c);
            out += b;
        } else {
            out += static_cast<char>(c);
        }
    }
}

Json& Json::open(const char* k, char c) {
    key(k);
    s_ += c;
    first_[++depth_] = true;
    return *this;
}

Json& Json::close(char c) {
    s_ += c;
    --depth_;
    return *this;
}

void Json::key(const char* k) {
    if (depth_ >= 0) {
        if (!first_[depth_]) s_ += ',';
        first_[depth_] = false;
    }
    if (k) {
        quote(k, std::strlen(k));
        s_ += ':';
    }
}

void Json::quote(const char* v, size_t n) {
    s_ += '"';
    escape(s_, v, n);
    s_ += '"';
}

}  // namespace app
