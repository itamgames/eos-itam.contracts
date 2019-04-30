#include "itamstorenft.hpp"

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

ACTION itamstorenft::issue(name to, name to_group, string nickname, symbol_code symbol_name, string item_id, string item_name, string category, string group_id, string options, uint64_t duration, bool transferable, string reason)
{
    const auto& currency = currencies.require_find(symbol_name.raw(), "invalid symbol");
    require_auth(currency->issuer);
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");

    currencies.modify(currency, _self, [&](auto &c) {
        c.supply.amount += 1;
    });

    name to_account = get_group_account(to, to_group);
    eosio_assert(is_account(to_account), "to account does not exists");

    uint64_t itemid = stoull(item_id, 0, 10);
    uint64_t groupid = stoull(group_id, 0, 10);

    account item { itemid, item_name, groupid, category, duration, options, transferable, to, to_account, nickname };
    
    add_balance(symbol_name.raw(), currency->issuer, item);

    SEND_INLINE_ACTION(
        *this,
        receipt,
        { { _self, name("active") } },
        { to, to_group, currency->app_id, itemid, nickname, groupid, item_name, category, options, duration, transferable, asset(), string("issue") }
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
    eosio_assert(account->owner == owner && account->account == owner_account, "wrong item owner");

    accounts.modify(account, same_payer, [&](auto &a) {
        a.item_name = item_name;
        a.options = options;
        a.duration = duration;
        a.transferable = transferable;
    });

    SEND_INLINE_ACTION(
        *this,
        receipt,
        { { _self, name("active") } },
        { owner, owner_group, currency->app_id, itemid, account->nickname, account->group_id, item_name, account->category, options, duration, transferable, asset(), string("modify") }
    );
}

ACTION itamstorenft::burn(name owner, name owner_group, symbol_code symbol_name, vector<string> item_ids, string reason)
{
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");

    uint64_t symbol_raw = symbol_name.raw();
    const auto& currency = currencies.require_find(symbol_raw, "Invalid symbol");

    name owner_account = get_group_account(owner, owner_group);
    name author = has_auth(owner_account) ? owner_account : currency->issuer;
    require_auth(author);

    currencies.modify(currency, _self, [&](auto &c) {
        c.supply -= asset(item_ids.size(), symbol(symbol_name, 0));
    });

    account_table accounts(_self, symbol_raw);
    for(auto iter = item_ids.begin(); iter != item_ids.end(); iter++)
    {
        uint64_t item_id = stoull(*iter, 0, 10);
        const auto& account = accounts.get(item_id, "invalid item");

        SEND_INLINE_ACTION(
            *this,
            receipt,
            { { _self, name("active") } },
            {
                owner,
                owner_group,
                currency->app_id,
                item_id,
                account.nickname,
                account.group_id,
                account.item_name,
                account.category,
                account.options,
                account.duration,
                account.transferable,
                asset(),
                string("burn")
            }
        );

        sub_balance(owner, owner_account, symbol_raw, item_id);
    }
}

ACTION itamstorenft::transfernft(name from, name to, symbol_code symbol_name, vector<string> item_ids, string memo)
{
    name from_account = has_auth(from) ? from : name(ITAM_GROUP_ACCOUNT);
    require_auth(from_account);

    transfernft_memo nft_memo;
    parseMemo(&nft_memo, memo, "|", 2);
    
    name to_account = get_group_account(to, name(nft_memo.to_group));
    eosio_assert(is_account(to_account), "to_group_account does not exist");

    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    allow_table allows(_self, _self.value);
    eosio_assert(from == _self || allows.find(from_account.value) != allows.end(), "only allowed accounts can use this action");
    
    uint64_t symbol_raw = symbol_name.raw();

    account_table accounts(_self, symbol_raw);
    uint64_t current_sec = now();
    for(auto iter = item_ids.begin(); iter != item_ids.end(); iter++)
    {
        uint64_t item_id = stoull(*iter, 0, 10);
        const auto& account = accounts.require_find(item_id, "invalid item");

        eosio_assert(account->owner == from && account->account == from_account, "wrong item owner");
        eosio_assert(account->transferable, "not transferable item");
        eosio_assert(account->duration == 0 || account->duration <= current_sec, "overdrawn duration");

        accounts.modify(account, from_account, [&](auto &a) {
            a.owner = to;
            a.account = to_account;
            a.nickname = nft_memo.nickname;
        });
    }

    require_recipient(from_account, to_account);
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

ACTION itamstorenft::receipt(name owner, name owner_group, uint64_t app_id, uint64_t item_id, string nickname, uint64_t group_id, string item_name, string category, string options, uint64_t duration, bool transferable, asset payment_quantity, string state)
{
    require_auth(_self);

    name account = get_group_account(owner, owner_group);
    require_recipient(_self, account);
}

void itamstorenft::add_balance(const uint64_t symbol_raw, const name& ram_payer, const account& item)
{
    account_table accounts(_self, symbol_raw);
    const auto& account = accounts.find(item.item_id);
    eosio_assert(account == accounts.end(), "already item id");

    accounts.emplace(ram_payer, [&](auto &a) {
        a = item;
    });
}

void itamstorenft::sub_balance(const name& owner, const name& owner_account, const uint64_t symbol_raw, const uint64_t item_id)
{
    account_table accounts(_self, symbol_raw);
    const auto& account = accounts.require_find(item_id, "invalid item");
    eosio_assert(account->owner == owner && account->account == owner_account, "wrong item owner");

    accounts.erase(account);
}
