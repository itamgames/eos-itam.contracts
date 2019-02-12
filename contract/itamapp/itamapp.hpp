#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/transaction.hpp>
#include "../include/json.hpp"
#include "../include/common.hpp"

using namespace eosio;
using namespace std;

CONTRACT itamapp : public contract {
    public:
        using contract::contract;
        using json = nlohmann::json;

        struct transferData {
            name from;
            name to;
            asset quantity;
            string memo;
        };

        // item 
        ACTION registitems(string params);
        ACTION deleteitems(string params);
        ACTION modifyitem(uint64_t appId, uint64_t itemId, string itemName, asset eos, asset itam);
        ACTION refunditem(uint64_t appId, uint64_t itemId, name buyer);

        // app
        ACTION registapp(uint64_t appId, name owner, asset amount, string params);
        ACTION deleteapp(uint64_t appId);
        ACTION refundapp(uint64_t appId, name buyer);

        // leader board
        ACTION registboard(uint64_t boardId, uint64_t appId, name owner);
        ACTION score(uint64_t boardId, uint64_t appId, string score, name user, string data);
        
        // settle
        void confirm(uint64_t appId);
        ACTION defconfirm(uint64_t appId);
        ACTION menconfirm(uint64_t appId);
        ACTION setsettle(uint64_t appId, name account);
        ACTION claimsettle(uint64_t appId);
        ACTION setconfig(uint64_t ratio, uint64_t refundableDay);

        // buy
        ACTION transfer(uint64_t from, uint64_t to);
        ACTION receiptapp(uint64_t appId, name from, asset quantity);
        ACTION receiptitem(uint64_t appId, uint64_t itemId, string itemName, string packageName, string token, name from, asset quantity);
    private:
        // item
        TABLE item {
            uint64_t itemId;
            string itemName;
            asset eos;
            asset itam;

            uint64_t primary_key() const { return itemId; }
        };
        typedef multi_index<"items"_n, item> itemTable;



        // app
        TABLE app
        {
            uint64_t appId;
            name owner;
            asset amount;

            uint64_t primary_key() const { return appId; }
        };
        typedef multi_index<"apps"_n, app> appTable;



        // achievement
        TABLE achievement
        {
            id_type id;
            string name;

            id_type primary_key() const { return id; }
        };
        typedef mutli_index<"achievements"_n, achievement> achievementTable



        // leader board
        TABLE leaderboard
        {
            id_type id;
            string name;
            uint64_t precision;
            string minimumScore;
            string maximumScore;
            
            id_type primary_key() const { return id; }
        };
        typedef multi_index<"leaderboards"_n, leaderboard> leaderboardTable;

        TABLE block
        {
            name owner;
            uint64_t primary_key() const { return owner.value; }
        };
        typedef multi_index<"blocks"_n, block> blockTable;



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
            name buyer;
            vector<pendingInfo> pendingList;

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

        TABLE config
        {
            name key;
            uint64_t ratio;
            uint64_t refundableDay;

            uint64_t primary_key() const { return key.value; }
        };
        typedef multi_index<"configs"_n, config> configTable;
    
    private:
        bool isValidPrecision(string number, uint64_t precision);
};

#define EOSIO_DISPATCH_EX( TYPE, MEMBERS ) \
extern "C" { \
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
        bool isAllowedContract = code == name("eosio.token").value || code == name("itamtokenadm").value; \
        if( code == receiver || isAllowedContract ) { \
            if(action == name("transfer").value) { \
                eosio_assert(isAllowedContract, "eosio.token or itamtokenadm can call internal transfer"); \
            } \
            switch( action ) { \
                EOSIO_DISPATCH_HELPER( TYPE, MEMBERS ) \
            } \
        } \
    } \
} \

#define ITEM_ACTION (registitems)(deleteitems)(modifyitem)(refunditem)
#define APP_ACTION (registapp)(deleteapp)(refundapp)
#define SETTLE_ACTION (claimsettle)(setsettle)(defconfirm)(menconfirm)(setconfig)
#define COMMON_ACTION (transfer)(receiptapp)(receiptitem)

EOSIO_DISPATCH_EX( itamapp, ITEM_ACTION APP_ACTION SETTLE_ACTION COMMON_ACTION )