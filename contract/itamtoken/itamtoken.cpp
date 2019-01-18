#include "itamtoken.hpp"

ACTION itamtoken::create(name issuer, asset maximum_supply)
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

ACTION itamtoken::issue(name to, asset quantity, string memo)
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
        SEND_INLINE_ACTION(*this, transfer, {token->issuer,"active"_n}, {token->issuer, to, quantity, memo});
    }
}

ACTION itamtoken::transfer(name from, name to, asset quantity, string memo)
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

ACTION itamtoken::staking(name owner, asset value)
{
    require_auth(owner);

    account_table accounts(_self, owner.value);
    const auto& account = accounts.get(value.symbol.code().raw(), "no balance object found");
    
    eosio_assert(account.balance.symbol == value.symbol, "invalid symbol");
    eosio_assert(account.balance.amount >= value.amount,"overdrawn balance");
    eosio_assert(value.amount > 0, "stake value must be positive");

    uint64_t refund_balance = 0;

    refund_table refunds(_self, owner.value);
    auto refund = refunds.find(value.symbol.code().raw());
    if(refund != refunds.end())
    {
        refund_balance = refund->total_refund.amount;
    }

    stake_table stakes(_self, owner.value);
    auto stake = stakes.find(owner.value);
    if(stake == stakes.end())
    {
        stakes.emplace(_self, [&](auto& s) {
            s.owner = owner;
            s.balance = value;
        });
    }
    else
    {
        uint64_t stake_balance = stake->balance.amount;
        uint64_t current_balance = account.balance.amount - (stake_balance + refund_balance);
        
        eosio_assert(current_balance >= value.amount, "overdrawn current balance");
        stakes.modify(stake, _self, [&](auto& s) {
            s.balance += value;
        });
    }
}

ACTION itamtoken::unstaking(name owner, asset value)
{
    require_auth(owner);

    stake_table stakes(_self, owner.value);
    auto stake = stakes.require_find(owner.value, "can't find stake id");

    eosio_assert(stake->balance >= value, "overdrawn balance for stake");

    stakes.modify(stake, _self, [&](auto& s) {
        s.balance -= value;
    });

    refund_table refunds(_self, owner.value);
    auto refund = refunds.find(owner.value);

    if(refund == refunds.end())
    {
        refunds.emplace(_self, [&](auto& v) {
            refund_info info;
            info.refund_amount = value;
            info.req_refund_ts = now();
            v.owner = owner;
            v.total_refund = value;
            v.refund_list.push_back(info);
        });
    }
    else
    {
        refunds.modify(refund, _self, [&](auto& v) {
            refund_info info;
            info.refund_amount = value;
            info.req_refund_ts = now();
            v.total_refund += value;
            v.refund_list.push_back(info);
        });
    }

    transaction txn;
    txn.actions.emplace_back(
        permission_level(_self, name("active")),
        _self,
        name("defrefund"),
        make_tuple(owner, value)
    );

    txn.delay_sec = SEC_REFUND_DELAY;
    txn.send(now(), owner);
}

ACTION itamtoken::defrefund(name owner)
{
    refund(owner);
}

ACTION itamtoken::menualrefund(name owner)
{
    refund(owner);
}

void itamtoken::refund(name owner)
{
    require_auth(_self);
    
    refund_table refunds(_self, owner.value);
    auto refund = refunds.require_find(owner.value, "refund info not exist");

    refunds.modify(refund, _self, [&](auto& v) {
        uint64_t dcount = 0;
        uint64_t table_rows = refund->refund_list.size();
        
        for(uint64_t i = 0; i < table_rows; i++)
        {
            if(refund->refund_list[i - dcount].req_refund_ts + SEC_REFUND_DELAY <= now())
            {
                v.total_refund.amount -= refund->refund_list[i].refund_amount.amount;
                v.refund_list.erase(v.refund_list.begin() + (i - dcount));
                dcount++;
            }
        }
    });

    // TODO: erase row if refund_list is empty.
}

void itamtoken::sub_balance(name owner, asset value)
{
    account_table accounts(_self, owner.value);
    
    const auto& account = accounts.get(value.symbol.code().raw(), "no balance object found");
    
    uint64_t stake_amount = 0;
    uint64_t refund_amount = 0;
    
    stake_table stakes(_self, owner.value);
    
    auto stake = stakes.find(owner.value);
    if(stake != stakes.end())
    {
        stake_amount += stake->balance.amount;
    }
    
    refund_table refunds(_self, owner.value);

    auto refund = refunds.find(owner.value);
    if(refund != refunds.end())
    {
        refund_amount = refund->total_refund.amount;
    }
    
    eosio_assert(account.balance.amount >= (value.amount + stake_amount + refund_amount), "overdrawn balance");
    
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

void itamtoken::add_balance(name owner, asset value, name ram_payer)
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

#define EOSIO_DISPATCH_EX( TYPE, MEMBERS ) \
extern "C" { \
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
      if( code == receiver || code == name("itamstoreapp").value || code == name("itamstoredex").value ) { \
         switch( action ) { \
            EOSIO_DISPATCH_HELPER( TYPE, MEMBERS ) \
         } \
         /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
      } \
   } \
} \

EOSIO_DISPATCH_EX( itamtoken, (test)(create)(issue)(transfer)(staking)(unstaking)(defrefund)(menualrefund) )