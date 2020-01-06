#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include "../include/string.hpp"
#include "../include/ownergroup.hpp"
#include "../include/accounts.hpp"

using namespace eosio;
using namespace std;

CONTRACT itamstorenft : contract
{
    public:
        itamstorenft(name receiver, name code, datastream<const char*> ds) : contract(receiver, code, ds), currencies(_self, _self.value) {}

        ACTION nft(name owner, name owner_group, string game_user_id, symbol_code symbol_name, string action, string reason);
        ACTION create(name issuer, symbol_code symbol_name, string app_id);
        ACTION issue(name to, name to_group, string nickname, symbol_code symbol_name, string item_id, string item_name, string group_id, string options, uint64_t duration, bool transferable, string reason);
        ACTION activate(name to, name to_group, string game_user_id, string nickname, symbol_code symbol_name, string item_id, string item_name, string group_id, string options, uint64_t duration, bool transferable, string reason);
        ACTION modify(name owner, name owner_group, symbol_code symbol_name, string item_id, string item_name, string options, uint64_t duration, bool transferable, string reason);
        ACTION changeowner(symbol_code symbol_name, string item_id, name owner, name owner_group, string nickname);
        ACTION burn(name owner, name owner_group, symbol_code symbol_name, string item_id, string reason);
        ACTION deactivate(name owner, name owner_group, symbol_code symbol_name, string item_id, string reason);
        ACTION burnall(symbol_code symbol);
        ACTION transfernft(name from, name to, symbol_code symbol_name, string item_id, string memo);
        ACTION addwhitelist(name account);
        ACTION delwhitelist(name account);
        ACTION changegroup(symbol_code symbol_name, string item_id, string group_id);
        ACTION receipt(name owner, name owner_group, uint64_t app_id, uint64_t item_id, string nickname, uint64_t group_id, string item_name, string options, uint64_t duration, bool transferable, asset payment_quantity, string state);
    private:
        void _issue(name to, name to_group, string nickname, symbol_code symbol_name, string item_id, string item_name, string group_id, string options, uint64_t duration, bool transferable, string reason);
        void _burn(name owner, name owner_group, symbol_code symbol_name, string item_id, string reason);
        TABLE currency
        {
            name issuer;
            asset supply;
            uint64_t app_id;

            uint64_t primary_key() const { return supply.symbol.code().raw(); }
        };
        typedef multi_index<name("currencies"), currency> currency_table;
        currency_table currencies;

        TABLE account
        {
            uint64_t item_id;
            string item_name;
            uint64_t group_id;
            uint64_t duration;
            string options;
            bool transferable;
            name owner;
            name owner_account;
            string nickname;

            uint64_t primary_key() const { return item_id; }
        };
        typedef multi_index<name("accounts"), account> account_table;

        TABLE allow
        {
            name account;
            uint64_t primary_key() const { return account.value; }
        };
        typedef multi_index<name("allows"), allow> allow_table;

        struct memo_data
        {
            string nickname;
            string to;
            string transfer_state;
        };
};

EOSIO_DISPATCH( itamstorenft, (nft)(activate)(deactivate)(create)(issue)(modify)(changegroup)(changeowner)(burn)(burnall)(receipt)(addwhitelist)(delwhitelist)(transfernft) )