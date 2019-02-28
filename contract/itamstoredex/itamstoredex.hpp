#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;
using namespace std;

CONTRACT itamstoredex : public contract {
    public:
        using contract::contract;
        ACTION sellorder(name owner, symbol_code symbol_name, uint64_t token_id);
        ACTION cancelorder(symbol_code symbol_name, uint64_t token_id);
        ACTION transfer(uint64_t from, uint64_t to);
        ACTION addcompany(name account, string company_name);
    private:
        TABLE order
        {
            uint64_t token_id;
            name owner;
            asset price;
            name group_name;

            uint64_t primary_key() const { return token_id; }
        };
        typedef multi_index<"orders", order> order_table;

        TABLE ownergroup
        {
            name group_name;

            uint64_t primary_key() const {  return owner.value; } 
        }
        typedef multi_index<"ownergroups", ownergroup> ownergroup_table;

        struct token
        {
            name owner;
            name owner_group;
            string category;
            string token_name;
            bool fungible;
            uint64_t count;
            string options;
        };

        struct account
        {
            asset balance;
            map<uint64_t, token> tokens;

            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };
        typedef multi_index<"accounts", account> account_table;

        struct transfer_data
        {
            name from;
            name to;
            asset quantity;
            string memo;
        };
};