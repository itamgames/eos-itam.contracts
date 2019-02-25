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
            // for(auto iter = currencies.begin(); iter != currencies.end(); iter = currencies.erase(iter));

            // account_table accounts(_self, name("itamgameusra").value
            // for(auto iter = accounts.begin(); iter != accounts.end(); iter = accounts.erase(iter));

            // account_table accounts2(_self, name("itamgameusrb").value);
            // for(auto iter = accounts2.begin(); iter != accounts2.end(); iter = accounts2.erase(iter));
        }

        ACTION create(name issuer, name symbol_name, uint64_t app_id, string structs);
        ACTION issue(name to, asset quantity, uint64_t token_id, string token_name, string category, bool fungible, string options, string reason);
        ACTION transfer(name from, name to, asset quantity, uint64_t token_id, string memo);
        ACTION transfernft(name from, name to, name symbol_name, vector<uint64_t> token_ids, string memo);
        ACTION burn(name owner, asset quantity, uint64_t token_id, string reason);
        ACTION burnnft(name owner, asset quantity, vector<uint64_t> token_ids, string reason);
        ACTION addcategory(name symbol_name, string category_name, vector<string> fields);
        ACTION modify(name owner, name symbol_name, uint64_t token_id, string token_name, string options, string reason);
    private:
        struct category
        {
            string name;
            vector<string> fields;
        };

        TABLE currency
        {
            name issuer;
            asset supply;
            uint64_t app_id;
            vector<category> categories;

            uint64_t primary_key() const { return supply.symbol.code().raw(); }
        };
        typedef multi_index<"currencies"_n, currency> currency_table;
        currency_table currencies;

        struct token
        {
            string category;
            string token_name;
            bool fungible;
            uint64_t count;
            string options;
        };

        TABLE account
        {
            asset balance;
            map<uint64_t, token> tokens;

            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };
        typedef multi_index<"accounts"_n, account> account_table;

        TABLE whitelist
        {
            name user;
            uint64_t primary_key() const { return user.value; }
        };
        typedef multi_index<"whitelists"_n, whitelist> whitelist_table;

        void add_balance(name owner, name ram_payer, asset quantity, string category, uint64_t token_id, const string& token_name, bool fungible, const string& options);
        void sub_balance(name to, asset quantity, uint64_t token_id);
        inline void validate_options(const vector<category>& categories, const string& category_name, const string& options);
        inline vector<string> get_fields(const vector<category>& categories, const string& category);
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

EOSIO_DISPATCH_EX(itamdigasset, (test)(create)(issue)(burn)(burnnft)(transfer)(transfernft)(modify)(addcategory))