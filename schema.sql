create table "word" (
  "word_id" integer primary key,
  "language" text not null,
  "value" text not null
);

create index ix_word_value
  on "word" ("value");

create table "ngram" (
  "ngram_id" integer primary key,
  "word_id" integer not null,
  "value" text not null,
  "sequence_no" integer not null,
  foreign key ("word_id") references "word"("word_id")
    on update cascade
    on delete cascade
);

create index ix_ngram_word_id
  on "ngram" ("word_id");

create index ix_ngram_value
  on "ngram" ("value");

create index ix_ngram_sequence_no
  on "ngram" ("sequence_no");
