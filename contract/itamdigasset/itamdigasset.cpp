#include "itamdigasset.hpp"

ACTION itamdigasset::create(name issuer, symbol_code symbol_name, uint64_t app_id)
{
    require_auth(_self);

    symbol game_symbol(symbol_name, 0);
    
    eosio_assert(game_symbol.is_valid(), "Invalid symbol");
    eosio_assert(is_account(issuer), "Invalid issuer");

    const auto& currency = currencies.find(symbol_name.raw());
    eosio_assert(currency == currencies.end(), "Already item created");

    currencies.emplace(_self, [&](auto &c) {
        c.issuer = issuer;
        c.supply = asset(0, game_symbol);
        c.app_id = app_id;
        c.sequence = 0;
    });
}

ACTION itamdigasset::issue(string to, name to_group, string nickname, symbol_code symbol_name, uint64_t group_id, string item_name, string category, string options, string reason)
{
    const auto& currency = currencies.require_find(symbol_name.raw(), "invalid symbol");
    require_auth(currency->issuer);
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");

    currencies.modify(currency, _self, [&](auto &c) {
        c.supply.amount += 1;
        c.sequence += 1;
    });

    uint64_t item_id = currency->sequence;

    string stringified_self = _self.to_string();
    item t { stringified_self, nickname, group_id, item_name, category, options, true };
    add_balance(_self, _self, symbol_name, item_id, t);

    name eos_name = name("eos");
    if(to != stringified_self || to_group != eos_name)
    {
        vector<uint64_t> item_ids {item_id};
        SEND_INLINE_ACTION(
            *this,
            transfernft,
            { {_self, name("active")} },
            { stringified_self, eos_name, to, to_group, nickname, symbol_name, item_ids, reason }
        );
    }
}

ACTION itamdigasset::modify(string owner, name owner_group, symbol_code symbol_name, uint64_t item_id, uint64_t group_id, string item_name, string category, string options, bool transferable, string reason)
{
    require_auth(_self);
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");
    
    uint64_t symbol_raw = symbol_name.raw();
    const auto& currency = currencies.require_find(symbol_raw, "Invalid symbol");

    name group_account = get_group_account(owner, owner_group);

    account_table accounts(_self, group_account.value);
    const auto& account = accounts.require_find(symbol_raw, "not enough balance");

    map<uint64_t, item> owner_items = account->items;
    auto iter = owner_items.find(item_id);
    eosio_assert(iter != owner_items.end(), "no item object found");

    item owner_item = iter->second;
    eosio_assert(owner_item.owner == owner, "different owner");

    accounts.modify(account, _self, [&](auto &a) {
        auto& item = a.items[item_id];
        item.group_id = group_id;
        item.item_name = item_name;
        item.category = category;
        item.options = options;
        item.transferable = transferable;
    });    
}

ACTION itamdigasset::transfernft(string from, name from_group, string to, name to_group, string to_nickname, symbol_code symbol_name, vector<uint64_t> item_ids, string memo)
{
    name from_group_account = get_group_account(from, from_group);
    require_auth(from_group_account);

    allow_table allows(_self, _self.value);
    eosio_assert(from_group_account == _self || allows.find(from_group_account.value) != allows.end(), "only allowed accounts can use this action");

    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");
    
    name to_group_account = get_group_account(to, to_group);
    eosio_assert(is_account(to_group_account), "to_group_account does not exist");
    
    uint64_t symbol_raw = symbol_name.raw();

    account_table accounts(_self, from_group_account.value);
    const auto& from_account = accounts.require_find(symbol_raw, "not enough balance");

    map<uint64_t, item> from_items = from_account->items;
    name author = has_auth(to_group_account) ? to_group_account : from_group_account;

    for(auto iter = item_ids.begin(); iter != item_ids.end(); iter++)
    {
        uint64_t item_id = *iter;

        item t = from_items[item_id];
        eosio_assert(t.owner == from, "different from owner");
        eosio_assert(t.transferable, "not transferable item");

        t.owner = to;
        t.nickname = to_nickname;
        t.transferable = false;

        // Always proceed to this flow.(sub -> add) because from_group can equal to_group
        sub_balance(from, from_group_account, from_group_account, symbol_raw, item_id);
        add_balance(to_group_account, author, symbol_name, item_id, t);
    }

    require_recipient(from_group_account, to_group_account);
}

