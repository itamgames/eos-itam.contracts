#include "itamstoredex.hpp"

ACTION itamstoredex::create(name issuer, symbol_code symbol_name, string app_id)
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
        c.app_id = stoull(app_id, 0, 10);
        c.sequence = 0;
    });
}

ACTION itamstoredex::issue(string to, name to_group, string nickname, symbol_code symbol_name, string item_id, string item_name, string category, string group_id, string options, uint64_t duration, bool transferable, string reason)
{
    const auto& currency = currencies.require_find(symbol_name.raw(), "invalid symbol");
    require_auth(currency->issuer);
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");

    currencies.modify(currency, _self, [&](auto &c) {
        c.supply.amount += 1;
    });

    uint64_t itemid = stoull(item_id, 0, 10);
    uint64_t groupid = stoull(group_id, 0, 10);

    item issuing_item { to, nickname, groupid, item_name, category, options, duration, transferable };

    name to_account = get_group_account(to, to_group);
    eosio_assert(is_account(to_account), "to account does not exists");
    
    add_balance(to_account, _self, symbol_name, itemid, issuing_item);

    SEND_INLINE_ACTION(
        *this,
        receipt,
        { { _self, name("active") } },
        { to, to_group, currency->app_id, itemid, nickname, groupid, item_name, category, options, duration, transferable, string("issue") }
    );
}

ACTION itamstoredex::modify(string owner, name owner_group, symbol_code symbol_name, string item_id, string item_name, string options, uint64_t duration, bool transferable, string reason)
{
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");
    
    uint64_t symbol_raw = symbol_name.raw();
    const auto& currency = currencies.require_find(symbol_raw, "invalid symbol");
    require_auth(currency->issuer);

    name owner_account = get_group_account(owner, owner_group);

    account_table accounts(_self, owner_account.value);
    const auto& account = accounts.require_find(symbol_raw, "not enough balance");

    uint64_t itemid = stoull(item_id, 0, 10);

    item owner_item = get_owner_item(owner, account->items, itemid);

    accounts.modify(account, _self, [&](auto &a) {
        auto& item = a.items[itemid];
        item.item_name = item_name;
        item.options = options;
        item.duration = duration;
        item.transferable = transferable;
    });

    SEND_INLINE_ACTION(
        *this,
        receipt,
        { { _self, name("active") } },
        { owner, owner_group, currency->app_id, itemid, owner_item.nickname, owner_item.group_id, item_name, owner_item.category, options, duration, transferable, string("modify") }
    );
}

ACTION itamstoredex::transfernft(name from, string to, symbol_code symbol_name, vector<string> item_ids, string memo)
{
    name from_group_account = has_auth(from) ? from : name(ITAM_GROUP_ACCOUNT);

    transfernft_memo nft_memo;
    parseMemo(&nft_memo, memo, "|", 2);
    
    name to_group_account = get_group_account(to, name(nft_memo.to_group));

    require_auth(from_group_account);
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");
    eosio_assert(is_account(to_group_account), "to_group_account does not exist");

    allow_table allows(_self, _self.value);
    eosio_assert(from == _self || allows.find(from_group_account.value) != allows.end(), "only allowed accounts can use this action");
    
    uint64_t symbol_raw = symbol_name.raw();

    account_table accounts(_self, from_group_account.value);
    const auto& from_account = accounts.require_find(symbol_raw, "not enough balance");

    const map<uint64_t, item>& from_items = from_account->items;
    uint64_t current_sec = now();
    string from_string = from.to_string();
    for(auto iter = item_ids.begin(); iter != item_ids.end(); iter++)
    {
        uint64_t item_id = stoull(*iter, 0, 10);

        auto from_item = from_items.find(item_id);
        eosio_assert(from_item != from_items.end(), "cannot found item id");
        
        item transferring_item = from_item->second;
        eosio_assert(transferring_item.owner == from_string, "different from owner");
        eosio_assert(transferring_item.transferable, "not transferable item");
        eosio_assert(transferring_item.duration == 0 || transferring_item.duration <= current_sec, "overdrawn duration");
        
        transferring_item.owner = to;
        transferring_item.nickname = nft_memo.nickname;
        
        // Always proceed to this flow.(sub -> add) because from_group can equal to_group
        sub_balance(from_string, from_group_account, from_group_account, symbol_raw, item_id);
        add_balance(to_group_account, from_group_account, symbol_name, item_id, transferring_item);
    }

    require_recipient(from_group_account, to_group_account);
}

