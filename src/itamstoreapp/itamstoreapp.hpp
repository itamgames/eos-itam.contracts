#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>
#include "../include/json.hpp"
#include "../include/string.hpp"
#include "../include/settle.hpp"
#include "../include/date.hpp"
#include "../include/ownergroup.hpp"
#include "../include/dispatcher.hpp"
#include "../include/accounts.hpp"

using namespace eosio;
using namespace std;
using namespace nlohmann;

CONTRACT itamstoreapp : public contract
{
    public:
        itamstoreapp(name receiver, name code, datastream<const char*> ds) : contract(receiver, code, ds), configs(_self, _self.value) {}
        
        ACTION registapp(string appId, name owner, asset price, string params);
        ACTION deleteapp(string appId);
        ACTION registitems(string params);
        ACTION deleteitems(string params);
        ACTION modifyitem(string appId, string itemId, string itemName, asset price);
        ACTION transfer(uint64_t from, uint64_t to);
        ACTION receiptapp(uint64_t appId, name from, name owner, name ownerGroup, asset quantity);
        ACTION receiptitem(uint64_t appId, uint64_t itemId, string itemName, name from, name owner, name ownerGroup, asset quantity);
        ACTION useitem(string appId, string itemId, string memo);
        ACTION setconfig(uint64_t ratio, uint64_t refundableDay);
        ACTION refundapp(string appId, name owner, uint64_t paymentTimestamp);
        ACTION refunditem(string appId, string itemId, name owner, uint64_t paymentTimestamp);
        ACTION defconfirm(uint64_t appId, name owner);
        ACTION confirmall(string appId);
    private:
        struct paymentInfo
        {
            uint64_t itemId;
            asset paymentAmount;
            asset settleAmount;
            uint64_t timestamp;
        };

        TABLE payment
        {
            name owner;
            name ownerGroup;
            vector<paymentInfo> progress;

            uint64_t primary_key() const { return owner.value; }
        };
        typedef multi_index<name("payments"), payment> paymentTable;

        TABLE config
        {
            name key;
            uint64_t ratio;
            uint64_t refundableDay;

            uint64_t primary_key() const { return key.value; }
        };
        typedef multi_index<name("configs"), config> configTable;
        configTable configs;

        void confirm(uint64_t appId, payment& p, uint64_t currentTimestamp, uint64_t refundableDay);
        void refund(const uint64_t appId, const uint64_t itemId, const name& owner, const uint64_t paymentTimestamp);
        
        TABLE item
        {
            uint64_t id;
            string itemName;
            asset price;

            uint64_t primary_key() const { return id; }
        };
        typedef multi_index<name("items"), item> itemTable;

        TABLE app
        {
            uint64_t id;
            name owner;
            asset price;

            uint64_t primary_key() const { return id; }
        };
        typedef multi_index<name("apps"), app> appTable;

        struct transferData
        {
            name from;
            name to;
            asset quantity;
            string memo;
        };

        struct memoData
        {
            string category;
            string owner;
            string ownerGroup;
            string appId;
            string itemId;
        };
};

#if defined(MAINNET) || defined(TESTNET)
    ALLOW_TRANSFER_ITAM_EOS_DISPATCHER( itamstoreapp, (registitems)(deleteitems)(modifyitem)(useitem)(registapp)(deleteapp)(transfer)(receiptapp)(receiptitem)(refundapp)(refunditem)(defconfirm)(confirmall)(setconfig), &itamstoreapp::transfer )
#else
    ALLOW_TRANSFER_ITAM_EOS_DISPATCHER( itamstoreapp, (registitems)(deleteitems)(modifyitem)(useitem)(registapp)(deleteapp)(transfer)(receiptapp)(receiptitem), &itamstoreapp::transfer )
#endif