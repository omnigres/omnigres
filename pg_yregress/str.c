#include <ctype.h>
#include <stddef.h>

void trim_trailing_whitespace(char *str) {
  if (str == NULL || *str == '\0') {
    return;
  }

  char *end = NULL;
  char *ptr = str;

  while (*ptr) {
    if (!isspace((unsigned char)*ptr)) {
      end = ptr;
    }
    ptr++;
  }

  if (end) {
    *(end + 1) = '\0';
  } else {
    *str = '\0';
  }
}
