#include "itamstorenft.hpp"

ACTION itamstorenft::nft(name owner, name owner_group, string game_user_id, symbol_code symbol_name, string action, string reason)
{
    require_auth(_self);
    currencies.get(symbol_name.raw(), "invalid symbol");
}

ACTION itamstorenft::activate(name owner, name owner_group, string game_user_id, string nickname, symbol_code symbol_name, string item_id, string item_name, string group_id, string options, uint64_t duration, bool transferable, string reason)
{
    _issue(owner, owner_group, nickname, symbol_name, item_id, item_name, group_id, options, duration, transferable, reason);
}

ACTION itamstorenft::deactivate(name owner, name owner_group, symbol_code symbol_name, string item_id, string reason)
{
    _burn(owner, owner_group, symbol_name, item_id, reason);
}

ACTION itamstorenft::create(name issuer, symbol_code symbol_name, string app_id)
{
    require_auth(_self);
    symbol game_symbol(symbol_name, 0);
    eosio_assert(game_symbol.is_valid(), "Invalid symbol");
    eosio_assert(is_account(issuer), "Invalid issuer");

    const auto& currency = currencies.find(symbol_name.raw());
    eosio_assert(currency == currencies.end(), "Already symbol created");

    currencies.emplace(_self, [&](auto &c) {
        c.issuer = issuer;
        c.supply = asset(0, game_symbol);
        c.app_id = stoull(app_id, 0, 10);
    });
}

ACTION itamstorenft::issue(name to, name to_group, string nickname, symbol_code symbol_name, string item_id, string item_name, string group_id, string options, uint64_t duration, bool transferable, string reason)
{
    _issue(to, to_group, nickname, symbol_name, item_id, item_name, group_id, options, duration, transferable, reason);
}

void itamstorenft::_issue(name to, name to_group, string nickname, symbol_code symbol_name, string item_id, string item_name, string group_id, string options, uint64_t duration, bool transferable, string reason)
{
    require_auth(_self);
    const auto& currency = currencies.require_find(symbol_name.raw(), "invalid symbol");
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");

    currencies.modify(currency, _self, [&](auto &c) {
        c.supply.amount += 1;
    });

    name to_account = get_group_account(to, to_group);
    eosio_assert(is_account(to_account), "to account does not exists");

    uint64_t itemid = stoull(item_id, 0, 10);
    uint64_t groupid = stoull(group_id, 0, 10);

    account_table accounts(_self, symbol_name.raw());
    const auto& account = accounts.find(itemid);
    eosio_assert(account == accounts.end(), "already item id");
    accounts.emplace(currency->issuer, [&](auto &a) {
        a.item_id = itemid;
        a.item_name = item_name;
        a.group_id = groupid;
        a.duration = duration;
        a.options = options;
        a.transferable = transferable;
        a.owner = to;
        a.owner_account = to_account;
        a.nickname = nickname;
    });
    
    SEND_INLINE_ACTION(
        *this,
        receipt,
        { { _self, name("active") } },
        { to, to_group, currency->app_id, itemid, nickname, groupid, item_name, options, duration, transferable, asset(), string("issue") }
    );
}

ACTION itamstorenft::modify(name owner, name owner_group, symbol_code symbol_name, string item_id, string item_name, string options, uint64_t duration, bool transferable, string reason)
{
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");
    
    uint64_t symbol_raw = symbol_name.raw();
    const auto& currency = currencies.require_find(symbol_raw, "invalid symbol");
    require_auth(currency->issuer);

    uint64_t itemid = stoull(item_id, 0, 10);
    account_table accounts(_self, symbol_name.raw());
    const auto& account = accounts.require_find(itemid, "invalid item");

    name owner_account = get_group_account(owner, owner_group);
    eosio_assert(account->owner == owner && account->owner_account == owner_account, "wrong item owner");

    accounts.modify(account, _self, [&](auto &a) {
        a.item_name = item_name;
        a.options = options;
        a.duration = duration;
        a.transferable = transferable;
    });

    SEND_INLINE_ACTION(
        *this,
        receipt,
        { { _self, name("active") } },
        { owner, owner_group, currency->app_id, itemid, account->nickname, account->group_id, item_name, options, duration, transferable, asset(), string("modify") }
    );
}

ACTION itamstorenft::changegroup(symbol_code symbol_name, string item_id, string group_id)
{
    require_auth(_self);

    uint64_t itemid = stoull(item_id, 0, 10);
    uint64_t groupid = stoull(group_id, 0, 10);

    account_table accounts(_self, symbol_name.raw());
    const auto& account = accounts.require_find(itemid, "invalid item");

    accounts.modify(account, _self, [&](auto& a) {
        a.group_id = groupid;
    });
}

