# Installation

`pg_yregess` is currently [developed as part of Omnigres](https://github.com/omnigres/omnigres/tree/master/pg_yregress) but can be easily built independently of Omnigres ("_out of
tree_").

```shell
git clone https://github.com/omnigres/omnigres
cd omnigres/pg_yregress
cmake -B build && cmake --build build --parallel
# If you want to install it, too:
sudo cmake --install build
```
