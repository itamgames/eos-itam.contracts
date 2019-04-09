#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>
#include "../include/dispatcher.hpp"
#include "../include/date.hpp"

using namespace eosio;
using namespace std;

CONTRACT itamtokenadm : public contract
{
    public:
        using contract::contract;

        ACTION create(name issuer, asset maximum_supply);
        ACTION issue(name to, asset quantity, name lock_type, string memo);
        ACTION transfer(name from, name to, asset quantity, string memo);
        ACTION burn(asset quantity, string memo);
        ACTION mint(asset quantity, string memo);
        ACTION setlockinfo(name lock_type, uint64_t start_timestamp_sec, vector<int32_t> percents);
        ACTION dellockinfo(name lock_type);
        ACTION regblacklist(name owner);
        ACTION delblacklist(name owner);

    private:

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

        TABLE lock
        {
            name owner;
            asset quantity;
            name lock_type;

            uint64_t primary_key() const { return owner.value; }
        };
        typedef multi_index<name("locks"), lock> lock_table;

        TABLE lockinfo
        {
            name lock_type;
            uint64_t start_timestamp_sec;
            vector<int32_t> percents;

            uint64_t primary_key() const { return lock_type.value; }
        };
        typedef multi_index<name("lockinfos"), lockinfo> lockinfo_table;

        TABLE blacklist
        {
            name owner;

            uint64_t primary_key() const { return owner.value; }
        };
        typedef multi_index<name("blacklists"), blacklist> blacklist_table;

        void sub_balance(name owner, asset value);
        void add_balance(name owner, asset value, name ram_payer);
        void change_max_supply(const asset& quantity, const string& memo);
};

EOSIO_DISPATCH( itamtokenadm, (create)(issue)(transfer)(burn)(mint)(setlockinfo)(dellockinfo)(regblacklist)(delblacklist) )