#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include "../include/dispatcher.hpp"
#include "../include/transferstruct.hpp"
#include "../include/string.hpp"
#include "../include/accounts.hpp"

using namespace eosio;
using namespace std;

CONTRACT itamitamitam : contract
{
    public:
        using contract::contract;

        ACTION modbalance(name owner, asset quantity);
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

        struct paymentMemo
        {
            string category;
            string owner;
            string ownerGroup;
            string appId;
            string itemId;
        };

        struct dexMemo
        {
            string buyerNickname;
            string symbolName;
            string itemId;
            string owner;
        };

        void add_balance(const name& owner, const asset& quantity);
        void sub_balance(const name& owner, const asset& quantity);
};

ALLOW_TRANSFER_ITAM_EOS_DISPATCHER( itamitamitam, (transferto)(modbalance), &itamitamitam::transfer )