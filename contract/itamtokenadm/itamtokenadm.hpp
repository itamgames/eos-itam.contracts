#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>

using namespace eosio;
using namespace std;

CONTRACT itamtokenadm : public contract
{
    public:
        using contract::contract;

        ACTION create(name issuer, asset maximum_supply);
        ACTION issue(name to, asset quantity, name lock_type, string memo);
        ACTION transfer(name from, name to, asset quantity, string memo);
        ACTION burn(name owner, asset quantity, string memo);
        ACTION mint(asset quantity, string memo);
        ACTION setlocktype(name lock_type, uint64_t start_timestamp_sec, vector<int32_t> percents);
        ACTION dellocktype(name lock_type);
        ACTION regblacklist(name owner);
        ACTION delblacklist(name owner);

    private:

        TABLE account
        {
            asset balance;

            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };
        typedef multi_index<name("accounts"), account> account_table;

        TABLE currency_stats
        {
            name issuer;
            asset supply;
            asset max_supply;

            uint64_t primary_key() const { return supply.symbol.code().raw(); }
        };
        typedef multi_index<name("stats"), currency_stats> currency_table;

        struct lockinfo
        {
            asset quantity;
            name lock_type;
        };

        TABLE lock
        {
            name owner;
            vector<lockinfo> lockinfos;

            uint64_t primary_key() const { return owner.value; }
        };
        typedef multi_index<name("locks"), lock> lock_table;

        TABLE locktype
        {
            name lock_type;
            int32_t start_timestamp_sec;
            vector<int32_t> percents;

            uint64_t primary_key() const { return lock_type.value; }
        };
        typedef multi_index<name("locktypes"), locktype> locktype_table;

        TABLE blacklist
        {
            name owner;

            uint64_t primary_key() const { return owner.value; }
        };
        typedef multi_index<name("blacklists"), blacklist> blacklist_table;

        const int SECONDS_OF_DAY = 86400;
        void sub_balance(name owner, asset value);
        void add_balance(name owner, asset value, name ram_payer);
        void change_max_supply(const asset& quantity);
};

EOSIO_DISPATCH( itamtokenadm, (create)(issue)(transfer)(burn)(mint)(setlocktype)(dellocktype)(regblacklist)(delblacklist) )