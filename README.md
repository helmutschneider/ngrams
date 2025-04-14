An ngram-index search implemented in SQLite. Slower than a regular 'LIKE' :) Swedish wordlist included.

```shell
make
./app bunga

Searching (ngram)...
bungalow
bungalowen
bungalowens
bungalower
bungalowerna
bungalowernas
bungalowers
bungalows
Elapsed: 0.5544 sec

Searching (like)...
bungalow
bungalowen
bungalowens
bungalower
bungalowerna
bungalowernas
bungalowers
bungalows
Elapsed: 0.0394 sec
```
