with recursive make_ngrams as (
  select '' as value,
         lower(w.value) as remainder,
         w.word_id,
         ? as ngram_len,
         -1 as seq_no
   from word w
  union all
  select substr(s.remainder, 1, s.ngram_len),
         substr(s.remainder, 2),
         s.word_id,
         s.ngram_len,
         s.seq_no + 1
    from make_ngrams s
   where length(s.remainder) >= s.ngram_len
)
insert into "ngram" ("word_id", "value", "seq_no")
select m.word_id, m.value, m.seq_no
  from make_ngrams m
 where m.value != '' -- the base case is empty.
