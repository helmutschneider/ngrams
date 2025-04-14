#include <stdio.h>
#include <stdbool.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define ASSERT(value)                    \
  if (!(value))                          \
  {                                      \
    printf("BADLY AT: L%d\n", __LINE__); \
    return 1;                            \
  }
#define DEBUG 0

#if DEBUG
#define DEBUG_PRINT(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

static int read_file(const char *filename, char *out)
{
  FILE *handle = fopen(filename, "rb");
  ASSERT(handle != NULL);

  size_t offset = 0;

  while (true)
  {
    size_t read = fread(out + offset, sizeof(char), 8192, handle);
    if (read == 0)
    {
      break;
    }
    offset += read;
  }

  out[offset] = '\0';
  fclose(handle);

  return 0;
}

static size_t trim(const char *str, size_t len, char *out)
{
  size_t start = 0;
  for (; start < len; ++start)
  {
    if (!isspace(str[start]))
    {
      break;
    }
  }

  size_t trimmed_len = 0;

  for (size_t i = 0; i < len; ++i)
  {
    if (isspace(str[i + start]))
    {
      out[i + start] = '\0';
      return trimmed_len;
    }
    out[i] = str[i + start];
    trimmed_len += 1;
  }

  return trimmed_len;
}

static int insert_words(sqlite3 *db, const char *filename, const char *language)
{
  FILE *handle = fopen(filename, "rb");
  ASSERT(handle != NULL);

  sqlite3_stmt *word_stmt;
  int ok;

  ok = sqlite3_prepare_v2(db, "insert into \"word\" (\"language\", \"value\") values (?, ?)", -1, &word_stmt, NULL);
  ASSERT(ok == SQLITE_OK);

  while (true)
  {
    char *line = NULL;
    size_t line_len;
    ssize_t ok = getline(&line, &line_len, handle);
    if (ok == -1)
    {
      break;
    }

    char word[512];
    size_t word_len = trim(line, line_len, word);
    if (word_len == 0)
    {
      continue;
    }

    DEBUG_PRINT("word: %s\n", word);
    ok = sqlite3_bind_text(word_stmt, 1, language, -1, NULL);
    ASSERT(ok == 0);
    ok = sqlite3_bind_text(word_stmt, 2, word, -1, NULL);
    ASSERT(ok == 0);
    ok = sqlite3_step(word_stmt);
    ASSERT(ok == SQLITE_OK || ok == SQLITE_DONE);
    ok = sqlite3_reset(word_stmt);
    ASSERT(ok == 0);

    sqlite3_int64 word_id = sqlite3_last_insert_rowid(db);
    ASSERT(word_id != 0);

    DEBUG_PRINT("word_id: %lld\n", word_id);
  }

  sqlite3_finalize(word_stmt);
  fclose(handle);

  return 0;
}

static int insert_ngrams(sqlite3 *db, const char *filename, size_t ngram_len)
{
  char buffer[8192];
  int ok;
  ok = read_file(filename, buffer);
  ASSERT(ok == 0);
  DEBUG_PRINT("%s\n", buffer);

  sqlite3_stmt *stmt;
  ok = sqlite3_prepare_v2(db, buffer, -1, &stmt, NULL);
  ASSERT(ok == SQLITE_OK);

  ok = sqlite3_bind_int(stmt, 1, ngram_len);
  ASSERT(ok == SQLITE_OK);

  ok = sqlite3_step(stmt);
  ASSERT(ok == SQLITE_OK || ok == SQLITE_DONE);

  sqlite3_finalize(stmt);

  return 0;
}

