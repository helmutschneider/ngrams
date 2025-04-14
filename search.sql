with recursive term_ngrams as (
    select '' as value,
           lower(?) as remainder,
           ? as ngram_len,
           -1 as seq_no
      union all
    select substr(tn.remainder, 1, tn.ngram_len),
           substr(tn.remainder, 2),
           tn.ngram_len,
           tn.seq_no + 1
      from term_ngrams tn
     where length(tn.remainder) >= tn.ngram_len
), first_ngram as (
  select n.*
    from ngram n
    join term_ngrams ts
      on n.value = ts.value
     and ts.seq_no = 0
), matches as (
    select n.word_id
  from ngram n
  join first_ngram tf
    on n.word_id = tf.word_id
  join term_ngrams ts
    on n.seq_no = (tf.seq_no + ts.seq_no)
   and n.value = ts.value
 group by n.word_id
having count(1) = ((select max(seq_no) from term_ngrams) + 1)
)
select w.value
  from word w
  join matches m
    on m.word_id = w.word_id
order by w.value
