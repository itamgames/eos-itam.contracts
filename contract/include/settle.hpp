#include <eosiolib/eosio.hpp>

const char* CENTRAL_SETTLE_ACCOUNT = "itamgamesgas";
const char* ITAM_SETTLE_ACCOUNT = "itamstincome";


struct settle
{
    uint64_t appId;
    eosio::name settleAccount;
    eosio::asset settleAmount;

    uint64_t primary_key() const { return appId; }
};
typedef eosio::multi_index<eosio::name("settles"), settle> settle_table;