ACTION itamstoredex::burn(string owner, name owner_group, symbol_code symbol_name, vector<string> item_ids, string reason)
{
    name owner_account = get_group_account(owner, owner_group);
    name author = has_auth(owner_account) ? owner_account : _self;
    require_auth(author);

    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");

    uint64_t symbol_raw = symbol_name.raw();
    const auto& currency = currencies.require_find(symbol_raw, "Invalid symbol");
    currencies.modify(currency, _self, [&](auto &c) {
        c.supply -= asset(item_ids.size(), symbol(symbol_name, 0));
    });

    account_table accounts(_self, owner_account.value);
    const auto& account = accounts.require_find(symbol_raw, "not enough balance");
    const map<uint64_t, item>& items = account->items;

    for(auto iter = item_ids.begin(); iter != item_ids.end(); iter++)
    {
        uint64_t item_id = stoull(*iter, 0, 10);
        auto owner_item = items.find(item_id);
        eosio_assert(owner_item != items.end(), "cannot found item id");
        
        item burning_item = owner_item->second;

        SEND_INLINE_ACTION(
            *this,
            receipt,
            { { _self, name("active") } },
            {
                owner,
                owner_group,
                currency->app_id,
                item_id,
                burning_item.nickname,
                burning_item.group_id,
                burning_item.item_name,
                burning_item.category,
                burning_item.options,
                burning_item.duration,
                burning_item.transferable,
                string("burn")
            }
        );

        sub_balance(owner, owner_account, author, symbol_raw, item_id);
    }
}

ACTION itamstoredex::sellorder(string owner, symbol_code symbol_name, string item_id, asset quantity)
{
    eosio_assert(quantity.symbol == symbol("EOS", 4), "only eos symbol available");
    name owner_name(owner);

    name owner_group_name;
    name owner_group_account;
    set_owner_group(name(owner), owner_group_name, owner_group_account);
    
    account_table accounts(_self, owner_group_account.value);
    const auto& account = accounts.get(symbol_name.raw(), "item not found");
    
    uint64_t itemid = stoull(item_id, 0, 10);
    item owner_item = get_owner_item(owner, account.items, itemid);
    // eosio_assert(owner_item.transferable, "not transferable item");

    order_table orders(_self, symbol_name.raw());
    orders.emplace(owner_group_account, [&](auto &o) {
        o.item_id = itemid;
        o.owner = owner;
        o.owner_group = owner_group_name;
        o.nickname = owner_item.nickname;
        o.quantity = quantity;
    });

    owner_item.owner = _self.to_string();
    
    // transfernft owner to itam contract
    sub_balance(owner, owner_group_account, owner_group_account, symbol_name.raw(), itemid);
    add_balance(_self, owner_group_account, symbol_name, itemid, owner_item);
}

ACTION itamstoredex::modifyorder(string owner, symbol_code symbol_name, string item_id, asset quantity)
{
    name owner_group_name;
    name owner_group_account;
    set_owner_group(name(owner), owner_group_name, owner_group_account);

    eosio_assert(quantity.symbol == symbol("EOS", 4), "only eos symbol available");
    
    order_table orders(_self, symbol_name.raw());
    const auto& order = orders.require_find(stoull(item_id, 0, 10), "not found item in orderbook");
    eosio_assert(order->owner == owner, "different owner");
    eosio_assert(order->owner_group == owner_group_name, "different owner group");

    orders.modify(order, owner_group_account, [&](auto& o) {
        o.quantity = quantity;
    });
}

ACTION itamstoredex::cancelorder(string owner, symbol_code symbol_name, string item_id)
{
    uint64_t itemid = stoull(item_id, 0, 10);
    order_table orders(_self, symbol_name.raw());
    const auto& order = orders.require_find(itemid, "not exists item id");

    name owner_group_name;
    name owner_group_account;
    set_owner_group(name(owner), owner_group_name, owner_group_account);

    eosio_assert(order->owner == owner, "different owner");
    eosio_assert(order->owner_group == owner_group_name, "different owner group name");

    account_table accounts(_self, _self.value);
    const auto& self_account = accounts.get(symbol_name.raw(), "not enough balance");
    
    item cancel_item = get_owner_item(_self.to_string(), self_account.items, itemid);
    cancel_item.owner = owner;

    sub_balance(_self.to_string(), _self, owner_group_account, symbol_name.raw(), itemid);
    add_balance(owner_group_account, owner_group_account, symbol_name, itemid, cancel_item);

    orders.erase(order);
}

