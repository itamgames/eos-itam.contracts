#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>
#include "../include/dispatcher.hpp"

using namespace eosio;
using namespace std;

CONTRACT itamtokenadm : public contract
{
    public:
        using contract::contract;

        ACTION create(name issuer, asset maximum_supply);
        ACTION issue(name to, asset quantity, string memo);
        ACTION transfer(name from, name to, asset amount, string memo);
        ACTION staking(name owner, asset value);
        ACTION unstaking(name owner, asset value);
        ACTION burn(asset quantity, string memo);
        ACTION mint(name owner, asset quantity, string memo);
        ACTION locktoken(name owner, asset quantity, uint64_t timestamp_sec);
        ACTION releasetoken(name owner, string symbol_name);
        ACTION defrefund(name owner);
        ACTION menrefund(name owner);

    private:
        const uint64_t SEC_REFUND_DELAY = 10;

        TABLE account
        {
            asset balance;

            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };
        typedef multi_index<name("accounts"), account> account_table;

        TABLE currency
        {
            name issuer;
            asset supply;
            asset max_supply;

            uint64_t primary_key() const { return supply.symbol.code().raw(); }
        };
        typedef multi_index<name("currencies"), currency> currency_table;

        TABLE stake
        {
            name owner;
            asset balance;

            uint64_t primary_key() const { return owner.value; }
        };
        typedef multi_index<name("stakes"), stake> stake_table;

        struct refund_info
        {
            asset refund_amount;
            uint64_t req_refund_ts;
        };

        TABLE refund
        {
            name owner;
            asset total_refund;
            std::vector<refund_info> refund_list;

            uint64_t primary_key() const { return owner.value; }
        };
        typedef multi_index<name("refunds"), refund> refund_table;

        struct lock_info
        {
            asset quantity;
            uint64_t timestamp;
        };

        TABLE lock
        {
            name owner;
            vector<lock_info> lock_infos;

            uint64_t primary_key() const { return owner.value; }
        };
        typedef multi_index<name("locks"), lock> lock_table;

        void sub_balance(name owner, asset value);
        void add_balance(name owner, asset value, name ram_payer);
        void refund(name owner);
};

EOSIO_DISPATCH( itamtokenadm, (create)(issue)(transfer)(staking)(unstaking)(burn)(defrefund)(menrefund)(locktoken)(releasetoken) )