#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>
#include "../../include/json.hpp"

using namespace eosio;
using namespace std;

CONTRACT itamstoreapp : public eosio::contract {
    public:
        using contract::contract;

        const uint64_t RATE_SELL =              70;

        using json = nlohmann::json;

        ACTION test(uint64_t cmd);
        ACTION regsellitem(string params);
        ACTION delsellitem(uint64_t appId, uint64_t itemid);
        ACTION modsellitem(uint64_t appId, uint64_t itemid, string itemname, asset eosvalue, asset itamvalue);
        ACTION receiptrans(uint64_t appId, uint64_t itemid, string itemname, name from, asset value, string notititle, string notitext, string notistr);
        ACTION regsettle(uint64_t appId, name account);
        ACTION msettlename(uint64_t appId, name account);
        ACTION resetsettle(uint64_t appId);
        ACTION rmsettle(uint64_t appId);
        ACTION claimsettle(uint64_t appId, name from);
        ACTION mdelsellitem(string jsonstr);
       
    private:
        TABLE sellitem {
            uint64_t            itemId;
            string              itemName;
            asset               eos;
            asset               itam;

            uint64_t primary_key() const { return itemId; }
        };

        typedef eosio::multi_index<"sellitems"_n, sellitem> sellItemTable;

        TABLE settle {
            uint64_t            appId;
            name                account;
            asset               eosvalue;
            asset               itamvalue;

            uint64_t primary_key() const { return appId; }
        };

        typedef eosio::multi_index<"settles"_n, settle> settle_t;

		struct tdata {
            name  	        from;
            name  	        to;
            asset         	quantity;
            string   		memo;
        };

        // to store parse string transfered memo
        struct st_memo {
            string          itemId;
            string          appId;
            string          itemname;
            string          notititle;
            string          notitext;
            string          notikey;
        };

    public:
        void transfer(uint64_t sender, uint64_t receiver);
        void parse_memo(string memo, st_memo* msg, string delimiter);
};