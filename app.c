#include <stdio.h>
#include <stdbool.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include "ngrams.h"

static int insert_words(sqlite3 *db, const char *filename, const char *language) {
  FILE *handle = fopen(filename, "rb");
  ASSERT(handle != NULL);

  sqlite3_stmt *word_stmt;
  int ok;

  ok = sqlite3_prepare_v2(db, "insert into \"word\" (\"language\", \"value\") values (?, ?)", -1, &word_stmt, NULL);
  ASSERT(ok == SQLITE_OK);

  while (true) {
    char *line = NULL;
    size_t line_len;
    ssize_t ok = getline(&line, &line_len, handle);
    if (ok == -1) {
      break;
    }

    char word[512];
    size_t word_len = trim(line, word);
    if (word_len == 0) {
      continue;
    }

    DEBUG_PRINT("word: %s, len = %zu\n", word, word_len);
    ok = sqlite3_bind_text(word_stmt, 1, language, -1, NULL);
    ASSERT(ok == 0);
    ok = sqlite3_bind_text(word_stmt, 2, word, -1, NULL);
    ASSERT(ok == 0);
    ok = sqlite3_step(word_stmt);
    ASSERT(ok == SQLITE_OK || ok == SQLITE_DONE);
    ok = sqlite3_reset(word_stmt);
    ASSERT(ok == 0);
  }

  sqlite3_finalize(word_stmt);
  fclose(handle);

  return 0;
}

static int insert_ngrams(sqlite3 *db, const char *filename, size_t ngram_len) {
  char buffer[8192];
  int ok;
  ok = read_file(filename, buffer);
  ASSERT(ok == 0);

  sqlite3_stmt *stmt;
  ok = sqlite3_prepare_v2(db, buffer, -1, &stmt, NULL);
  ASSERT_MSG(ok == SQLITE_OK, "%s\n", sqlite3_errmsg(db));

  ok = sqlite3_bind_int(stmt, 1, ngram_len);
  ASSERT(ok == SQLITE_OK);

  ok = sqlite3_step(stmt);
  ASSERT(ok == SQLITE_OK || ok == SQLITE_DONE);

  sqlite3_finalize(stmt);

  return 0;
}

static uint64_t ngram_hash(const char *ngram) {
  uint64_t hash = 0;
  size_t len = strnlen(ngram, 16);

  for (size_t i = 0; i < len; ++i) {
    uint8_t ch = ngram[i];
    size_t shl = (len - i - 1) * 8;
    hash |= (ch << shl);
  }

  return hash;
}

static void sqlite3_ngram_hash(sqlite3_context *ctx, int argc, sqlite3_value **args) {
  const unsigned char *ngram = sqlite3_value_text(args[0]);
  sqlite3_int64 hash = ngram_hash((const char *)ngram);

  sqlite3_result_int64(ctx, hash);
}

