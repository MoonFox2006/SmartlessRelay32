#include <stdbool.h>
#include <string.h>
#include "percent.h"

static const char ESCAPE_CHARS[] = " !\"#$%&'()*+,/:;=?@[]";

static bool is_hex(char c) {
    return ((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'F')) || ((c >= 'a') && (c <= 'f'));
}

static unsigned char from_hex(char c) {
    return (c >= '0') && (c <= '9') ? c - '0' : (c >= 'A') && (c <= 'F') ? c - 'A' + 10 : (c >= 'a') && (c <= 'f') ? c - 'a' + 10 : 0xFF;
}

static char to_hex(unsigned char d) {
    return d > 9 ? 'A' + d - 10 : '0' + d;
}

size_t percent_encoded_length(const char *str, size_t len) {
    size_t result = 0;

    while (len && *str) {
        if (strchr(ESCAPE_CHARS, *str))
            result += 2; // %Xx
        ++result;
        ++str;
        --len;
    }
    return result;
}

void percent_encode(char *out, const char *str, size_t len) {
    while (len && *str) {
        if (strchr(ESCAPE_CHARS, *str)) {
            *out++ = '%';
            *out++ = to_hex(*str >> 4);
            *out++ = to_hex(*str & 0x0F);
        } else
            *out++ = *str;
        ++str;
        --len;
    }
    *out = '\0';
}

size_t percent_decoded_length(const char *str, size_t len) {
    size_t result = 0;

    while (len && *str) {
        if (*str == '%') {
            if (len < 3)
                break;
            if (! is_hex(*(++str)))
                break;
            if (! is_hex(*(++str)))
                break;
            len -= 2;
        }
        ++result;
        ++str;
        --len;
    }
    return result;
}

void percent_decode(char *out, const char *str, size_t len) {
    while (len && *str) {
        if (*str == '%') {
            unsigned char d;

            if (len < 3)
                break;
            len -= 2;
            d = from_hex(*(++str));
            if (d > 0x0F)
                break;
            *out = d << 4;
            d = from_hex(*(++str));
            if (d > 0x0F)
                break;
            *out |= d;
        } else if (*str == '+') {
            *out = ' ';
        } else
            *out = *str;
        ++out;
        ++str;
        --len;
    }
    *out = '\0';
}
