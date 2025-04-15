An ngram-index search implemented in SQLite.

The search algorithm is a recursive CTE that matches ngrams from the search term with the indexed word. See [search.sql](search.sql) for details.

Swedish & English wordlists are included.

## Usage
```shell
make && ./app
```

A simple benchmark vs 'LIKE' is included:
```shell
./app bench [term_here]
./app bench adjacent

# Macbook Pro M1 Pro
# Searching 'adjacent', Iters: 1000, Algo: ngram
# Elapsed: 0.6218 sec
# Searching 'adjacent', Iters: 1000, Algo: LIKE
# Elapsed: 49.6461 sec
```
