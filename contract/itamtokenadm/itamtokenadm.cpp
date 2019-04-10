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

ACTION itamtokenadm::issue(name to, asset quantity, name lock_type, string memo)
{
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");
    
    uint64_t symbol_name = quantity.symbol.code().raw();
    currency_table currencies(_self, symbol_name);

    const auto& currency = currencies.get(symbol_name, "token with symbol does not exist, create token before issue");

    require_auth(currency.issuer);
    
    eosio_assert(quantity.amount > 0, "must issue positive quantity");
    eosio_assert(quantity.amount <= currency.max_supply.amount - currency.supply.amount, "quantity exceeds available supply");
    
    currencies.modify(currency, _self, [&](auto& s) {
        s.supply += quantity;
    });
    
    add_balance(currency.issuer, quantity, currency.issuer);

    if(to != currency.issuer)
    {
        SEND_INLINE_ACTION(
            *this,
            transfer,
            { currency.issuer, name("active") },
            { currency.issuer, to, quantity, memo }
        );
    }

    if(lock_type.to_string() == "") return;
    locktype_table locktypes(_self, _self.value);
    eosio_assert(locktypes.find(lock_type.value) != locktypes.end(), "invalid lock_type");

    lock_table locks(_self, _self.value);
    const auto& lock = locks.find(to.value);
    
    lockinfo info { quantity, lock_type };

    if(lock == locks.end())
    {
        locks.emplace(_self, [&](auto &l) {
            l.owner = to;
            l.lockinfos.push_back(info);
        });
    }
    else
    {
        locks.modify(lock, _self, [&](auto &l) {
            l.lockinfos.push_back(info);
        });
    }
}

ACTION itamtokenadm::transfer(name from, name to, asset quantity, string memo)
{
    eosio_assert(from != to, "cannot transfer to self");
    require_auth(from);
    eosio_assert(is_account(to), "to account does not exist");

    blacklist_table blacklists(_self, _self.value);
    eosio_assert(blacklists.find(from.value) == blacklists.end(), "blacklist owner");
    
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

ACTION itamtokenadm::burn(name owner, asset quantity, string memo)
{
    require_auth(owner);
    eosio_assert(quantity.amount > 0, "quantity must be positive");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    currency_table currencies(_self, quantity.symbol.code().raw());
    const auto& currency = currencies.require_find(quantity.symbol.code().raw(), "invalid symbol");
    currencies.modify(currency, _self, [&](auto &c) {
        c.supply -= quantity;
    });

    sub_balance(owner, quantity);
}

ACTION itamtokenadm::mint(asset quantity, string memo)
{
    eosio_assert(quantity.amount > 0, "quantity must be positive");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    currency_table currencies(_self, quantity.symbol.code().raw());
    const auto& currency = currencies.require_find(quantity.symbol.code().raw(), "invalid symbol");
    require_auth(currency->issuer);

    currencies.modify(currency, _self, [&](auto &c) {
        c.max_supply += quantity;
        eosio_assert(c.max_supply.amount > 0, "max_supply must be positive");
    });
}

ACTION itamtokenadm::setlocktype(name lock_type, uint64_t start_timestamp_sec, vector<int32_t> percents)
{
    require_auth(_self);

    locktype_table locktypes(_self, _self.value);
    const auto& locktype = locktypes.find(lock_type.value);

    uint64_t total_percents = 0;
    for(auto iter = percents.begin(); iter != percents.end(); iter++)
    {
        total_percents += *iter;
    }
    eosio_assert(total_percents == 100, "total percents must be 100");

    if(locktype == locktypes.end())
    {
        locktypes.emplace(_self, [&](auto &l) {
            l.lock_type = lock_type;
            l.start_timestamp_sec = start_timestamp_sec;
            l.percents = percents;
        });
    }
    else
    {
        locktypes.modify(locktype, _self, [&](auto &l) {
            l.start_timestamp_sec = start_timestamp_sec;
            l.percents = percents;
        });
    }
}

ACTION itamtokenadm::dellocktype(name lock_type)
{
    require_auth(_self);

    locktype_table locktypes(_self, _self.value);
    const auto& locktype = locktypes.require_find(lock_type.value, "invalid lock_type");
    locktypes.erase(locktype);
}

ACTION itamtokenadm::regblacklist(name owner)
{
    require_auth(_self);

    blacklist_table blacklists(_self, _self.value);
    eosio_assert(blacklists.find(owner.value) == blacklists.end(), "blacklist already exists");
    blacklists.emplace(_self, [&](auto &b) {
        b.owner = owner;
    });
}

ACTION itamtokenadm::delblacklist(name owner)
{
    require_auth(_self);

    blacklist_table blacklists(_self, _self.value);
    const auto& blacklist = blacklists.require_find(owner.value, "no blacklist owner");
    blacklists.erase(blacklist);
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

void itamtokenadm::sub_balance(name owner, asset value)
{
    account_table accounts(_self, owner.value);
    const auto& account = accounts.get(value.symbol.code().raw(), "no balance object found");
    int64_t balance_amount = account.balance.amount;

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

    lock_table locks(_self, _self.value);
    const auto& lock = locks.find(owner.value);

    if(lock == locks.end()) return;

    int64_t lock_amount = 0;
    int32_t current_time_sec = now();
    map<uint64_t, int32_t> release_percents;

    const vector<lockinfo>& owner_lockinfos = lock->lockinfos;
    for(auto owner_lockinfo = owner_lockinfos.begin(); owner_lockinfo != owner_lockinfos.end(); owner_lockinfo++)
    {
        name owner_lock_type = owner_lockinfo->lock_type;
        uint64_t release_percent = 0;

        auto iter = release_percents.find(owner_lock_type.value);
        if(iter == release_percents.end())
        {
            locktype_table locktypes(_self, _self.value);
            const auto& locktype = locktypes.get(owner_lock_type.value, "invalid lock_type");
            
            int32_t pass_month = (current_time_sec - locktype.start_timestamp_sec) / (SECONDS_OF_DAY * 30);
            for(int i = 0; i < pass_month; i++)
            {
                release_percent += locktype.percents[i];
            }

            release_percents[owner_lock_type.value] = release_percent;
        }

        int32_t lock_percent = 100 - release_percent;
        lock_amount += (owner_lockinfo->quantity * lock_percent / 100).amount;
    }

    eosio_assert(balance_amount - lock_amount - value.amount >= 0, "overdrawn balance");
}