static int create_or_open_db(sqlite3 **db, const char *filename, size_t ngram_len)
{
  int ok;

  ok = sqlite3_open_v2(filename, db, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, NULL);
  ASSERT(ok == SQLITE_OK);

  ok = sqlite3_exec(*db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
  ASSERT(ok == SQLITE_OK);

  sqlite3_stmt *exists_stmt;
  ok = sqlite3_prepare_v2(*db, "select 1 from sqlite_schema where type = 'table' and name = ?", -1, &exists_stmt, NULL);
  ASSERT(ok == SQLITE_OK);

  ok = sqlite3_bind_text(exists_stmt, 1, "word", -1, NULL);
  ASSERT(ok == SQLITE_OK);

  ok = sqlite3_step(exists_stmt);
  bool exists = ok == SQLITE_ROW;
  sqlite3_finalize(exists_stmt);

  if (!exists)
  {
    char buffer[8192];
    ok = read_file("schema.sql", buffer);
    ASSERT(ok == 0);
    ok = sqlite3_exec(*db, buffer, NULL, NULL, NULL);
    ASSERT(ok == SQLITE_OK);
    ok = sqlite3_exec(*db, "begin transaction", NULL, NULL, NULL);
    ASSERT(ok == SQLITE_OK);
    ok = insert_words(*db, "ss100.txt", "sv-SE");
    ASSERT(ok == 0);
    ok = insert_ngrams(*db, "ngrams.sql", ngram_len);
    ASSERT(ok == 0);
    ok = sqlite3_exec(*db, "commit", NULL, NULL, NULL);
    ASSERT(ok == SQLITE_OK);
  }

  return 0;
}

static double elapsed_sec(struct timespec since)
{
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);

  double ts = ((double)since.tv_sec) + ((double)since.tv_nsec) / 1e9;
  double tend = ((double)now.tv_sec) + ((double)now.tv_nsec) / 1e9;

  return tend - ts;
}

static const int NGRAM_LEN = 3;

static int search_ngram(sqlite3 *db, const char *term)
{
  int ok;
  char buffer[8192];
  ok = read_file("search.sql", buffer);
  ASSERT(ok == 0);

  sqlite3_stmt *search_stmt;
  ok = sqlite3_prepare_v2(db, buffer, -1, &search_stmt, NULL);
  ASSERT(ok == SQLITE_OK);
  ok = sqlite3_bind_text(search_stmt, 1, term, -1, NULL);
  ASSERT(ok == SQLITE_OK);
  ok = sqlite3_bind_int(search_stmt, 2, NGRAM_LEN);
  ASSERT(ok == SQLITE_OK);

  while (sqlite3_step(search_stmt) == SQLITE_ROW)
  {
    const unsigned char* word = sqlite3_column_text(search_stmt, 0);
    printf("%s\n", word);
  }

  sqlite3_finalize(search_stmt);

  return 0;
}

static int search_like(sqlite3 *db, const char *term)
{
  int ok;

  sqlite3_stmt *search_stmt;
  ok = sqlite3_prepare_v2(db, "select w.value from word w where value like '%' || ? || '%' order by w.value", -1, &search_stmt, NULL);
  ASSERT(ok == SQLITE_OK);
  ok = sqlite3_bind_text(search_stmt, 1, term, -1, NULL);
  ASSERT(ok == SQLITE_OK);

  while (sqlite3_step(search_stmt) == SQLITE_ROW)
  {
    const unsigned char* word = sqlite3_column_text(search_stmt, 0);
    printf("%s\n", word);
  }
  sqlite3_finalize(search_stmt);

  return 0;
}

int main(int argc, const char **argv)
{
  if (argc < 2)
  {
    printf("Usage: ./app [search_term]\n");
    return 1;
  }

  int ok;
  sqlite3 *db;
  ok = create_or_open_db(&db, "words.db", NGRAM_LEN);
  ASSERT(ok == 0);

  struct timespec ts;

  printf("Searching (ngram)...\n");
  clock_gettime(CLOCK_REALTIME, &ts);
  search_ngram(db, argv[1]);
  printf("Elapsed: %.4f sec\n", elapsed_sec(ts));
  printf("\n");

  printf("Searching (like)...\n");
  clock_gettime(CLOCK_REALTIME, &ts);
  search_like(db, argv[1]);
  printf("Elapsed: %.4f sec\n", elapsed_sec(ts));

  sqlite3_close_v2(db);
  return 0;
}
