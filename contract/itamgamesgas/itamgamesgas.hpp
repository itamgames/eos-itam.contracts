#include <eosiolib/eosio.hpp>
#include "../include/dispatcher.hpp"
#include "../include/string.hpp"
#include "../include/transferstruct.hpp"

using namespace eosio;

CONTRACT itamgamesgas : contract
{
    public:
        using contract::contract;

        ACTION setsettle(string appId, name account);
        ACTION claimsettle(string appId);
        ACTION transfer(uint64_t from, uint64_t to);
    private:
        TABLE settle
        {
            uint64_t appId;
            name settleAccount;
            asset settleAmount;

            uint64_t primary_key() const { return appId; }
        };
        typedef multi_index<name("settles"), settle> settleTable;

        TABLE ownergroup
        {
            name groupName;
            name account;

            uint64_t primary_key() const { return groupName.value; }
        };
        typedef multi_index<name("ownergroups"), ownergroup> ownerGroupTable;

        struct memoData
        {
            string appId;  
        };
};

EOSIO_DISPATCH_EX( itamgamesgas, (setsettle)(claimsettle)(transfer) )