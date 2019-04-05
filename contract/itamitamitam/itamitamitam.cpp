#include "itamitamitam.hpp"

ACTION itamitamitam::transfer(uint64_t from, uint64_t to)
{
    transferData data = unpack_action_data<transferData>();
    if (data.from == _self || data.to != _self) return;
    
    name owner(data.memo);

    accountTable accounts(_self, data.quantity.symbol.code().raw());
    const auto& account = accounts.find(owner.value);

    if(account == accounts.end())
    {
        accounts.emplace(_self, [&](auto &a) {
            a.owner = owner;
            a.balance = data.quantity;
        });
    }
    else
    {
        accounts.modify(account, _self, [&](auto &a) {
            a.balance += data.quantity;
        });
    }
}

ACTION itamitamitam::transferto(name from, name to, asset quantity, string memo)
{
    require_auth(_self);
    
    accountTable accounts(_self, quantity.symbol.code().raw());
    const auto& account = accounts.require_find(from.value, "owner does not exist");
    eosio_assert(account->balance >= quantity, "overdrawn balance");

    accounts.modify(account, _self, [&](auto &a) {
        a.balance -= quantity;
    });
    
    if(account->balance.amount == 0)
    {
        accounts.erase(account);
    }

    if(_self == to)
    {
        name toOwner(memo);
        eosio_assert(from != toOwner, "cannot transfer to self");
        const auto& toAccount = accounts.find(toOwner.value);
        if(toAccount == accounts.end())
        {
            accounts.emplace(_self, [&](auto &a) {
                a.owner = toOwner;
                a.balance = quantity;
            });
        }
        else
        {
            accounts.modify(account, _self, [&](auto &a) {
                a.balance += quantity;
            });
        }
    }
    else
    {
        action(
            permission_level { _self , name("active") },
            name("eosio.token"),
            name("transfer"),
            make_tuple( _self, to, quantity, memo )
        ).send();
    }
}