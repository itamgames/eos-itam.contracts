#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>
#include "json.hpp"

using namespace eosio;
using namespace std;

CONTRACT itamstoreapp : public eosio::contract {
    public:
        using contract::contract;

        const uint64_t RATE_SELL =              70;

        using json = nlohmann::json;

        ACTION test(uint64_t cmd);
        ACTION regsellitem(uint64_t gid, uint64_t itemid, string itemname, asset eosvalue, asset itamvalue, string itemdesc);
        ACTION delsellitem(uint64_t gid, uint64_t itemid);
        ACTION modsellitem(uint64_t gid, uint64_t itemid, string itemname, asset eosvalue, asset itamvalue, string itemdesc);
        ACTION receiptrans(uint64_t gameid, uint64_t itemid, string itemname, name from, asset value, string notititle, string notitext, string notistr, string options);
        ACTION regsettle(uint64_t gid, name account);
        ACTION msettlename(uint64_t gid, name account);
        ACTION resetsettle(uint64_t gid);
        ACTION rmsettle(uint64_t gid);
        ACTION claimsettle(uint64_t gid, name from);
        ACTION mregsellitem(string jsonstr);
       
    private:
        struct [[eosio::table]] sitem {
            uint64_t            id;
            uint64_t            gid;
            string              itemname;
            asset               eosvalue;
            asset               itamvalue;
            string              itemdesc;

            uint64_t primary_key() const { return id; }
        };

        typedef eosio::multi_index<"sitems"_n, sitem> sitems_t;

        struct [[eosio::table]] settle {
            uint64_t            gid;
            name                account;
            asset               eosvalue;
            asset               itamvalue;

            uint64_t primary_key()const { return gid; }
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
            string          itemid;
            string          gid;
            string          itemname;
            string          notititle;
            string          notitext;
            string          notikey;
        };

    public:
        void transfer(uint64_t sender, uint64_t receiver);
        void parse_memo(string memo, st_memo* msg, string delimiter);
};