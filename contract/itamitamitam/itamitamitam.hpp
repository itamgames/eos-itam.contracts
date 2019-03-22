#include <eosiolib/eosio.hpp>
#include "../include/dispatcher.hpp"

using namespace eosio;
using namespace std;

CONTRACT itamitamitam : contract
{
    public:
        using contract::contract;

        ACTION addgroup(name owner, name group_account);
        ACTION modifygroup(name owner, name group_account);
    private:
        TABLE ownergroup
        {
            name owner;
            name account;

            uint64_t primary_key() const { return owner.value; }
        };
        typedef multi_index<name("ownergroups"), ownergroup> ownergroup_table;
};

EOSIO_DISPATCH_EX(itamitamitam, (addgroup)(modifygroup))