#include <cstring>
#include "Base32.h"

// Base32 alphabet (with padding character '=')
const int alphabet_size = 32;
const char base32_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

// Convert a Base32 character to its 5-bit integer value
int base32_char_to_value(char c) {
  for (int i = 0; i < alphabet_size; ++i) {
    if (base32_alphabet[i] == c) {
      return i;
    }
  }
  return -1; // Invalid character
}

// Decode a Base32 string
int decode_base32(const char* encoded, uint8_t* decoded) {
  int encoded_length = strlen(encoded);
  int decoded_index = 0;
  uint32_t buffer = 0;
  int bits_left = 0;

  for (int i = 0; i < encoded_length; ++i) {
    char c = encoded[i];
    if (c == '=') {
        break; // Stop decoding at padding character '='
    }

    int value = base32_char_to_value(c);
    if (value == -1) {
        continue; // Skip invalid characters
    }

    buffer = (buffer << 5) | value; // Shift buffer and add the 5 bits
    bits_left += 5;

    // If there are 8 or more bits in the buffer, output a byte
    if (bits_left >= 8) {
      decoded[decoded_index++] = (buffer >> (bits_left - 8)) & 0xFF;
      bits_left -= 8;
    }
  }

  return decoded_index; // Return the number of decoded bytes
}
