#include <eosiolib/eosio.hpp>

const char* CENTRAL_SETTLE_ACCOUNT = "itamgamestle";
const char* ITAM_SETTLE_ACCOUNT = "itamstincome";

struct settle
{
    uint64_t appId;
    name settleAccount;
    asset settleAmount;

    uint64_t primary_key() const { return appId; }
};
typedef multi_index<name("settles"), settle> settle_table;