#include "itamtokenadm.hpp"

ACTION itamtokenadm::create(name issuer, asset maximum_supply)
{
    require_auth(_self);
    
    eosio_assert(maximum_supply.is_valid(), "invalid supply.");
    eosio_assert(maximum_supply.amount > 0, "max-supply must be positive.");

    currency_table currency(_self, maximum_supply.symbol.code().raw());
    eosio_assert(currency.find(maximum_supply.symbol.code().raw()) == currency.end(), "symbol already exists.");
    
    currency.emplace(_self, [&](auto& s) {
        s.supply.symbol = maximum_supply.symbol;
        s.max_supply = maximum_supply;
        s.issuer = issuer;
    });
}

ACTION itamtokenadm::issue(name to, asset quantity, string memo)
{
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");
    
    uint64_t symbol_name = quantity.symbol.code().raw();
    currency_table currency(_self, symbol_name);

    auto token = currency.require_find(symbol_name, "token with symbol does not exist, create token before issue");

    require_auth(token->issuer);
    
    eosio_assert(quantity.amount > 0, "must issue positive quantity");
    eosio_assert(quantity.amount <= token->max_supply.amount - token->supply.amount, "quantity exceeds available supply");
    
    currency.modify(token, _self, [&](auto& s) {
        s.supply += quantity;
    });
    
    add_balance(token->issuer, quantity, token->issuer);

    if(to != token->issuer)
    {
        SEND_INLINE_ACTION(
            *this,
            transfer,
            { token->issuer, name("active") },
            { token->issuer, to, quantity, memo }
        );
    }
}

ACTION itamtokenadm::transfer(name from, name to, asset quantity, string memo)
{
    eosio_assert(from != to, "cannot transfer to self");
    require_auth(from);
    eosio_assert(is_account(to), "to account does not exist");
    
    uint64_t symbol_name = quantity.symbol.code().raw();
    currency_table currencies(_self, symbol_name);
    const auto& currency = currencies.get(symbol_name);

    require_recipient(from);
    require_recipient(to);

    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must transfer positive quantity");
    eosio_assert(quantity.symbol == currency.supply.symbol, "symbol precision mismatch");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    sub_balance(from, quantity);
    add_balance(to, quantity, from);
}

ACTION itamtokenadm::staking(name owner, asset quantity)
{
    require_auth(owner);

    account_table accounts(_self, owner.value);
    const auto& account = accounts.get(quantity.symbol.code().raw(), "no balance object found");
    
    eosio_assert(account.balance.symbol == quantity.symbol, "invalid symbol");
    eosio_assert(account.balance.amount >= quantity.amount,"overdrawn balance");
    eosio_assert(quantity.amount > 0, "stake quantity must be positive");

    refund_table refunds(_self, owner.value);
    const auto& refund = refunds.find(quantity.symbol.code().raw());
    uint64_t refund_balance = refund != refunds.end() ? refund->total_refund.amount : 0;

    lock_table locks(_self, quantity.symbol.code().raw());
    const auto& lock = locks.find(owner.value);
    uint64_t lock_balance = lock != locks.end() ? lock->total_lock.amount : 0;

    stake_table stakes(_self, owner.value);
    const auto& stake = stakes.find(owner.value);
    if(stake == stakes.end())
    {
        stakes.emplace(_self, [&](auto& s) {
            s.owner = owner;
            s.balance = quantity;
        });
    }
    else
    {
        uint64_t stake_balance = stake->balance.amount;
        uint64_t current_balance = account.balance.amount - (stake_balance + refund_balance + lock_balance);
        
        eosio_assert(current_balance >= quantity.amount, "overdrawn current balance");
        stakes.modify(stake, _self, [&](auto& s) {
            s.balance += quantity;
        });
    }
}

ACTION itamtokenadm::unstaking(name owner, asset quantity)
{
    require_auth(owner);

    stake_table stakes(_self, owner.value);
    const auto& stake = stakes.require_find(owner.value, "can't find stake id");

    eosio_assert(stake->balance >= quantity, "overdrawn balance for stake");

    stakes.modify(stake, _self, [&](auto& s) {
        s.balance -= quantity;
    });

    refund_table refunds(_self, owner.value);
    const auto& refund = refunds.find(owner.value);

    if(refund == refunds.end())
    {
        refunds.emplace(_self, [&](auto& v) {
            refund_info info;
            info.refund_amount = quantity;
            info.req_refund_ts = now();
            v.owner = owner;
            v.total_refund = quantity;
            v.refund_list.push_back(info);
        });
    }
    else
    {
        refunds.modify(refund, _self, [&](auto& v) {
            refund_info info;
            info.refund_amount = quantity;
            info.req_refund_ts = now();
            v.total_refund += quantity;
            v.refund_list.push_back(info);
        });
    }

    transaction txn;
    txn.actions.emplace_back(
        permission_level(_self, name("active")),
        _self,
        name("defrefund"),
        make_tuple(owner, quantity)
    );

    txn.delay_sec = SEC_REFUND_DELAY;
    txn.send(now(), owner);
}

ACTION itamtokenadm::defrefund(name owner)
{
    refund(owner);
}

ACTION itamtokenadm::menrefund(name owner)
{
    refund(owner);
}

void itamtokenadm::refund(name owner)
{
    require_auth(_self);
    
    refund_table refunds(_self, owner.value);
    const auto& refund = refunds.require_find(owner.value, "refund info not exist");

    refunds.modify(refund, _self, [&](auto& v) {
        uint64_t current_sec = now();
        for(auto iter = v.refund_list.begin(); iter != v.refund_list.end();)
        {
            if(iter->req_refund_ts + SEC_REFUND_DELAY <= current_sec)
            {
                v.total_refund.amount -= iter->refund_amount.amount;
                iter = v.refund_list.erase(iter);
            }
            else iter++;
        }
    });

    if(refund->refund_list.size() == 0)
    {
        refunds.erase(refund);
    }
}

