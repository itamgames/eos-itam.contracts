#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include "../include/string.hpp"
#include "../include/dispatcher.hpp"
#include "../include/ownergroup.hpp"
#include "../include/transferstruct.hpp"
#include "../include/settle.hpp"

using namespace eosio;
using namespace std;

#ifndef BETA
    #define NFT_CONTRACT "itamstorenft"
#else
    #define NFT_CONTRACT "itamtestsnft"
#endif

CONTRACT itamstoredex : contract
{
    public:
        using contract::contract;

        ACTION sellorder(name owner, symbol_code symbol_name, string item_id, asset quantity);
        ACTION cancelorder(name owner, symbol_code symbol_name, string item_id);
        ACTION deleteorders(symbol_code symbol_name, vector<string> item_ids);
        ACTION resetorders(symbol_code symbol_name);
        ACTION transfer(uint64_t from, uint64_t to);

        ACTION settoken(name contract_name, string symbol_name, uint32_t precision);
        ACTION deletetoken(string symbol_name, uint32_t precision);
        ACTION setconfig(uint64_t fees_rate, uint64_t settle_rate);

        ACTION receipt(name owner, name owner_group, uint64_t app_id, uint64_t item_id, string nickname, uint64_t group_id, string item_name, string options, uint64_t duration, bool transferable, asset payment_quantity, string state);
    private:
        TABLE order
        {
            uint64_t item_id;
            name owner;
            name owner_account;
            string owner_nickname;
            asset quantity;

            uint64_t primary_key() const { return item_id; }
        };
        typedef multi_index<name("orders"), order> order_table;

        TABLE config
        {
            name key;
            uint64_t fees_rate;
            uint64_t settle_rate;

            uint64_t primary_key() const { return key.value; }
        };
        typedef multi_index<name("configs"), config> config_table;

        TABLE token
        {
            symbol available_symbol;
            name contract_name;

            uint64_t primary_key() const { return available_symbol.code().raw(); }
        };
        typedef multi_index<name("tokens"), token> token_table;

        // itamstorenft table
        struct currency
        {
            name issuer;
            asset supply;
            uint64_t app_id;

            uint64_t primary_key() const { return supply.symbol.code().raw(); }
        };
        typedef multi_index<name("currencies"), currency> currency_table;

        // itamstorenft table
        struct account
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

        struct transfer_memo
        {
            string buyer_nickname;
            string symbol_name;
            string item_id;
            string owner;
        };
};

ALLOW_TRANSFER_ALL_DISPATCHER( itamstoredex, (sellorder)(deleteorders)(cancelorder)(resetorders)(setconfig)(receipt)(settoken)(deletetoken)(transfer) )