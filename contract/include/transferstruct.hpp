#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

struct transferData
{
    eosio::name from;
    eosio::name to;
    eosio::asset quantity;
    std::string memo;
};