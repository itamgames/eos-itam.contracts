#include "itamdigasset.hpp"

ACTION itamdigasset::create(name issuer, symbol_code symbol_name, uint64_t app_id)
{
    require_auth(_self);

    symbol game_symbol(symbol_name, 0);
    
    eosio_assert(game_symbol.is_valid(), "Invalid symbol");
    eosio_assert(is_account(issuer), "Invalid issuer");

    const auto& currency = currencies.find(symbol_name.raw());
    eosio_assert(currency == currencies.end(), "Already token created");

    currencies.emplace(_self, [&](auto &c) {
        c.issuer = issuer;
        c.supply = asset(0, game_symbol);
        c.app_id = app_id;
        c.sequence = 0;
    });
}

ACTION itamdigasset::issue(string to, name to_group, symbol_code symbol_name, uint64_t group_id, string token_name, string options, string reason)
{
    require_auth(_self);

    eosio_assert(is_account(to_group), "to_group account does not exist");
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");

    const auto& currency = currencies.require_find(symbol_name.raw(), "invalid symbol");

    currencies.modify(currency, _self, [&](auto &c) {
        c.supply.amount += 1;
        c.sequence += 1;
    });

    uint64_t token_id = currency->sequence;

    string stringified_issuer = currency->issuer.to_string();
    add_balance(stringified_issuer, currency->issuer, _self, symbol_name, group_id, token_id, token_name, options);

    if(to != stringified_issuer || to_group != currency->issuer)
    {
        vector<uint64_t> token_ids {token_id};
        SEND_INLINE_ACTION(
            *this,
            transfernft,
            { {currency->issuer, name("active")} },
            { stringified_issuer, currency->issuer, to, to_group, symbol_name, token_ids, reason }
        );
    }
}

ACTION itamdigasset::modify(string owner, name owner_group, symbol_code symbol_name, uint64_t token_id, uint64_t group_id, string token_name, string options, string reason)
{
    require_auth(_self);
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");
    
    uint64_t symbol_raw = symbol_name.raw();
    const auto& currency = currencies.require_find(symbol_raw, "Invalid symbol");

    account_table accounts(_self, owner_group.value);
    const auto& account = accounts.require_find(symbol_raw, "not enough balance");

    map<uint64_t, token> owner_tokens = account->tokens;
    auto iter = owner_tokens.find(token_id);
    eosio_assert(iter != owner_tokens.end(), "no item object found");

    token owner_token = iter->second;
    eosio_assert(owner_token.owner == owner, "different owner");

    accounts.modify(account, _self, [&](auto &a) {
        a.tokens[token_id].group_id = group_id;
        a.tokens[token_id].token_name = token_name;
        a.tokens[token_id].options = options;
    });    
}

ACTION itamdigasset::transfernft(string from, name from_group, string to, name to_group, symbol_code symbol_name, vector<uint64_t> token_ids, string memo)
{
    require_auth(from_group);

    eosio_assert(is_account(to_group), "receive group account does not exist");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");
    
    uint64_t symbol_raw = symbol_name.raw();

    account_table accounts(_self, from_group.value);
    const auto& from_account = accounts.require_find(symbol_raw, "not enough balance");

    map<uint64_t, token> from_tokens = from_account->tokens;
    name author = has_auth(to_group) ? to_group : from_group;
    for(auto iter = token_ids.begin(); iter != token_ids.end(); iter++)
    {
        uint64_t token_id = *iter;
        token from_token = from_tokens[token_id];
        
        // Always proceed to this flow.(sub -> add) because from_group can equal to_group
        sub_balance(from, from_group, symbol_raw, token_id);
        add_balance(to, to_group, author, symbol_name, from_token.group_id, token_id, from_token.token_name, from_token.options);
    }
}

ACTION itamdigasset::burn(string owner, name owner_group, symbol_code symbol_name, vector<uint64_t> token_ids, string reason)
{
    name payer = has_auth(owner_group) ? owner_group : _self;
    eosio_assert(has_auth(payer), "Unauthorized Error");
    uint64_t symbol_raw = symbol_name.raw();

    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");

    const auto& currency = currencies.require_find(symbol_raw, "Invalid symbol");
    currencies.modify(currency, payer, [&](auto &c) {
        c.supply -= asset(token_ids.size(), symbol(symbol_name, 0));
    });

    for(auto iter = token_ids.begin(); iter != token_ids.end(); iter++)
    {
        sub_balance(owner, owner_group, symbol_raw, *iter);
    }
}