void itamtokenadm::sub_balance(name owner, asset value)
{
    account_table accounts(_self, owner.value);
    const auto& account = accounts.get(value.symbol.code().raw(), "no balance object found");

    uint64_t lock_amount = 0;
    lock_table locks(_self, value.symbol.code().raw());
    const auto& lock = locks.find(owner.value);
    if(lock != locks.end())
    {
        lock_amount = lock->total_lock.amount;
    }
    
    uint64_t stake_amount = 0;
    stake_table stakes(_self, owner.value);
    const auto& stake = stakes.find(owner.value);
    if(stake != stakes.end())
    {
        stake_amount += stake->balance.amount;
    }
    
    uint64_t refund_amount = 0;
    refund_table refunds(_self, owner.value);
    const auto& refund = refunds.find(owner.value);
    if(refund != refunds.end())
    {
        refund_amount = refund->total_refund.amount;
    }
    
    uint64_t need_minimum_amount = value.amount + stake_amount + refund_amount + lock_amount;
    eosio_assert(account.balance.amount >= need_minimum_amount, "overdrawn balance");
    
    if(account.balance.amount == value.amount) 
    {
        accounts.erase(account);
    } 
    else
    {
        accounts.modify(account, owner, [&](auto& a) {
            a.balance -= value;
        });
    }
}

ACTION itamtokenadm::burn(asset quantity, string memo)
{
    require_auth(_self);
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");
    eosio_assert(quantity.amount > 0, "quantity must be positive");

    currency_table currencies( _self, quantity.symbol.code().raw());
    const auto& currency = currencies.require_find(quantity.symbol.code().raw(), "invalid symbol");

    currencies.modify(currency, _self, [&](auto &c) {
        c.supply -= quantity;
    });

    sub_balance(_self, quantity);
}

ACTION itamtokenadm::mint(name owner, asset quantity, string memo)
{
    require_auth(_self);
    eosio_assert(is_account(owner), "owner does not exist");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");
    eosio_assert(quantity.amount > 0, "quantity must be positive");

    currency_table currencies(_self, quantity.symbol.code().raw());
    const auto& currency = currencies.require_find(quantity.symbol.code().raw(), "invalid symbol");

    currencies.modify(currency, _self, [&](auto &c) {
        c.supply += quantity;
        c.max_supply += quantity;
    });

    add_balance(owner, quantity, _self);
}

ACTION itamtokenadm::locktoken(name owner, asset quantity, uint64_t timestamp_sec)
{
    require_auth(_self);
    eosio_assert(is_account(owner), "owner does not exist");
    
    SEND_INLINE_ACTION(
        *this,
        issue,
        { { _self, name("active") } },
        { owner, quantity, "issue lock token" }
    );

    uint64_t symbol_raw = quantity.symbol.code().raw();
    currency_table currencies(_self, symbol_raw);
    const auto& currency = currencies.get(symbol_raw, "invalid symbol");

    lock_table locks(_self, symbol_raw);
    const auto& lock = locks.find(owner.value);

    lock_info info { quantity, timestamp_sec };
    if(lock == locks.end())
    {
        locks.emplace(_self, [&](auto &l) {
            l.owner = owner;
            l.total_lock = quantity;
            l.lock_infos.push_back(info);
        });
    }
    else
    {
        locks.modify(lock, _self, [&](auto &l) {
            l.total_lock += quantity;
            l.lock_infos.push_back(info);
        });
    }

    transaction tx;
    tx.actions.emplace_back(
        permission_level( _self, name("active") ),
        _self,
        name("releasetoken"),
        make_tuple( owner, quantity.symbol.code().to_string() )
    );

    uint64_t current_sec = now();
    eosio_assert(timestamp_sec > current_sec, "overdrawn timestamp second");

    tx.delay_sec = timestamp_sec - current_sec;
    tx.send(current_sec, _self);
}

ACTION itamtokenadm::releasetoken(name owner, string symbol_name)
{
    require_auth(_self);
    uint64_t symbol_raw = symbol(symbol_name, 4).code().raw();
    currency_table currencies(_self, symbol_raw);
    const auto& currency = currencies.get(symbol_raw, "invalid symbol");

    lock_table locks(_self, symbol_raw);
    const auto& lock = locks.require_find(owner.value, "owner doesn't have lock balance");

    uint64_t current_sec = now();
    locks.modify(lock, _self, [&](auto &l) {
        for(auto iter = l.lock_infos.begin(); iter != l.lock_infos.end();)
        {
            if(iter->timestamp <= current_sec)
            {
                locks.modify(lock, _self, [&](auto &l) {
                    l.total_lock -= iter->quantity;
                    iter = l.lock_infos.erase(iter);
                });
            }
            else iter++;
        }
    });

    if(lock->total_lock.amount == 0)
    {
        locks.erase(lock);
    }
}

void itamtokenadm::add_balance(name owner, asset value, name ram_payer)
{
    account_table accounts(_self, owner.value);
    auto account = accounts.find(value.symbol.code().raw());
    
    if(account == accounts.end()) 
    {
        accounts.emplace(ram_payer, [&](auto& a) {
            a.balance = value;
        });
    } 
    else 
    {
        accounts.modify(account, ram_payer, [&](auto& a) {
            a.balance += value;
        });
    }
}