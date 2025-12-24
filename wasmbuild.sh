#! /bin/sh

mkdir -p public/wasm
clang --target=wasm32 \
    -O3 \
    -flto \
    -nostdlib \
    -Wl,--no-entry \
    -Wl,--export-all \
    -Wl,--allow-undefined \
    -Wl,--initial-memory=67239936 \
    -Wl,--max-memory=536870912 \
    -o public/wasm/totp_search.wasm src/wasm/totp_search.c
