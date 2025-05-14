#include "ngrams.h"
#include <assert.h>

#define TEST(name) \
  do { \
    printf(#name "... "); \
    int res = name(); \
    printf(res ? "FAIL\n" : "OK\n"); \
  } while (0);

static int trim_with_newline() {
  char a[] = "\r\ncowabunga\r\n";
  char b[512];
  int len = trim(a, b);
  assert(len == 9);
  assert(strcmp(b, "cowabunga") == 0);
  return 0;
}

static int trim_with_null() {
  char a[] = "\r\ncowabunga";
  char b[512];
  int len = trim(a, b);
  assert(len == 9);
  assert(strcmp(b, "cowabunga") == 0);
  return 0;
}

int main() {
  printf("Running tests...\n");
  TEST(trim_with_newline);
  TEST(trim_with_null);

  return 0;
}
