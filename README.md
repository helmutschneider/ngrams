An ngram-index search implemented in SQLite. Usually not faster than a regular 'LIKE', but it works.

Swedish wordlist included.

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
Elapsed: 0.0063 sec

Searching (like)...
bungalow
bungalowen
bungalowens
bungalower
bungalowerna
bungalowernas
bungalowers
bungalows
Elapsed: 0.0214 sec

```
