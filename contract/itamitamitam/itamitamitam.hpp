#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include "../include/ownergroup.hpp"
#include "../include/dispatcher.hpp"
#include "../include/transferstruct.hpp"
#include "../include/string.hpp"

using namespace std;
using namespace eosio;

CONTRACT itamitamitam : contract
{
    public:
        using contract::contract;

        ACTION transfer(uint64_t from, uint64_t to);
        ACTION transferto(name from, name to, asset quantity, string memo);
    private:
        TABLE account
        {
            name owner;
            asset balance;

            uint64_t primary_key() const { return owner.value; }
        };
        typedef multi_index<name("accounts"), account> accountTable;

        struct memoData
        {
            string owner;
            string ownerGroup;
        };
};

EOSIO_DISPATCH_EX( itamitamitam, (transfer)(transferto) )