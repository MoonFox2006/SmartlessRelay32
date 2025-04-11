#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

size_t percent_encoded_length(const char *str, size_t len);
void percent_encode(char *out, const char *str, size_t len);

size_t percent_decoded_length(const char *str, size_t len);
void percent_decode(char *out, const char *str, size_t len);

#ifdef __cplusplus
}
#endif
