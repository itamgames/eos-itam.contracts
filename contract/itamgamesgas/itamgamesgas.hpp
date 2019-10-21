#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include "../include/dispatcher.hpp"
#include "../include/transferstruct.hpp"
#include "../include/string.hpp"

using namespace eosio;

CONTRACT itamgamesgas : contract
{
    public:
        using contract::contract;

        ACTION setsettle(string app_id, name settle_account);
        ACTION claimsettle(string app_id);
        ACTION transfer(uint64_t from, uint64_t to);
    private:
        TABLE account
        {
            uint64_t app_id;
            name settle_account;

            uint64_t primary_key() const { return app_id; }
        };
        typedef multi_index<name("accounts"), account> account_table;

        TABLE settlement
        {
            asset quantity;
            
            uint64_t primary_key() const { return quantity.symbol.code().raw(); }
        };
        typedef multi_index<name("settlements"), settlement> settlement_table;

        // itamstoredex token
        struct token
        {
            symbol available_symbol;
            name contract_name;

            uint64_t primary_key() const { return available_symbol.code().raw(); }
        };
        typedef multi_index<name("tokens"), token> token_table;
};

ALLOW_TRANSFER_ALL_DISPATCHER( itamgamesgas, (setsettle)(claimsettle)(transfer) )