ACTION itamstorenft::changeowner(symbol_code symbol_name, string item_id, name owner, name owner_group, string nickname)
{
    require_auth(_self);

    account_table accounts(_self, symbol_name.raw());
    const auto& account = accounts.require_find(stoull(item_id, 0, 10), "invalid item");

    accounts.modify(account, _self, [&](auto &a) {
        a.owner = owner;
        a.owner_account = get_group_account(owner, owner_group);
        a.nickname = nickname;
    });
}

ACTION itamstorenft::burn(name owner, name owner_group, symbol_code symbol_name, string item_id, string reason)
{
    _burn(owner, owner_group, symbol_name, item_id, reason);
}

void itamstorenft::_burn(name owner, name owner_group, symbol_code symbol_name, string item_id, string reason)
{
    require_auth(_self);
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");

    uint64_t symbol_raw = symbol_name.raw();
    const auto& currency = currencies.require_find(symbol_raw, "Invalid symbol");
    currencies.modify(currency, _self, [&](auto &c) {
        c.supply.amount -= 1;
    });

    account_table accounts(_self, symbol_raw);
    uint64_t itemid = stoull(item_id, 0, 10);
    const auto& account = accounts.require_find(itemid, "invalid item");

    SEND_INLINE_ACTION(
        *this,
        receipt,
        { { _self, name("active") } },
        {
            owner,
            owner_group,
            currency->app_id,
            itemid,
            account->nickname,
            account->group_id,
            account->item_name,
            account->options,
            account->duration,
            account->transferable,
            asset(),
            string("burn")
        }
    );

    name owner_account = get_group_account(owner, owner_group);
    eosio_assert(account->owner == owner && account->owner_account == owner_account, "wrong item owner");
    accounts.erase(account);
}

ACTION itamstorenft::burnall(symbol_code symbol)
{
    const auto& currency = currencies.require_find(symbol.raw(), "Invalid symbol");
    require_auth(currency->issuer);
    currencies.modify(currency, _self, [&](auto &c) {
        c.supply.amount = 0;
    });
    account_table accounts(_self, symbol.raw());
    for(auto account = accounts.begin(); account != accounts.end(); account = accounts.erase(account));
}

ACTION itamstorenft::transfernft(name from, name to, symbol_code symbol_name, string item_id, string memo)
{
    eosio_assert(is_account(to), "to account does not exist");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    uint64_t itemid = stoull(item_id, 0, 10);
    account_table accounts(_self, symbol_name.raw());
    const auto& from_item = accounts.require_find(itemid, "invalid item id");
    
    eosio_assert(from_item->transferable, "not transferable item");
    eosio_assert(from_item->duration == 0 || from_item->duration <= now(), "overdrawn duration");

    name author;
    name dex_contract(DEX_CONTRACT);

    if(has_auth(dex_contract))
    {
        author = dex_contract;
    }
    else
    {
        author = has_auth(from) ? from : name(ITAM_GROUP_ACCOUNT);
        require_auth(author);

        eosio_assert(from_item->owner == from && from_item->owner_account == author, "wrong item owner");
    }

    allow_table allows(_self, _self.value);
    eosio_assert(allows.find(author.value) != allows.end(), "only allowed accounts can use this action");

    if(symbol_name.to_string() != "DARKT")
    {
        accounts.erase(from_item);
    }
    else
    {
        memo_data nft_memo;
        parseMemo(&nft_memo, memo, "|", 3);

        accounts.modify(from_item, author, [&](auto &a) {
            a.owner = name(nft_memo.to);
            a.owner_account = to;
            a.nickname = nft_memo.nickname;
            a.transferable = nft_memo.transfer_state != string("complete");
        });
    }

    require_recipient(from_item->owner_account, to);
}

ACTION itamstorenft::addwhitelist(name account)
{
    require_auth(_self);

    allow_table allows(_self, _self.value);
    eosio_assert(allows.find(account.value) == allows.end(), "already whitelist exists");
    allows.emplace(_self, [&](auto &a) {
        a.account = account;
    });
}

ACTION itamstorenft::delwhitelist(name account)
{
    require_auth(_self);

    allow_table allows(_self, _self.value);
    const auto& allow = allows.require_find(account.value);
    allows.erase(allow);
}

ACTION itamstorenft::receipt(name owner, name owner_group, uint64_t app_id, uint64_t item_id, string nickname, uint64_t group_id, string item_name, string options, uint64_t duration, bool transferable, asset payment_quantity, string state)
{
    require_auth(_self);

    name account = get_group_account(owner, owner_group);
    require_recipient(_self, account);
}