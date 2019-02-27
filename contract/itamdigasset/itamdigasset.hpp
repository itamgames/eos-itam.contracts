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

        ACTION create(name issuer, string symbol_name, uint64_t app_id, string structs);
        ACTION issue(name to, asset quantity, string token_name, string category, bool fungible, string options, string reason);
        ACTION transfer(name from, name to, asset quantity, uint64_t token_id, string memo);
        ACTION transfernft(name from, name to, string symbol_name, vector<uint64_t> token_ids, string memo);
        ACTION burn(name owner, asset quantity, uint64_t token_id, string reason);
        ACTION burnnft(name owner, string symbol_name, vector<uint64_t> token_ids, string reason);
        ACTION addcategory(string symbol_name, string category_name, vector<string> fields);
        ACTION modify(name owner, string symbol_name, uint64_t token_id, string token_name, string options, string reason);
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
            uint64_t sequence;

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

        void add_balance(name owner, name ram_payer, asset quantity, const string& category, uint64_t token_id, string token_name, const string& options);
        void add_nft_balance(name owner, name ram_payer, const symbol& sym, const string& category, uint64_t token_id, const string& token_name, const string& options);
        void sub_balance(name owner, asset quantity, uint64_t token_id);
        void sub_nft_balance(name owner, const symbol& sym, uint64_t token_id);

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