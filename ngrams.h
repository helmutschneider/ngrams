#ifndef _NGRAMS_H_
#define _NGRAMS_H_

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define ASSERT_MSG(value, ...)           \
  if (!(value)) {                        \
    printf("BADLY AT: L%d\n", __LINE__); \
    printf(__VA_ARGS__);                 \
    return 1;                            \
  }

#define ASSERT(value) ASSERT_MSG(value, " ")

#if defined(DEBUG) || 0
  #define DEBUG 1
#endif

#if DEBUG
#define DEBUG_PRINT(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

static int read_file(const char *filename, char *out) {
  FILE *handle = fopen(filename, "rb");
  ASSERT(handle != NULL);

  size_t offset = 0;

  while (true) {
    size_t read = fread(out + offset, sizeof(char), 8192, handle);
    if (read == 0) {
      break;
    }
    offset += read;
  }

  out[offset] = '\0';
  fclose(handle);

  DEBUG_PRINT("%s\n", out);

  return 0;
}

static size_t trim(const char *str, char *out) {
  size_t len = strlen(str);

  if (len == 0) {
    return 0;
  }

  int start = -1;
  int end = 0;

  for (size_t i = 0; i < len; ++i) {
    char ch = str[i];
    if (!isspace(ch) && start == -1) {
      start = i;
    }
    if (start != -1 && (ch == '\0' || isspace(ch))) {
      break;
    }
    end++;
  }

  if (end == start) {
    return 0;
  }

  assert(start != -1);
  assert(end != -1);

  size_t trimmed_len = end - start;
  memcpy(out, str + start, trimmed_len);
  out[trimmed_len] = '\0';

  return trimmed_len;
}

static double elapsed_sec(struct timespec since) {
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);

  double ts = ((double)since.tv_sec) + ((double)since.tv_nsec) / 1e9;
  double tend = ((double)now.tv_sec) + ((double)now.tv_nsec) / 1e9;

  return tend - ts;
}

#endif