ACTION itamdigasset::sellorder(string owner, name owner_group, symbol_code symbol_name, uint64_t token_id, asset price)
{
    eosio_assert(price.symbol == symbol("EOS", 4), "only eos symbol available");

    order_table orders(_self, symbol_name.raw());
    orders.emplace(owner_group, [&](auto &o) {
        o.token_id = token_id;
        o.owner = owner;
        o.owner_group = owner_group;
        o.price = price;
    });

    SEND_INLINE_ACTION(
        *this,
        transfernft,
        { { owner_group, name("active") } },
        { owner, owner_group, _self.to_string(), _self, symbol_name, vector<uint64_t> { token_id }, string("order") }
    );
}

ACTION itamdigasset::modifyorder(string owner, name owner_group, symbol_code symbol_name, uint64_t token_id, asset price)
{
    require_auth(owner_group);
    eosio_assert(price.symbol == symbol("EOS", 4), "only eos symbol available");
    
    order_table orders(_self, symbol_name.raw());
    const auto& order = orders.require_find(token_id, "not found token in orderbook");
    eosio_assert(order->owner == owner, "different owner");

    orders.modify(order, owner_group, [&](auto& o) {
        o.price = price;
    });
}

ACTION itamdigasset::cancelorder(string owner, name owner_group, symbol_code symbol_name, uint64_t token_id)
{
    order_table orders(_self, symbol_name.raw());
    const auto& order = orders.require_find(token_id, "not exists token id");

    require_auth(order->owner_group);
    eosio_assert(order->owner == owner, "different owner");

    SEND_INLINE_ACTION(
        *this,
        transfernft,
        { { _self, name("active") } },
        { _self.to_string(), _self, order->owner, order->owner_group, symbol_name, vector<uint64_t> { token_id }, string("cancel order") }
    );
    orders.erase(order);
}

ACTION itamdigasset::transfer(uint64_t from, uint64_t to)
{
    transfer_data data = unpack_action_data<transfer_data>();
    if (data.from == _self || data.to != _self) return;

    memo_data message;
    parseMemo(&message, data.memo, "|");

    symbol sym(message.symbol_name, 0);
    uint64_t token_id = stoull(message.token_id, 0, 10);
    
    order_table orders(_self, sym.code().raw());
    const auto& order = orders.require_find(token_id, "token not found in orders");
    eosio_assert(order->price == data.quantity, "wrong quantity");

    // TODO: save ratio in table
    uint64_t ratio = 15;

    // send price - fees to item owner
    action(
        permission_level{ _self, name("active") },
        name("eosio.token"),
        name("transfer"),
        make_tuple(_self, order->owner_group, order->price - (order->price * ratio / 100), string("test"))
    ).send();

    // // send nft token to buyer
    SEND_INLINE_ACTION(
        *this,
        transfernft,
        { { _self, name("active") } },
        { _self.to_string(), _self, message.buyer, data.from, sym.code(), vector<uint64_t> { token_id }, string("trade complete") }
    );

    orders.erase(order);
}

void itamdigasset::add_balance(const string& owner, name group_name, name ram_payer, symbol_code symbol_name, uint64_t group_id, uint64_t token_id, const string& token_name, const string& options)
{
    account_table accounts(_self, group_name.value);
    const auto& account = accounts.find(symbol_name.raw());

    token t { owner, group_id, token_name, options };
    if(account == accounts.end())
    {
        accounts.emplace(ram_payer, [&](auto &a) {
            a.balance.amount = 1;
            a.balance.symbol = symbol(symbol_name, 0);
            a.tokens[token_id] = t;
        });
    }
    else
    {
        accounts.modify(account, ram_payer, [&](auto &a) {
            eosio_assert(a.tokens.count(token_id) == 0, "already token id created");
            a.balance.amount += 1;
            a.tokens[token_id] = t;
        });
    }
}

void itamdigasset::sub_balance(const string& owner, name group_name, uint64_t symbol_raw, uint64_t token_id)
{
    account_table accounts(_self, group_name.value);
    const auto& account = accounts.require_find(symbol_raw, "not enough balance");
    map<uint64_t, token> tokens = account->tokens;

    const auto& token = tokens.find(token_id);
    eosio_assert(token != tokens.end(), "no item object found");
    eosio_assert(token->second.owner == owner, "different owner");

    accounts.modify(account, group_name, [&](auto &a) {
        a.balance.amount -= 1;
        a.tokens.erase(token_id);
    });
}