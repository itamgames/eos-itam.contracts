#include "itamdigasset.hpp"

ACTION itamdigasset::create(name issuer, string symbol_name, uint64_t app_id, string structs)
{
    require_auth(_self);

    symbol game_symbol(symbol_name, 0);
    
    eosio_assert(game_symbol.is_valid(), "Invalid symbol");
    eosio_assert(is_account(issuer), "Invalid issuer");

    const auto& currency = currencies.find(game_symbol.code().raw());
    eosio_assert(currency == currencies.end(), "Already token created");

    auto parsed_structs = json::parse(structs);

    currencies.emplace(_self, [&](auto &c) {
        c.issuer = issuer;
        c.supply = asset(0, game_symbol);
        c.app_id = app_id;
        c.sequence = 0;

        for(int i = 0; i < parsed_structs.size(); i++)
        {
            auto parsed_category = parsed_structs[i];
            string category_name = parsed_category["category"];
            vector<string> fields = parsed_category["fields"];

            c.categories.push_back(category{category_name, fields});
        }
    });
}

ACTION itamdigasset::issue(name to, asset quantity, string token_name, string category, bool fungible, string options, string reason)
{
    eosio_assert(is_account(to), "to account does not exist");
    eosio_assert(quantity.amount > 0, "must issue positive quantity");
    eosio_assert(!(fungible == false && quantity.amount > 1), "invalid fungible value");
    eosio_assert(quantity.symbol.precision() == 0, "precision must be set 0");
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");

    const auto& currency = currencies.require_find(quantity.symbol.code().raw(), "invalid symbol");
    eosio_assert(has_auth(_self) || has_auth(currency->issuer), "Unauthorized Error");
    uint64_t token_id = currency->sequence;

    currencies.modify(currency, _self, [&](auto &c) {
        c.supply += quantity;
        c.sequence += 1;
    });

    validate_options(currency->categories, category, options);

    if(fungible == true)
    {
        add_balance(currency->issuer, currency->issuer, quantity, category, token_id, token_name, options);
    }
    else
    {
        add_nft_balance(currency->issuer, currency->issuer, quantity.symbol, category, token_id, token_name, options);
    }
    

    if(to != currency->issuer)
    {
        if(fungible == true)
        {
            SEND_INLINE_ACTION(*this, transfer, {currency->issuer,"active"_n}, {currency->issuer, to, quantity, token_id, reason});
        }
        else
        {
            vector<uint64_t> token_ids {token_id};
            string symbol_name = quantity.symbol.code().to_string();

            SEND_INLINE_ACTION(*this, transfernft, {currency->issuer,"active"_n}, {currency->issuer, to, symbol_name, token_ids, reason});
        }
        
        
    }
}

ACTION itamdigasset::addcategory(string symbol_name, string category_name, vector<string> fields)
{
    require_auth(_self);

    const auto& currency = currencies.require_find(symbol(symbol_name, 0).code().raw(), "invalid symbol");
    for(auto iter = currency->categories.begin(); iter != currency->categories.end(); iter++)
    {
        eosio_assert(iter->name != category_name, "Duplicate category name");
    }

    currencies.modify(currency, _self, [&](auto &c) {
        c.categories.push_back(category{category_name, fields});
    });
}

ACTION itamdigasset::modify(name owner, string symbol_name, uint64_t token_id, string token_name, string options, string reason)
{
    require_auth(_self);

    symbol sym(symbol_name, 0);

    const auto& currency = currencies.require_find(sym.code().raw(), "Invalid symbol");

    account_table accounts(_self, owner.value);
    const auto& account = accounts.require_find(symbol(symbol_name, 0).code().raw(), "not enough balance");

    map<uint64_t, token> owner_tokens = account->tokens;
    eosio_assert(owner_tokens.count(token_id) > 0, "no item object found");

    token owner_token = owner_tokens[token_id];
    validate_options(currency->categories, owner_token.category, options);

    accounts.modify(account, _self, [&](auto &a) {
        a.tokens[token_id].token_name = token_name;
        a.tokens[token_id].options = options;
    });    
}

ACTION itamdigasset::transfer(name from, name to, asset quantity, uint64_t token_id, string memo)
{
    require_auth(from);

    eosio_assert(from != to, "cannot transfer to self");
    eosio_assert(is_account(to), "to account does not exist");
    eosio_assert(quantity.amount > 0, "must transfer positive quantity");
    eosio_assert(quantity.symbol.precision() == 0, "precision must be set 0");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    account_table accounts(_self, from.value);
    const auto& from_account = accounts.require_find(quantity.symbol.code().raw(), "not enough balance");
    
    map<uint64_t, token> from_tokens = from_account->tokens;
    token from_token = from_tokens[token_id];

    name ram_payer = has_auth(to) ? to : from;
    
    add_balance(to, ram_payer, quantity, from_token.category, token_id, from_token.token_name, from_token.options);
    sub_balance(from, quantity, token_id);
    
    require_recipient(from);
    require_recipient(to);
}