ACTION itamdigasset::burn(string owner, name owner_group, symbol_code symbol_name, vector<uint64_t> item_ids, string reason)
{
    name group_account = get_group_account(owner, owner_group);
    name author = has_auth(group_account) ? group_account : _self;
    require_auth(author);
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");

    uint64_t symbol_raw = symbol_name.raw();
    const auto& currency = currencies.require_find(symbol_raw, "Invalid symbol");
    currencies.modify(currency, _self, [&](auto &c) {
        c.supply -= asset(item_ids.size(), symbol(symbol_name, 0));
    });

    for(auto iter = item_ids.begin(); iter != item_ids.end(); iter++)
    {
        sub_balance(owner, group_account, author, symbol_raw, *iter);
    }
}

ACTION itamdigasset::sellorder(string owner, name owner_group, symbol_code symbol_name, uint64_t item_id, asset price)
{
    eosio_assert(price.symbol == symbol("EOS", 4), "only eos symbol available");

    name group_account = get_group_account(owner, owner_group);
    require_auth(group_account);
    
    account_table accounts(_self, group_account.value);
    const auto& account = accounts.get(symbol_name.raw(), "item not found");
    
    map<uint64_t, item> owner_items = account.items;
    item owner_item = owner_items[item_id];

    order_table orders(_self, symbol_name.raw());
    orders.emplace(group_account, [&](auto &o) {
        o.item_id = item_id;
        o.owner = owner;
        o.owner_group = owner_group;
        o.nickname = owner_item.nickname;
        o.price = price;
    });

    eosio_assert(owner_item.transferable, "not transferable item");
    owner_item.owner = _self.to_string();
    
    // transfernft owner to itam contract
    sub_balance(owner, group_account, group_account, symbol_name.raw(), item_id);
    add_balance(_self, group_account, symbol_name, item_id, owner_item);
}

ACTION itamdigasset::modifyorder(string owner, name owner_group, symbol_code symbol_name, uint64_t item_id, asset price)
{
    name group_account = get_group_account(owner, owner_group);
    require_auth(group_account);
    eosio_assert(price.symbol == symbol("EOS", 4), "only eos symbol available");
    
    order_table orders(_self, symbol_name.raw());
    const auto& order = orders.require_find(item_id, "not found item in orderbook");
    eosio_assert(order->owner == owner, "different owner");

    orders.modify(order, group_account, [&](auto& o) {
        o.price = price;
    });
}

ACTION itamdigasset::cancelorder(string owner, name owner_group, symbol_code symbol_name, uint64_t item_id)
{
    order_table orders(_self, symbol_name.raw());
    const auto& order = orders.require_find(item_id, "not exists item id");

    name group_account = get_group_account(owner, owner_group);

    require_auth(group_account);
    eosio_assert(order->owner == owner, "different owner");

    account_table accounts(_self, _self.value);
    const auto& self_account = accounts.get(symbol_name.raw(), "not enough balance");

    map<uint64_t, item> self_items = self_account.items;
    item t = self_items[item_id];
    
    t.owner = owner;
    t.transferable = false;

    sub_balance(_self.to_string(), _self, group_account, symbol_name.raw(), item_id);
    add_balance(group_account, group_account, symbol_name, item_id, t);

    orders.erase(order);
}

ACTION itamdigasset::transfer(uint64_t from, uint64_t to)
{
    transfer_data data = unpack_action_data<transfer_data>();
    if (data.from == _self || data.to != _self) return;

    memo_data message;
    parseMemo(&message, data.memo, "|");

    symbol sym(message.symbol_name, 0);
    uint64_t item_id = stoull(message.item_id, 0, 10);
    
    order_table orders(_self, sym.code().raw());
    const auto& order = orders.require_find(item_id, "item not found in orders");
    eosio_assert(order->price == data.quantity, "wrong quantity");

    name owner_group_account = get_group_account(order->owner, order->owner_group);
    
    config_table configs(_self, _self.value);
    const auto& config = configs.get(_self.value, "config must be set");

    asset fees = order->price * config.fees_rate / 100;

    // send price - fees to item owner
    action(
        permission_level{ _self, name("active") },
        name("eosio.token"),
        name("transfer"),
        make_tuple(_self, owner_group_account, order->price - fees, string("trade complete"))
    ).send();

    asset settleAmountToITAM = fees;

    // increment settle amount
    settle_table settles(_self, _self.value);
    const auto& settle = settles.find(sym.code().raw());
    if(settle != settles.end())
    {
        settles.modify(settle, _self, [&](auto &s) {
            s.settle_amount += fees * config.settle_rate / 100;
        });

        settleAmountToITAM -= fees * config.settle_rate / 100;
    }

    action(
        permission_level{ _self, name("active") },
        name("eosio.token"),
        name("transfer"),
        make_tuple(_self, name(ITAM_SETTLE_ACCOUNT), settleAmountToITAM, string("dex fees"))
    ).send();

    // send nft item to buyer
    SEND_INLINE_ACTION(
        *this,
        transfernft,
        { { _self, name("active") } },
        { _self.to_string(), name("eos"), message.buyer, name(message.buyer_group), message.buyer_nickname, sym.code(), vector<uint64_t> { item_id }, string("trade complete") }
    );

    orders.erase(order);
}

