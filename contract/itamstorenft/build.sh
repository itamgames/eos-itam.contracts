#!/bin/sh

eosio-cpp -abigen -D="PROD" itamstorenft.cpp -o itamstorenft-prod.wasm
eosio-cpp -abigen -D="PROD_BETA" itamstorenft.cpp -o itamstorenft-prod-beta.wasm
eosio-cpp -abigen -D="DEV" itamstorenft.cpp -o itamstorenft-dev.wasm
eosio-cpp -abigen -D="DEV_BETA" itamstorenft.cpp -o itamstorenft-dev-beta.wasm