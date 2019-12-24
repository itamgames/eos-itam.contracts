#include "itamgamesgas.hpp"

ACTION itamgamesgas::setsettle(string app_id, name settle_account)
{
    require_auth(_self);
    eosio_assert(is_account(settle_account), "account does not exist");

    uint64_t appid = stoull(app_id, 0, 10);

    account_table accounts(_self, _self.value);
    const auto& account = accounts.find(appid);
    if(account == accounts.end())
    {
        accounts.emplace(_self, [&](auto &a) {
            a.app_id = appid;
            a.settle_account = settle_account;
        });
    }
    else
    {
        accounts.modify(account, _self, [&](auto &a) {
            a.settle_account = settle_account;
        });
    }
}

ACTION itamgamesgas::claimsettle(string app_id)
{
    require_auth(_self);
    uint64_t appid = stoull(app_id, 0, 10);

    account_table accounts(_self, _self.value);
    const auto& account = accounts.get(appid, "invaid app id");
    name settle_account = account.settle_account;

    name dex_contract(DEX_CONTRACT);
    token_table tokens(dex_contract, dex_contract.value);
    name transfer_action("transfer");
    
    settlement_table settlements(_self, appid);
    for(auto settle = settlements.begin(); settle != settlements.end(); settle++)
    {
        settlements.modify(settle, _self, [&](auto &s) {
            if(s.quantity.amount > 0)
            {
                const auto& token = tokens.get(s.quantity.symbol.code().raw(), "invalid token contract");
                
                action(
                    permission_level( _self, name("active") ),
                    token.contract_name,
                    transfer_action,
                    make_tuple( _self, settle_account, s.quantity, string("ITAM Store settlement") )
                ).send();

                SEND_INLINE_ACTION(
                    *this,
                    receiptstle,
                    { { _self, name("active") } },
                    { app_id, settle_account, s.quantity }
                );

                s.quantity.amount = 0;
            }
        });
    }
}

ACTION itamgamesgas::receiptstle(string app_id, name settle_account, asset quantity)
{
    require_auth(_self);
}

ACTION itamgamesgas::transfer(uint64_t from, uint64_t to)
{
    transfer_data data = unpack_action_data<transfer_data>();
    name dex_contract(DEX_CONTRACT);

    if(data.from == dex_contract)
    {
        token_table tokens(dex_contract, dex_contract.value);
        const auto& token = tokens.get(data.quantity.symbol.code().raw(), "invalid token symbol");
        eosio_assert(token.contract_name == _code, "wrong token contract");
    }
    else if(data.from != name(APP_CONTRACT))
    {
        return;
    }

    uint64_t appid = stoull(data.memo, 0, 10);
    settlement_table settlements(_self, appid);
    const auto& settle = settlements.find(data.quantity.symbol.code().raw());

    if(settle == settlements.end())
    {
        settlements.emplace(_self, [&](auto &s) {
            s.quantity = data.quantity;
        });
    }
    else
    {
        settlements.modify(settle, _self, [&](auto &s) {
            s.quantity += data.quantity;
        });
    }
}