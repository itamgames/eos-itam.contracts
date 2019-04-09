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
    lockinfo_table lockinfos(_self, _self.value);
    eosio_assert(lockinfos.find(lock_type.value) != lockinfos.end(), "invalid lock_type");

    lock_table locks(_self, _self.value);
    const auto& lock = locks.find(to.value);

    if(lock == locks.end())
    {
        locks.emplace(_self, [&](auto &l) {
            l.owner = to;
            l.quantity = quantity;
            l.lock_type = lock_type;
        });
    }
    else
    {
        locks.modify(lock, _self, [&](auto &l) {
            l.quantity += quantity;
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

ACTION itamtokenadm::burn(asset quantity, string memo)
{
    eosio_assert(quantity.amount > 0, "quantity must be positive");
    change_max_supply(quantity * -1, memo);
}

ACTION itamtokenadm::mint(asset quantity, string memo)
{
    eosio_assert(quantity.amount > 0, "quantity must be positive");
    change_max_supply(quantity, memo);
}

ACTION itamtokenadm::setlockinfo(name lock_type, uint64_t start_timestamp_sec, vector<int32_t> percents)
{
    require_auth(_self);

    lockinfo_table lockinfos(_self, _self.value);
    const auto& lockinfo = lockinfos.find(lock_type.value);

    uint64_t total_percents = 0;
    for(auto iter = percents.begin(); iter != percents.end(); iter++)
    {
        total_percents += *iter;
    }
    eosio_assert(total_percents == 100, "total percents must be 100");

    if(lockinfo == lockinfos.end())
    {
        lockinfos.emplace(_self, [&](auto &l) {
            l.lock_type = lock_type;
            l.start_timestamp_sec = start_timestamp_sec;
            l.percents = percents;
        });
    }
    else
    {
        lockinfos.modify(lockinfo, _self, [&](auto &l) {
            l.start_timestamp_sec = start_timestamp_sec;
            l.percents = percents;
        });
    }
}

ACTION itamtokenadm::dellockinfo(name lock_type)
{
    require_auth(_self);

    lockinfo_table lockinfos(_self, _self.value);
    const auto& lockinfo = lockinfos.require_find(lock_type.value, "invalid lock_type");
    lockinfos.erase(lockinfo);
}

ACTION itamtokenadm::regblacklist(name owner)
{
    require_auth(_self);

    blacklist_table blacklists(_self, _self.value);
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

void itamtokenadm::change_max_supply(const asset& quantity, const string& memo)
{
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    currency_table currencies(_self, quantity.symbol.code().raw());
    const auto& currency = currencies.require_find(quantity.symbol.code().raw(), "invalid symbol");
    require_auth(currency->issuer);

    currencies.modify(currency, _self, [&](auto &c) {
        c.max_supply += quantity;
        eosio_assert(c.max_supply.amount > 0, "max_supply must be positive");
    });
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

    lock_table locks(_self, _self.value);
    const auto& lock = locks.find(owner.value);

    int64_t lock_amount = 0;
    if(lock != locks.end())
    {
        lockinfo_table lockinfos(_self, _self.value);
        const auto& lockinfo = lockinfos.get(lock->lock_type.value, "invalid lock_type");

        asset total_lock = lock->quantity;
        const vector<int32_t>& lock_percents = lockinfo.percents;
        
        int32_t release_percent = 0;
        int32_t pass_month = ((int32_t)now() - (int32_t)lockinfo.start_timestamp_sec) / (SECONDS_OF_DAY * 30);

        for(int i = 0; i < pass_month; i++)
        {
            release_percent += lock_percents[i];
        }

        int32_t lock_percent = 100 - release_percent;
        lock_amount = (total_lock * lock_percent / 100).amount;
    }
    eosio_assert(account.balance.amount - lock_amount - value.amount >= 0, "overdrawn balance");

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