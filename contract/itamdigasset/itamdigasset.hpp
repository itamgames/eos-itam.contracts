#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include "../include/json.hpp"

using namespace eosio;
using namespace std;

CONTRACT itamdigasset : contract
{
    public:
        itamdigasset(name receiver, name code, datastream<const char*> ds) : contract(receiver, code, ds),
        currencies(_self, _self.value) {}
        using json = nlohmann::json;

        ACTION test()
        {
            currencyTable currencies(_self, _self.value);
            for(auto iter = currencies.begin(); iter != currencies.end(); iter = currencies.erase(iter));

            accountTable accounts(_self, name("itamgameusra").value);
            for(auto iter = accounts.begin(); iter != accounts.end(); iter = accounts.erase(iter));

            accountTable accounts2(_self, name("itamgameusrb").value);
            for(auto iter = accounts2.begin(); iter != accounts2.end(); iter = accounts2.erase(iter));
        }

        ACTION create(name issuer, string name, uint64_t appId, string itemStructs);
        ACTION issue(name to, asset quantity, uint64_t itemId, string itemName, bool fungible, string itemDetail, string memo);
        ACTION transfer(name from, name to, asset quantity, uint64_t itemId, string memo);
        ACTION burn(name user, asset quantity, uint64_t itemId);
    private:
        struct itemStruct
        {
            string category;
            vector<string> fields;
        };

        TABLE currency
        {
            name issuer;
            asset supply;
            uint64_t appId;
            vector<itemStruct> itemStructs;

            uint64_t primary_key() const { return supply.symbol.code().raw(); }
        };
        typedef multi_index<"currencies"_n, currency> currencyTable;
        currencyTable currencies;

        struct item
        {
            string itemName;
            bool fungible;
            uint64_t count;
            string option;
        };

        TABLE account
        {
            asset balance;
            map<uint64_t, item> items;

            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };
        typedef multi_index<"accounts"_n, account> accountTable;

        void addBalance(name owner, name ramPayer, asset quantity, uint64_t itemId, const string& itemName, bool fungible, const string& data);
        void subBalance(name to, asset quantity, uint64_t itemId);
        inline void validateItemDetail(const vector<itemStruct>& itemStructs, const string& itemDetail);
        vector<string> getItemStruct(const vector<itemStruct>& itemStructs, const string& category);
};

#define EOSIO_DISPATCH_EX( TYPE, MEMBERS ) \
extern "C" { \
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
        if( code == receiver || code == name("itamstoreapp").value ) { \
            switch( action ) { \
                EOSIO_DISPATCH_HELPER( TYPE, MEMBERS ) \
            } \
        } \
    } \
} \

EOSIO_DISPATCH_EX(itamdigasset, (test)(create)(issue)(burn)(transfer))