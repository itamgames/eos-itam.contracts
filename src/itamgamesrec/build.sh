#!/bin/sh

COMPILE_OPTION=""
if [ "$1" = "mainnet" ]; then
    COMPILE_OPTION="MAINNET"
elif [ "$1" = "mainnet-beta" ]; then
    COMPILE_OPTION="MAINNET_BETA"
elif [ "$1" = "testnet" ]; then
    COMPILE_OPTION="TESTNET"
elif [ "$1" = "testnet-beta" ]; then
    COMPILE_OPTION="TESTNET_BETA"
else
    echo "require option: \"mainnet\", \"mainnet-beta\", \"testnet\", \"testnet-beta\""
    exit 1
fi

eosio-cpp -abigen -D="$COMPILE_OPTION" itamgamesrec.cpp -o itamgamesrec.wasm