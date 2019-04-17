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
        ACTION staking(name owner, asset value);
        ACTION unstaking(name owner, asset value);
        ACTION defrefund(name owner);
        ACTION menualrefund(name owner);
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

         TABLE stake {
            name owner;
            asset balance;

            uint64_t primary_key() const { return owner.value; }
        };
        typedef multi_index<"stakes"_n, stake> stake_table;

        struct refund_info {
            asset refund_quantity;
            uint64_t req_refund_sec;
        };

        TABLE refund {
            name owner;
            asset total_refund;
            std::vector<refund_info> refund_list;

            uint64_t primary_key() const { return owner.value; }
        };
        typedef multi_index<"refunds"_n, refund> refund_table;

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
        const int SEC_REFUND_DELAY = 10;
        void sub_balance(name owner, asset value);
        void add_balance(name owner, asset value, name ram_payer);
        void assert_overdrawn_balance(const name& owner, int64_t available_balance, const asset& value);
        void refund(name owner);
};

EOSIO_DISPATCH( itamtokenadm, (create)(issue)(transfer)(burn)(mint)(staking)(unstaking)(defrefund)(menualrefund)(setlocktype)(dellocktype)(regblacklist)(delblacklist) )