ACTION itamstoredex::transfer(uint64_t from, uint64_t to)
{
    transfer_data data = unpack_action_data<transfer_data>();
    if (data.from == _self || data.to != _self || data.from == name("itamgamesgas")) return;

    transfer_memo message;
    parseMemo(&message, data.memo, "|", 4);

    symbol sym(message.symbol_name, 0);
    uint64_t item_id = stoull(message.item_id, 0, 10);
    
    order_table orders(_self, sym.code().raw());
    const auto& order = orders.require_find(item_id, "item not found in orders");
    eosio_assert(order->quantity == data.quantity, "wrong quantity");

    name owner_group_account = get_group_account(order->owner, order->owner_group);
    
    config_table configs(_self, _self.value);
    const auto& config = configs.get(_self.value, "config must be set");

    asset fees = order->quantity * config.fees_rate / 100;

    // send price - fees to item owner
    action(
        permission_level{ _self, name("active") },
        name("eosio.token"),
        name("transfer"),
        make_tuple(_self, owner_group_account, order->quantity - fees, string("trade complete"))
    ).send();

    asset settle_quantity_to_vendor = fees * config.settle_rate / 100;
    asset settle_quantity_to_itam = fees - settle_quantity_to_vendor;
    const auto& currency = currencies.get(sym.code().raw(), "invalid symbol");

    if(settle_quantity_to_vendor.amount > 0)
    {
        action(
            permission_level{ _self, name("active") },
            name("eosio.token"),
            name("transfer"),
            make_tuple( _self, name(CENTRAL_SETTLE_ACCOUNT), settle_quantity_to_vendor, to_string(currency.app_id) )
        ).send();
    }

    if(settle_quantity_to_itam.amount > 0)
    {
        action(
            permission_level{ _self, name("active") },
            name("eosio.token"),
            name("transfer"),
            make_tuple( _self, name(ITAM_SETTLE_ACCOUNT), settle_quantity_to_itam, to_string(currency.app_id) )
        ).send();
    }

    // send nft item to buyer
    string owner_group_name = data.from == name(ITAM_GROUP_ACCOUNT) ? "itam" : "eos";
    string nft_memo_data = message.buyer_nickname + string("|") + owner_group_name;

    SEND_INLINE_ACTION(
        *this,
        transfernft,
        { { _self, name("active") } },
        { _self, message.owner, symbol(message.symbol_name, 0).code(), vector<string> { message.item_id }, nft_memo_data }
    );

    orders.erase(order);
}

ACTION itamstoredex::addwhitelist(name account)
{
    require_auth(_self);

    allow_table allows(_self, _self.value);
    eosio_assert(allows.find(account.value) == allows.end(), "already whitelist exists");
    allows.emplace(_self, [&](auto &a) {
        a.account = account;
    });
}

ACTION itamstoredex::delwhitelist(name account)
{
    require_auth(_self);

    allow_table allows(_self, _self.value);
    const auto& allow = allows.require_find(account.value);
    allows.erase(allow);
}

ACTION itamstoredex::setconfig(uint64_t fees_rate, uint64_t settle_rate)
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

ACTION itamstoredex::receipt(string owner, name owner_group, uint64_t app_id, uint64_t item_id, string nickname, uint64_t group_id, string item_name, string category, string options, uint64_t duration, bool transferable, string state)
{
    require_auth(_self);

    name account = get_group_account(owner, owner_group);
    require_recipient(_self, account);
}

void itamstoredex::add_balance(const name& group_account, const name& ram_payer, const symbol_code& symbol_name, uint64_t item_id, const item& i)
{
    account_table accounts(_self, group_account.value);
    const auto& account = accounts.find(symbol_name.raw());

    if(account == accounts.end())
    {
        accounts.emplace(ram_payer, [&](auto &a) {
            a.balance.amount = 1;
            a.balance.symbol = symbol(symbol_name, 0);
            a.items[item_id] = i; 
        });
    }
    else
    {
        accounts.modify(account, ram_payer, [&](auto &a) {
            eosio_assert(a.items.count(item_id) == 0, "already item id created");
            a.balance.amount += 1;
            a.items[item_id] = i;
        });
    }
    
}

void itamstoredex::sub_balance(const string& owner, const name& group_account, const name& ram_payer, uint64_t symbol_raw, uint64_t item_id)
{
    account_table accounts(_self, group_account.value);
    const auto& account = accounts.require_find(symbol_raw, "not enough balance");
    const auto& item = get_owner_item(owner, account->items, item_id);
    
    accounts.modify(account, ram_payer, [&](auto &a) {
        a.balance.amount -= 1;
        a.items.erase(item_id);
    });
}

itamstoredex::item itamstoredex::get_owner_item(const string& owner, const map<uint64_t, item>& owner_items, uint64_t item_id)
{
    auto owner_item = owner_items.find(item_id);
    eosio_assert(owner_item != owner_items.end(), "cannot found item id");
    eosio_assert(owner_item->second.owner == owner, "different owner");
    
    return owner_item->second;
}

void itamstoredex::set_owner_group(const name& owner, name& owner_group_name, name& owner_group_account)
{
    if(has_auth(owner))
    {
        owner_group_name = name("eos");
        owner_group_account = owner;
    }
    else
    {
        owner_group_name = name("itam");
        owner_group_account = name(ITAM_GROUP_ACCOUNT);
        require_auth(owner_group_account);
    }
}