ACTION itamdigasset::setsettle(symbol_code symbol_name, name account)
{
    require_auth(_self);
    eosio_assert(is_account(account), "account does not exist");

    const auto& currency = currencies.get(symbol_name.raw(), "invalid symbol");

    settle_table settles(_self, _self.value);
    const auto& settle = settles.find(symbol_name.raw());

    if(settle == settles.end())
    {
        settles.emplace(_self, [&](auto &s) {
            s.sym = symbol_name;
            s.account = account;
            s.settle_amount = asset(0, symbol("EOS", 4));
        });
    }
    else
    {
        settles.modify(settle, _self, [&](auto &s) {
            s.account = account;
        }); 
    }
}

ACTION itamdigasset::claimsettle(symbol_code symbol_name)
{
    require_auth(_self);

    settle_table settles(_self, _self.value);
    const auto& settle = settles.get(symbol_name.raw(), "settle account does not exist");

    action(
        permission_level{ _self, name("active") },
        name("eosio.token"),
        name("transfer"),
        make_tuple( _self, settle.account, settle.settle_amount, string("itamgames dex fees") )
    ).send();
}

void itamdigasset::add_balance(name group_account, name ram_payer, symbol_code symbol_name, uint64_t item_id, const item& t)
{
    account_table accounts(_self, group_account.value);
    const auto& account = accounts.find(symbol_name.raw());

    if(account == accounts.end())
    {
        accounts.emplace(ram_payer, [&](auto &a) {
            a.balance.amount = 1;
            a.balance.symbol = symbol(symbol_name, 0);
            a.items[item_id] = t; 
        });
    }
    else
    {
        accounts.modify(account, ram_payer, [&](auto &a) {
            eosio_assert(a.items.count(item_id) == 0, "already item id created");
            a.balance.amount += 1;
            a.items[item_id] = t;
        });
    }
    
}

void itamdigasset::sub_balance(const string& owner, name group_account, name ram_payer, uint64_t symbol_raw, uint64_t item_id)
{
    account_table accounts(_self, group_account.value);
    const auto& account = accounts.require_find(symbol_raw, "not enough balance");
    map<uint64_t, item> items = account->items;

    const auto& item = items.find(item_id);
    eosio_assert(item != items.end(), "no item object found");
    eosio_assert(item->second.owner == owner, "different owner");

    accounts.modify(account, ram_payer, [&](auto &a) {
        a.balance.amount -= 1;
        a.items.erase(item_id);
    });
}

ACTION itamdigasset::modifygroup(name before_group_account, name after_group_account)
{
    require_auth(_self);

    account_table before_accounts(_self, before_group_account.value);
    account_table after_accounts(_self, after_group_account.value);

    for(auto account = before_accounts.begin(); account != before_accounts.end(); account = before_accounts.erase(account))
    {
        after_accounts.emplace(_self, [&](auto &a) {
            a.balance = account->balance;
            a.items = account->items;
        });
    }
}

ACTION itamdigasset::addwhitelist(name allow_account)
{
    require_auth(_self);

    allow_table allows(_self, _self.value);
    allows.emplace(_self, [&](auto &a) {
        a.account = allow_account;
    });
}

ACTION itamdigasset::setconfig(uint64_t fees_rate, uint64_t settle_rate)
{
    require_auth(_self);

    config_table configs(_self, _self.value);
    const auto& config = configs.find(_self.value);

    if(config == configs.end())
    {
        configs.emplace(_self, [&](auto &c) {
            c.key = _self;
            c.fees_rate = fees_rate;
            c.settle_rate = settle_rate;
        });
    }
    else
    {
        configs.modify(config, _self, [&](auto &c) {
            c.fees_rate = fees_rate;
            c.settle_rate = settle_rate;
        });
    }
}