#ifndef BASE32_H
#define BASE32_H

#include <stdint.h>

int decode_base32(const char* encoded, uint8_t* decoded);

#endif // BASE32_H
