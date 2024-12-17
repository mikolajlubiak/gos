#include "ctype.h"

bool isLower(char chr) { return chr >= 'a' && chr <= 'z'; }

bool isUpper(char chr) { return !isLower(chr); }

char toUpper(char chr) {
  if (isLower(chr)) {
    return chr + 'A' - 'a';
  }

  return chr;
}

char toLower(char chr) {
  if (isUpper(chr)) {
    return chr + 'a' - 'A';
  }

  return chr;
}
