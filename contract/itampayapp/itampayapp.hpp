#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>

using namespace std;
using namespace eosio;

CONTRACT itampayapp : contract {
    public:
        using contract::contract;

        ACTION test();
        ACTION registapp(uint64_t appId, name owner, asset amount, string params);
        ACTION removeapp(uint64_t appId);
        ACTION refundapp(uint64_t appId, name buyer);
        ACTION defconfirm(uint64_t appId);
        ACTION menconfirm(uint64_t appId);
        ACTION modifysettle(uint64_t appId, name newAccount);
        ACTION claimsettle(uint64_t appId);
        ACTION setconfig(uint64_t ratio, uint64_t refundableDay);
        ACTION receipt(uint64_t appId, asset amount, name owner, name buyer);
        void confirm(uint64_t appId);
        void transfer(uint64_t from, uint64_t to);
    private:
        const uint64_t SECOND_OF_DAY = 86400;

        TABLE config
        {
            name key;
            uint64_t ratio;
            uint64_t refundableDay;

            uint64_t primary_key() const { return key.value; }
        };
        typedef multi_index<"configs"_n, config> configTable;

        TABLE app
        {
            uint64_t appId;
            name owner;
            asset amount;

            uint64_t primary_key() const { return appId; }
        };
        typedef multi_index<"apps"_n, app> appTable;

        TABLE pending
        {
            name buyer;
            asset settleAmount;
            uint64_t timestamp;

            uint64_t primary_key() const { return buyer.value; }
        };
        typedef multi_index<"pendings"_n, pending> pendingTable;

        TABLE settle
        {
            uint64_t appId;
            name account;
            asset eos;
            asset itam;

            uint64_t primary_key() const { return appId; }
        };
        typedef multi_index<"settles"_n, settle> settleTable;

        struct transferData {
            name from;
            name to;
            asset quantity;
            string memo;
        };
};