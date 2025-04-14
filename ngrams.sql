with recursive make_ngrams as (
  select w.word_id,
         lower(w.value) as remainder,
         ? as ngram_len,
         '' as value,
         -1 as sequence_no
   from word w
  union all
  select s.word_id,
         substr(s.remainder, 2),
         s.ngram_len,
         substr(s.remainder, 1, s.ngram_len),
         s.sequence_no + 1
    from make_ngrams s
   where length(s.remainder) >= s.ngram_len
)
insert into "ngram" ("word_id", "value", "sequence_no")
select m.word_id, m.value, m.sequence_no
  from make_ngrams m
 where m.value != '' -- the base case is empty.
 order by m.sequence_no
