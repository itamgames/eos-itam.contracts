#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>
#include "../include/json.hpp"
#include "../include/string.hpp"
#include "../include/dispatcher.hpp"
#include "../include/ownergroup.hpp"

using namespace eosio;
using namespace std;
using namespace nlohmann;

CONTRACT itamstoreapp : public contract
{
    public:
        itamstoreapp(name receiver, name code, datastream<const char*> ds) : contract(receiver, code, ds),
        configs(_self, _self.value){}
        
        ACTION registapp(uint64_t appId, name owner, asset amount, string params);
        ACTION deleteapp(uint64_t appId);
        ACTION refundapp(uint64_t appId, string buyer, name buyerGroup);
        ACTION registitems(string params);
        ACTION deleteitems(string params);
        ACTION modifyitem(uint64_t appId, uint64_t itemId, string itemName, asset price);
        ACTION refunditem(uint64_t appId, uint64_t itemId, string buyer, name buyerGroup);
        ACTION useitem(uint64_t appId, uint64_t itemId, string memo);
        ACTION transfer(uint64_t from, uint64_t to);
        ACTION receiptapp(uint64_t appId, name from, string owner, name ownerGroup, asset quantity);
        ACTION receiptitem(uint64_t appId, uint64_t itemId, string itemName, name from, string owner, name ownerGroup, asset quantity);
        ACTION defconfirm(uint64_t appId, string owner, name ownerGroup);
        ACTION menconfirm(uint64_t appId, string owner, name ownerGroup);
        ACTION setsettle(uint64_t appId, name account);
        ACTION claimsettle(uint64_t appId);
        ACTION setconfig(uint64_t ratio, uint64_t refundableDay);
    private:
        // item
        TABLE item
        {
            uint64_t id;
            string itemName;
            asset price;

            uint64_t primary_key() const { return id; }
        };
        typedef multi_index<"items"_n, item> itemTable;

        // app
        TABLE app
        {
            uint64_t id;
            name owner;
            asset price;

            uint64_t primary_key() const { return id; }
        };
        typedef multi_index<"apps"_n, app> appTable;

        // settle
        const uint64_t SECONDS_OF_DAY = 86400; // 1 day == 24 hours == 1440 minutes == 86400 seconds

        struct pendingInfo
        {
            uint64_t appId;
            uint64_t itemId;
            asset settleAmount;
            uint64_t timestamp;
        };

        TABLE pending
        {
            name ownerGroup;
            map<string, vector<pendingInfo>> infos;

            uint64_t primary_key() const { return ownerGroup.value; }
        };
        typedef multi_index<"pendings"_n, pending> pendingTable;

        TABLE settle
        {
            uint64_t appId;
            name account;
            asset settleAmount;

            uint64_t primary_key() const { return appId; }
        };
        typedef multi_index<"settles"_n, settle> settleTable;

        TABLE config
        {
            name key;
            uint64_t ratio;
            uint64_t refundableDay;

            uint64_t primary_key() const { return key.value; }
        };
        typedef multi_index<"configs"_n, config> configTable;
        configTable configs;

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

        void confirm(uint64_t appId, const string& owner, name ownerGroup);
        void refund(uint64_t appId, uint64_t itemId, string owner, name ownerGroup, asset refund);
};

#define ITEM_ACTION (registitems)(deleteitems)(modifyitem)(refunditem)(useitem)
#define APP_ACTION (registapp)(deleteapp)(refundapp)
#define SETTLE_ACTION (claimsettle)(setsettle)(defconfirm)(menconfirm)(setconfig)
#define BUY_ACTION (transfer)(receiptapp)(receiptitem)

EOSIO_DISPATCH_EX( itamstoreapp, ITEM_ACTION APP_ACTION SETTLE_ACTION BUY_ACTION )