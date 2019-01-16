#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;
using namespace std;

CONTRACT itamstoredex : public eosio::contract {
    public:
        const uint64_t ISTAT_ACTIVE =       1; // normal status
        const uint64_t ISTAT_4SALE =        2; // for sale status

        using contract::contract;
        
        ACTION test(uint64_t cmd);
        ACTION createitem(uint64_t id, uint64_t gid, string itemname, string itemopt, name to);
        ACTION edititem(uint64_t id, uint64_t gid, string itemname, string itemopt, name holder);
        ACTION removeitem(uint64_t id, name holder);
        ACTION sellorder(uint64_t id, name from, asset eosvalue, asset itamvalue);
        ACTION revokeorder(uint64_t id, name from);
        ACTION receiptrans(uint64_t id, uint64_t gid, string itemname, string itemopt);

    private:
        struct [[eosio::table]] item {
            uint64_t            id;
            uint64_t            gid;
            string              itemname;
            string              itemopt;
            uint64_t            istatus;
            asset               eosvalue;
            asset               itamvalue;
            name                owner;

            uint64_t primary_key()const { return id; }
        };

        typedef eosio::multi_index<"items"_n, item> items_t;

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
        void parseMemo(string memo, st_memo* msg, string delimiter);
};