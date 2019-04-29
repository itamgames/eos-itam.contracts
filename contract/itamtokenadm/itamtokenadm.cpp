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

    lock_table locks(_self, to.value);
    const auto& lock = locks.find(quantity.symbol.code().raw());
    
    lockinfo info { quantity, lock_type };

    if(lock == locks.end())
    {
        locks.emplace(_self, [&](auto &l) {
            l.sym = quantity.symbol;
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
    });
}

ACTION itamtokenadm::staking(name owner, asset value)
{
    require_auth(owner);

    account_table accounts(_self, owner.value);
    const auto& account = accounts.get(value.symbol.code().raw(), "no balance object found");
    
    eosio_assert(account.balance.symbol == value.symbol, "invalid symbol");
    eosio_assert(value.amount > 0, "stake value must be positive");
    assert_overdrawn_balance(owner, account.balance.amount, value);

    stake_table stakes(_self, owner.value);
    const auto& stake = stakes.find(value.symbol.code().raw());
    if(stake == stakes.end())
    {
        stakes.emplace(owner, [&](auto& s) {
            s.balance = value;
        });
    }
    else
    {
        stakes.modify(stake, owner, [&](auto &s) {
            s.balance += value;
        });
    }
}

ACTION itamtokenadm::unstaking(name owner, asset value)
{
    require_auth(owner);

    stake_table stakes(_self, owner.value);
    auto stake = stakes.require_find(value.symbol.code().raw(), "cannot found staking");

    eosio_assert(stake->balance >= value, "overdrawn balance for stake");

    stakes.modify(stake, _self, [&](auto& s) {
        s.balance -= value;
    });

    refund_table refunds(_self, owner.value);
    auto refund = refunds.find(value.symbol.code().raw());

    refund_info info { value, now() };
    if(refund == refunds.end())
    {
        refunds.emplace(_self, [&](auto& v) {
            v.total_refund = value;
            v.refund_list.push_back(info);
        });
    }
    else
    {
        refunds.modify(refund, _self, [&](auto& v) {
            v.total_refund += value;
            v.refund_list.push_back(info);
        });
    }

    transaction txn;
    txn.actions.emplace_back(
        permission_level( _self, name("active") ),
        _self,
        name("defrefund"),
        make_tuple( owner, value.symbol.code() )
    );

    txn.delay_sec = SEC_REFUND_DELAY;
    txn.send( now(), _self );
}

ACTION itamtokenadm::defrefund(name owner, symbol_code sym_code)
{
    refund(owner, sym_code);
}

ACTION itamtokenadm::menualrefund(name owner, symbol_code sym_code)
{
    refund(owner, sym_code);
}

void itamtokenadm::refund(const name& owner, const symbol_code& sym_code)
{
    require_auth(_self);
    
    refund_table refunds(_self, owner.value);
    const auto& refund = refunds.require_find(sym_code.raw(), "refund info not exist");

    refunds.modify(refund, _self, [&](auto& r) {
        vector<refund_info>& refund_list = r.refund_list;
        uint64_t current_sec = now();

        for(auto iter = refund_list.begin(); iter != refund_list.end();)
        {
            if(iter->req_refund_sec + SEC_REFUND_DELAY <= current_sec)
            {
                r.total_refund -= iter->refund_quantity;
                iter = r.refund_list.erase(iter);
            }
            else iter++;
        }
    });

    if(refund->total_refund.amount == 0)
    {
        refunds.erase(refund);
    }
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
    assert_overdrawn_balance(owner, account.balance.amount, value);

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

void itamtokenadm::assert_overdrawn_balance(const name& owner, int64_t available_balance, const asset& value)
{
    lock_table locks(_self, owner.value);
    const auto& lock = locks.find(value.symbol.code().raw());
    if(lock != locks.end())
    {
        int32_t current_time_sec = now();
        map<uint64_t, int32_t> release_percents;
        locktype_table locktypes(_self, _self.value);
        const vector<lockinfo>& owner_lockinfos = lock->lockinfos;

        for(auto owner_lockinfo = owner_lockinfos.begin(); owner_lockinfo != owner_lockinfos.end(); owner_lockinfo++)
        {
            name owner_lock_type = owner_lockinfo->lock_type;
            uint64_t release_percent = 0;

            auto iter = release_percents.find(owner_lock_type.value);
            if(iter == release_percents.end())
            {
                const auto& locktype = locktypes.get(owner_lock_type.value, "invalid lock_type");
                
                int32_t pass_month = (current_time_sec - locktype.start_timestamp_sec) / (SECONDS_OF_DAY * 30);
                for(int i = 0; i < pass_month; i++)
                {
                    release_percent += locktype.percents[i];
                }

                release_percents[owner_lock_type.value] = release_percent;
            }
            else
            {
                release_percent = iter->second;
            }
            
            int32_t lock_percent = 100 - release_percent;
            available_balance -= (owner_lockinfo->quantity * lock_percent / 100).amount;
        }
    }

    stake_table stakes(_self, owner.value);
    const auto& stake = stakes.find(value.symbol.code().raw());
    if(stake != stakes.end())
    {
        available_balance -= stake->balance.amount;
    }

    refund_table refunds(_self, owner.value);
    const auto& refund = refunds.find(value.symbol.code().raw());
    if(refund != refunds.end())
    {
        available_balance -= refund->total_refund.amount;
    }

    eosio_assert(available_balance >= value.amount, "overdrawn balance");
}