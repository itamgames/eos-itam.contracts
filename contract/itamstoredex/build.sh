#!/bin/sh

eosio-cpp -abigen itamstoredex.cpp -o itamstoredex-prod.wasm
eosio-cpp -abigen -D="BETA" itamstoredex.cpp -o itamstoredex-beta.wasm