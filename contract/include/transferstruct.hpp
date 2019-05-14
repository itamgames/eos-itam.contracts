#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

struct transfer_data
{
    eosio::name from;
    eosio::name to;
    eosio::asset quantity;
    std::string memo;
};