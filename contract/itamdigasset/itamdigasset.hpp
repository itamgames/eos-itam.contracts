#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include "../include/string.hpp"
#include "../include/dispatcher.hpp"
#include "../include/ownergroup.hpp"

using namespace eosio;
using namespace std;

CONTRACT itamdigasset : contract
{
    public:
        itamdigasset(name receiver, name code, datastream<const char*> ds) : contract(receiver, code, ds),
        currencies(_self, _self.value) {}
        
        ACTION test()
        {
            account_table accounts(_self, _self.value);
            for(auto iter = accounts.begin(); iter != accounts.end(); iter = accounts.erase(iter));
        }
        ACTION create(name issuer, symbol_code symbol_name, uint64_t app_id);
        ACTION issue(string to, name to_group, string nickname, symbol_code symbol_name, uint64_t group_id, string item_name, string category, string options, string reason);
        ACTION modify(string owner, name owner_group, symbol_code symbol_name, uint64_t item_id, uint64_t group_id, string item_name, string category, string options, bool transferable, string reason);
        ACTION transfernft(string from, name from_group, string to, name to_group, string to_nickname, symbol_code symbol_name, vector<uint64_t> item_ids, string memo);
        ACTION burn(string owner, name owner_group, symbol_code symbol_name, vector<uint64_t> item_ids, string reason);

        ACTION sellorder(string owner, name owner_group, symbol_code symbol_name, uint64_t item_id, asset price);
        ACTION modifyorder(string owner, name owner_group, symbol_code symbol_name, uint64_t item_id, asset price);
        ACTION cancelorder(string owner, name owner_group, symbol_code symbol_name, uint64_t item_id);
        ACTION transfer(uint64_t from, uint64_t to);

        ACTION setconfig(uint64_t fees_rate, uint64_t settle_rate);
        ACTION setsettle(symbol_code symbol_name, name account);
        ACTION claimsettle(symbol_code symbol_name);

        ACTION addwhitelist(name allow_account);
        ACTION modifygroup(name owner, name group_account);
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

        struct item
        {
            string owner;
            string nickname;
            uint64_t group_id;
            string item_name;
            string category;
            string options;
            bool transferable;
        };

        TABLE account
        {
            asset balance;
            map<uint64_t, item> items;

            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };
        typedef multi_index<name("accounts"), account> account_table;

        TABLE order
        {
            uint64_t item_id;
            string owner;
            name owner_group;
            string nickname;
            asset price;

            uint64_t primary_key() const { return item_id; }
        };
        typedef multi_index<name("orders"), order> order_table;

        TABLE allow
        {
            name account;
            uint64_t primary_key() const { return account.value; }
        };
        typedef multi_index<name("allows"), allow> allow_table;

        TABLE settle
        {
            symbol_code sym;
            name account;
            asset settle_amount;

            uint64_t primary_key() const { return sym.raw(); }
        };
        typedef multi_index<name("settles"), settle> settle_table;

        TABLE config
        {
            name key;
            uint64_t fees_rate;
            uint64_t settle_rate;

            uint64_t primary_key() const { return key.value; }
        };
        typedef multi_index<name("configs"), config> config_table;

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
            string buyer_nickname;
            string buyer_group;
            string symbol_name;
            string item_id;
        };

        void add_balance(name group_account, name ram_payer, symbol_code symbol_name, uint64_t item_id, const item& t);
        void sub_balance(const string& owner, name group_account, name ram_payer, uint64_t symbol_raw, uint64_t item_id);
};

EOSIO_DISPATCH_EX(itamdigasset, (create)(issue)(burn)(transfernft)(modify)(sellorder)(modifyorder)(cancelorder)(modifygroup)(addwhitelist)(transfer)(setconfig)(setsettle)(claimsettle))