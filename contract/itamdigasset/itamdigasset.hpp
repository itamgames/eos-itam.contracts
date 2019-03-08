#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include "../include/json.hpp"
#include "../include/common.hpp"

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
            account_table accounts(_self, _self.value);
            for(auto iter = accounts.begin(); iter != accounts.end(); iter = accounts.erase(iter));
        }
        ACTION create(name issuer, symbol_code symbol_name, uint64_t app_id);
        ACTION issue(string to, name to_group, symbol_code symbol_name, uint64_t group_id, string token_name, string options, string reason);
        ACTION modify(string owner, name owner_group, symbol_code symbol_name, uint64_t token_id, uint64_t group_id, string token_name, string options, string reason);
        ACTION transfernft(string from, name from_group, string to, name to_group, symbol_code symbol_name, vector<uint64_t> token_ids, string memo);
        ACTION burn(string owner, name owner_group, symbol_code symbol_name, vector<uint64_t> token_ids, string reason);
        ACTION addcategory(symbol_code symbol_name, string first_category, string second_category);

        ACTION sellorder(string owner, name owner_group, symbol_code symbol_name, uint64_t token_id, asset price);
        ACTION modifyorder(string owner, name owner_group, symbol_code symbol_name, uint64_t token_id, asset price);
        ACTION cancelorder(string owner, name owner_group, symbol_code symbol_name, uint64_t token_id);
        ACTION transfer(uint64_t from, uint64_t to);
    private:
        TABLE currency
        {
            name issuer;
            asset supply;
            uint64_t app_id;
            uint64_t sequence;

            uint64_t primary_key() const { return supply.symbol.code().raw(); }
        };
        typedef multi_index<name("currencies"), currency> currency_table;
        currency_table currencies;

        struct token
        {
            string owner;
            uint64_t group_id;
            string token_name;
            string options;
        };

        TABLE account
        {
            asset balance;
            map<uint64_t, token> tokens;

            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };
        typedef multi_index<name("accounts"), account> account_table;

        TABLE order
        {
            uint64_t token_id;
            string owner;
            name owner_group;
            asset price;

            uint64_t primary_key() const { return token_id; }
        };
        typedef multi_index<name("orders"), order> order_table;

        struct transfer_data
        {
            name from;
            name to;
            asset quantity;
            string memo;
        };

        struct memo_data
        {
            string buyer;
            string symbol_name;
            string token_id;
        };

        void add_balance(const string& owner, name group_name, name ram_payer, symbol_code symbol_name, uint64_t group_id, uint64_t token_id, const string& token_name, const string& options);
        void sub_balance(const string& owner, name group_name, uint64_t symbol_raw, uint64_t token_id);
};

#define EOSIO_DISPATCH_EX( TYPE, MEMBERS ) \
extern "C" { \
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
        if( code == receiver || code == name("eosio.token").value ) { \
            switch( action ) { \
                EOSIO_DISPATCH_HELPER( TYPE, MEMBERS ) \
            } \
        } \
    } \
} \

EOSIO_DISPATCH_EX(itamdigasset, (create)(issue)(burn)(transfernft)(modify)(sellorder)(modifyorder)(cancelorder)(transfer))