static int create_or_open_db(sqlite3 **db, const char *filename, size_t ngram_len) {
  int ok;

  ok = sqlite3_open_v2(filename, db, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, NULL);
  ASSERT(ok == SQLITE_OK);

  ok = sqlite3_create_function_v2(*db, "ngram_hash", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_DIRECTONLY, NULL, sqlite3_ngram_hash, NULL, NULL, NULL);
  ASSERT_MSG(ok == SQLITE_OK, "%s\n", sqlite3_errmsg(*db));

  ok = sqlite3_exec(*db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
  ASSERT(ok == SQLITE_OK);

  sqlite3_stmt *exists_stmt;
  ok = sqlite3_prepare_v2(*db, "select 1 from sqlite_schema where type = 'table' and name = 'word'", -1, &exists_stmt, NULL);
  ASSERT(ok == SQLITE_OK);

  ok = sqlite3_step(exists_stmt);
  bool exists = ok == SQLITE_ROW;
  sqlite3_finalize(exists_stmt);

  if (!exists) {
    printf("Building search index...\n");
    ok = sqlite3_exec(*db, "begin transaction", NULL, NULL, NULL);
    ASSERT(ok == SQLITE_OK);

    char buffer[8192];
    ok = read_file("schema.sql", buffer);
    ASSERT(ok == 0);
    ok = sqlite3_exec(*db, buffer, NULL, NULL, NULL);
    ASSERT_MSG(ok == SQLITE_OK, "%s\n", sqlite3_errmsg(*db));
    ok = insert_words(*db, "sv.txt", "sv-SE");
    ASSERT(ok == 0);
    ok = insert_words(*db, "en.txt", "en-US");
    ASSERT(ok == 0);
    ok = insert_ngrams(*db, "ngrams.sql", ngram_len);
    ASSERT(ok == 0);

    ok = sqlite3_exec(*db, "commit", NULL, NULL, NULL);
    ASSERT(ok == SQLITE_OK);
  }

  return 0;
}

static int search_ngram(sqlite3 *db, const char *term, size_t ngram_len, bool do_print) {
  int ok;
  char buffer[8192];
  ok = read_file("search.sql", buffer);
  ASSERT(ok == 0);

  sqlite3_stmt *search_stmt;
  ok = sqlite3_prepare_v2(db, buffer, -1, &search_stmt, NULL);
  ASSERT(ok == SQLITE_OK);
  ok = sqlite3_bind_text(search_stmt, 1, term, -1, NULL);
  ASSERT(ok == SQLITE_OK);
  ok = sqlite3_bind_int(search_stmt, 2, ngram_len);
  ASSERT(ok == SQLITE_OK);

  while (sqlite3_step(search_stmt) == SQLITE_ROW) {
    const unsigned char *word = sqlite3_column_text(search_stmt, 0);
    if (do_print) {
      printf("%s\n", word);
    }
  }

  sqlite3_finalize(search_stmt);

  return 0;
}

static int search_like(sqlite3 *db, const char *term, bool do_print) {
  int ok;

  sqlite3_stmt *search_stmt;
  ok = sqlite3_prepare_v2(db, "select w.value from word w where value like '%' || ? || '%'", -1, &search_stmt, NULL);
  ASSERT(ok == SQLITE_OK);
  ok = sqlite3_bind_text(search_stmt, 1, term, -1, NULL);
  ASSERT(ok == SQLITE_OK);

  while (sqlite3_step(search_stmt) == SQLITE_ROW) {
    const unsigned char *word = sqlite3_column_text(search_stmt, 0);
    if (do_print) {
      printf("%s\n", word);
    }
  }
  sqlite3_finalize(search_stmt);

  return 0;
}

static void benchmark(sqlite3 *db, const char *term, size_t ngram_len) {
  const size_t BENCHMARK_ITERS = 1000;
  struct timespec ts;
  
  printf("Term = '%s', Iters = %zu, Method = search_ngram\n", term, BENCHMARK_ITERS);
  clock_gettime(CLOCK_REALTIME, &ts);
  for (size_t i = 0; i < BENCHMARK_ITERS; ++i) {
    search_ngram(db, term, ngram_len, false);
  }
  printf("Elapsed: %.4f sec\n", elapsed_sec(ts));

  printf("Term = '%s', Iters = %zu, Method = search_like\n", term, BENCHMARK_ITERS);
  clock_gettime(CLOCK_REALTIME, &ts);
  for (size_t i = 0; i < BENCHMARK_ITERS; ++i) {
    search_like(db, term, false);
  }
  printf("Elapsed: %.4f sec\n", elapsed_sec(ts));
}

static const int NGRAM_LEN = 3;

int main(int argc, const char **argv) {
  int ok;
  sqlite3 *db;
  ok = create_or_open_db(&db, "words.db", NGRAM_LEN);
  ASSERT(ok == 0);

  if (argc == 3 && strncmp(argv[1], "b", 1) == 0) {
    benchmark(db, argv[2], NGRAM_LEN);
    return 1;
  }

  while (true) {
    printf("Search: ");
    char buffer[512];
    char *term = gets(buffer);
    if (term == NULL || strnlen(term, 512) == 0) {
      break;
    }

    search_ngram(db, term, NGRAM_LEN, true);
  }

  sqlite3_close_v2(db);
  return 0;
}
