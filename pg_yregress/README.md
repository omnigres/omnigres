# pg_yregress

`pg_yregress` is a regression testing tool for Postgres.

It is used internally for Omnigres and can also be built independently of Omnigres:


```shell
cd pg_yregress
cmake -B build
cmake --build build --parallel
```

This build will be using user- or system-installed Postgres.

You can read more about this tool in its [documentation](https://docs.omnigres.org/pg_yregress/usage).
