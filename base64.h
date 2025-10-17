#ifndef BASE64_H
#define BASE64_H

#include <stddef.h> // for size_t

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char BYTE;

// Encodes input bytes into a Base64 string.
// Returns a newly allocated string — caller must free().
char *base64_encode(const BYTE *buf, size_t len);

// Decodes a Base64 string into bytes.
// Returns a newly allocated buffer — caller must free().
// The output length will be stored in *out_len.
BYTE *base64_decode(const char *str, size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif // BASE64_H
