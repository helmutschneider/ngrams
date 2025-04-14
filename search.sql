with needle_ngrams as (
  select '' as value,
         lower(?) as remainder,
         -1 as seq_no,
         ? as ngram_len
  union all
  select substr(tn.remainder, 1, tn.ngram_len),
         substr(tn.remainder, 2),
         tn.seq_no + 1,
         tn.ngram_len
    from needle_ngrams tn
   where length(tn.remainder) >= tn.ngram_len
), matches as (
  select n.word_id,
         n.seq_no,
         tn.seq_no as needle_seq_no,
         (select max(seq_no) from needle_ngrams) as last_needle_seq_no
    from ngram n
    join needle_ngrams tn
      on n.value = tn.value
     and tn.seq_no = 0
   union all
  select n.word_id,
         n.seq_no,
         tn.seq_no,
         m.last_needle_seq_no
    from ngram n
    join matches m
      on n.word_id = m.word_id
     and n.seq_no = (m.seq_no + 1)
    join needle_ngrams tn
      on tn.seq_no = (m.needle_seq_no + 1)
   where tn.value = n.value
)
select w.value
 from matches m
 join word w
   on w.word_id = m.word_id
where m.needle_seq_no = m.last_needle_seq_no