ACTION itamdigasset::transfernft(name from, name to, string symbol_name, vector<uint64_t> token_ids, string memo)
{
    require_auth(from);

    eosio_assert(from != to, "cannot transfer to self");
    eosio_assert(is_account(to), "to account does not exist");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    symbol sym(symbol_name, 0);
    
    account_table accounts(_self, from.value);
    const auto& from_account = accounts.require_find(sym.code().raw(), "not enough balance");

    map<uint64_t, token> from_tokens = from_account->tokens;

    name ram_payer = has_auth(to) ? to : from;

    for(auto iter = token_ids.begin(); iter != token_ids.end(); iter++)
    {
        uint64_t token_id = *iter;
        token from_token = from_tokens[token_id];
        
        eosio_assert(from_token.fungible == false, "transfernft cannot transfer fungible token");

        add_nft_balance(to, ram_payer, sym, from_token.category, token_id, from_token.token_name, from_token.options);
        sub_nft_balance(from, sym, token_id);
    }
}

ACTION itamdigasset::burn(name owner, asset quantity, uint64_t token_id, string reason)
{
    name payer = has_auth(owner) ? owner : _self;
    eosio_assert(has_auth(payer), "Unauthorized Error");
    eosio_assert(quantity.amount > 0, "quantity must be positive");

    const auto& currency = currencies.require_find(quantity.symbol.code().raw(), "Invalid symbol");
    currencies.modify(currency, payer, [&](auto &c) {
        c.supply -= quantity;
    });

    sub_balance(owner, quantity, token_id);
}

ACTION itamdigasset::burnnft(name owner, string symbol_name, vector<uint64_t> token_ids, string reason)
{
    name payer = has_auth(owner) ? owner : _self;
    eosio_assert(has_auth(payer), "Unauthorized Error");

    symbol sym(symbol_name, 0);
    
    const auto& currency = currencies.require_find(sym.code().raw(), "Invalid symbol");
    currencies.modify(currency, payer, [&](auto &c) {
        c.supply -= asset(token_ids.size(), sym);
    });

    for(auto iter = token_ids.begin(); iter != token_ids.end(); iter++)
    {
        sub_nft_balance(owner, sym, *iter);
    }
}

void itamdigasset::add_balance(name owner, name ram_payer, asset quantity, const string& category, uint64_t token_id, string token_name, const string& options)
{
    account_table accounts(_self, owner.value);
    const auto& account = accounts.find(quantity.symbol.code().raw());

    token t { category, token_name, true, (uint64_t)quantity.amount, options };

    if(account == accounts.end())
    {
        accounts.emplace(ram_payer, [&](auto &a) {
            a.balance = quantity;
            a.tokens[token_id] = t;
        });
    }
    else
    {
        accounts.modify(account, ram_payer, [&](auto &a) {
            a.balance += quantity;
            a.tokens[token_id].count += quantity.amount;
        });
    }
    
}

void itamdigasset::add_nft_balance(name owner, name ram_payer, const symbol& sym, const string& category, uint64_t token_id, const string& token_name, const string& options)
{
    account_table accounts(_self, owner.value);
    const auto& account = accounts.find(sym.code().raw());

    token t { category, token_name, false, 1, options };
    
    if(account == accounts.end())
    {
        accounts.emplace(ram_payer, [&](auto &a) {
            a.balance.amount = 1;
            a.balance.symbol = sym;
            a.tokens[token_id] = t;
        });
    }
    else
    {
        accounts.modify(account, ram_payer, [&](auto &a) {
            a.balance.amount += 1;
            a.tokens[token_id] = t;
        });
    }
    
}

void itamdigasset::sub_balance(name owner, asset quantity, uint64_t token_id)
{
    account_table accounts(_self, owner.value);
    const auto& account = accounts.require_find(quantity.symbol.code().raw(), "not enough balance");
    map<uint64_t, token> tokens = account->tokens;

    eosio_assert(tokens.count(token_id) > 0, "no item object found");
    eosio_assert(tokens[token_id].count >= quantity.amount, "quantity must below than items count");

    accounts.modify(account, _self, [&](auto &a) {
        a.balance -= quantity;

        if(a.tokens[token_id].count - quantity.amount == 0)
        {
            a.tokens.erase(token_id);
        }
        else
        {
            a.tokens[token_id].count -= quantity.amount;
        }
    });
}

void itamdigasset::sub_nft_balance(name owner, const symbol& sym, uint64_t token_id)
{
    account_table accounts(_self, owner.value);
    const auto& account = accounts.require_find(sym.code().raw(), "not enough balance");
    map<uint64_t, token> tokens = account->tokens;

    eosio_assert(tokens.count(token_id) > 0, "no item object found");

    accounts.modify(account, owner, [&](auto &a) {
        a.balance.amount -= 1;
        a.tokens.erase(token_id);
    });
}

void itamdigasset::validate_options(const vector<category>& categories, const string& category_name, const string& options)
{
    auto requested_fields = json::parse(options);
    vector<string> require_fields = get_fields(categories, category_name);

    eosio_assert(requested_fields.size() == require_fields.size(), "invalid item options");
    
    for(auto iter = require_fields.begin(); iter != require_fields.end(); iter++)
    {
        eosio_assert(requested_fields.count(*iter) > 0, "invalid item options");
    }
}

vector<string> itamdigasset::get_fields(const vector<category>& categories, const string& category)
{
    for(auto iter = categories.begin(); iter != categories.end(); iter++)
    {
        if(iter->name == category)
        {
            return iter->fields;
        }
    }

    eosio_assert(false, "invalid category");
    
    vector<string> temp;
    return temp;
}