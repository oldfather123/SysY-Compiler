# SysYCC 编译青春

## Build
```shell
cargo build -r --bin sysy-frontend

or

cargo build -r --bin compiler
```

## Run
```shell
./target/release/sysy-frontend test/functional/00_main.sy -o output/functional/00_main.ll

or

./target/release/compiler test/functional/00_main.sy -S -o output/functional/00_main.s
```
