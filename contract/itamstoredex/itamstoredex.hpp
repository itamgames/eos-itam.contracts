#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include "../include/string.hpp"
#include "../include/dispatcher.hpp"
#include "../include/ownergroup.hpp"
#include "../include/settle.hpp"

using namespace eosio;
using namespace std;

CONTRACT itamstoredex : contract
{
    public:
        itamstoredex(name receiver, name code, datastream<const char*> ds) : contract(receiver, code, ds),
        currencies(_self, _self.value) {}

        ACTION create(name issuer, symbol_code symbol_name, string app_id);
        ACTION issue(string to, name to_group, string nickname, symbol_code symbol_name, string item_id, string item_name, string category, string group_id, string options, uint64_t duration, bool transferable, string reason);
        ACTION modify(string owner, name owner_group, symbol_code symbol_name, string item_id, string item_name, string options, uint64_t duration, bool transferable, string reason);
        ACTION transfernft(name from, string to, symbol_code symbol_name, vector<string> item_ids, string memo);
        ACTION burn(string owner, name owner_group, symbol_code symbol_name, vector<string> item_ids, string reason);

        ACTION sellorder(string owner, symbol_code symbol_name, string item_id, asset quantity);
        ACTION cancelorder(string owner, symbol_code symbol_name, string item_id);
        ACTION transfer(uint64_t from, uint64_t to);

        ACTION setconfig(uint64_t fees_rate, uint64_t settle_rate);
        ACTION addwhitelist(name account);
        ACTION delwhitelist(name account);

        ACTION receipt(string owner, name owner_group, uint64_t app_id, uint64_t item_id, string nickname, uint64_t group_id, string item_name, string category, string options, uint64_t duration, bool transferable, string state);
        ACTION receiptdex(string owner, name owner_group, uint64_t app_id, uint64_t item_id, string nickname, uint64_t group_id, string item_name, string category, string options, uint64_t duration, string state);
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
            uint64_t duration;
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
            asset quantity;

            uint64_t primary_key() const { return item_id; }
        };
        typedef multi_index<name("orders"), order> order_table;

        TABLE allow
        {
            name account;
            uint64_t primary_key() const { return account.value; }
        };
        typedef multi_index<name("allows"), allow> allow_table;

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

        struct transfer_memo
        {
            string buyer_nickname;
            string symbol_name;
            string item_id;
            string owner;
        };

        struct transfernft_memo
        {
            string nickname;
            string to_group;
        };

        void add_balance(const name& group_account, const name& ram_payer, const symbol_code& symbol_name, uint64_t item_id, const item& i);
        void sub_balance(const string& owner, const name& group_account, const name& ram_payer, uint64_t symbol_raw, uint64_t item_id);
        item get_owner_item(const string& owner, const map<uint64_t, item>& owner_items, uint64_t item_id);
        void set_owner_group(const name& owner, name& owner_group_name, name& owner_group_account);
};

EOSIO_DISPATCH_EX( itamstoredex, (create)(issue)(burn)(transfernft)(modify)(sellorder)(cancelorder)(addwhitelist)(delwhitelist)(transfer)(setconfig)(receipt)(receiptdex) )