with recursive make_ngrams as (
  select lower(?) as remainder,
         ? as ngram_len,
         '' as value,
         -1 as sequence_no
  union all
  select substr(s.remainder, 2),
         s.ngram_len,
         substr(s.remainder, 1, s.ngram_len),
         s.sequence_no + 1
    from make_ngrams s
   where length(s.remainder) >= s.ngram_len
), t_matches as (
  select n.*,
         m.sequence_no as iter_sequence_no,
         (select max(sequence_no) from make_ngrams) as last_sequence_no
    from make_ngrams m
    join ngram n
      on m.value = n.value
     and m.sequence_no = 0
   union all
  select n.*,
         m.sequence_no,
         t.last_sequence_no
    from ngram n
    join t_matches t
      on n.word_id = t.word_id
     and n.sequence_no = t.sequence_no + 1
    join make_ngrams m
      on m.sequence_no = t.iter_sequence_no + 1
     and m.value = n.value
)
select w.value
  from t_matches
  join word w
    on w.word_id = t_matches.word_id
 where t_matches.iter_sequence_no = t_matches.last_sequence_no
 order by w.value