#include "itamitamitam.hpp"

ACTION itamitamitam::transfer(uint64_t from, uint64_t to)
{
    transferData data = unpack_action_data<transferData>();

    if(data.from != _self && data.to == _self)
    {
        name owner(data.memo);
        add_balance(owner, data.quantity);
    }
    else if(data.from == _self && data.to == name("itamstoreapp"))
    {        
        paymentMemo memo;
        parseMemo(&memo, data.memo, "|", 5);
        sub_balance(name(memo.owner), data.quantity);
    }
    else if(data.from == _self && data.to == name("itamstoredex"))
    {
        dexMemo memo;
        parseMemo(&memo, data.memo, "|", 4);
        sub_balance(name(memo.owner), data.quantity);
    }
}

ACTION itamitamitam::transferto(name from, name to, asset quantity, string memo)
{
    require_auth(_self);

    sub_balance(from, quantity);

    if(_self == to)
    {
        name receiver(memo);
        eosio_assert(from != receiver, "cannot transfer to self");
        add_balance(receiver, quantity);
    }
    else
    {
        name eosio_token("eosio.token");

        action(
            permission_level { _self , name("active") },
            quantity.symbol.code().raw() == eosio_token.value ? eosio_token : name("itamtokenadm"),
            name("transfer"),
            make_tuple( _self, to, quantity, memo )
        ).send();
    }
}

void itamitamitam::add_balance(const name& owner, const asset& quantity)
{
    accountTable accounts(_self, quantity.symbol.code().raw());
    const auto& account = accounts.find(owner.value);

    if(account == accounts.end())
    {
        accounts.emplace(_self, [&](auto &a) {
            a.owner = owner;
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

void itamitamitam::sub_balance(const name& owner, const asset& quantity)
{
    accountTable accounts(_self, quantity.symbol.code().raw());
    const auto& account = accounts.find(owner.value);
    eosio_assert(account != accounts.end() && account->balance >= quantity, "overdrawn balance");

    if((account->balance - quantity).amount == 0)
    {
        accounts.erase(account);
    }
    else
    {
        accounts.modify(account, _self, [&](auto &a) {
            a.balance -= quantity;
        });
    }
}