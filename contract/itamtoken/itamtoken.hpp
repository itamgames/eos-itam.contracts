#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>

using namespace eosio;
using namespace std;

CONTRACT itamtoken : public contract {
    public:
        using contract::contract;

        ACTION create(name issuer, asset maximum_supply);
        ACTION issue(name to, asset quantity, string memo);
        ACTION transfer(name from, name to, asset amount, string memo);
        ACTION staking(name owner, asset value);
        ACTION unstaking(name owner, asset quantity);
        ACTION defrefund(name owner);
        ACTION menualrefund(name owner);
        ACTION test(name owner, asset quantity)
        {
            refund_table refunds(_self, owner.value);
            auto refund = refunds.find(owner.value);

            if(refund != refunds.end())
            {
                refunds.erase(refund);
                print("refund erase!");
            }

            account_table accounts(_self, owner.value);
            auto account = accounts.find(quantity.symbol.code().raw());

            if(account != accounts.end())
            {
                accounts.erase(account);
                print("account erase!");
            }
            

            stake_table stakes(_self, owner.value);
            auto stake = stakes.find(owner.value);

            if(stake != stakes.end())
            {
                stakes.erase(stake);
                print("stake erase");
            }
        }

    private:
        const uint64_t SEC_REFUND_DELAY = 10;

        TABLE account {
            asset balance;

            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };
        typedef multi_index<"accounts"_n, account> account_table;

        TABLE currency {
            name issuer;
            asset supply;
            asset max_supply;

            uint64_t primary_key() const { return supply.symbol.code().raw(); }
        };
        typedef multi_index<"currencies"_n, currency> currency_table;

        TABLE stake {
            name owner;
            asset balance;

            uint64_t primary_key() const { return owner.value; }
        };
        typedef multi_index<"stakes"_n, stake> stake_table;

        struct refund_info {
            asset refund_amount;
            uint64_t req_refund_ts;
        };

        TABLE refund {
            name owner;
            asset total_refund;
            std::vector<refund_info> refund_list;

            uint64_t primary_key() const { return owner.value; }
        };
        typedef multi_index<"refunds"_n, refund> refund_table;

        void sub_balance(name owner, asset value);
        void add_balance(name owner, asset value, name ram_payer);
        void refund(name owner);
};