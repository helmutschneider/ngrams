An ngram-index search implemented in SQLite.

Usually not faster than a regular 'LIKE', but it works. The search algorithm is a recursive CTE that matches ngrams from the search term with the indexed word. See [search.sql](search.sql) for details.

Swedish & English wordlists are included.

## Usage

```shell
make
./app
```
