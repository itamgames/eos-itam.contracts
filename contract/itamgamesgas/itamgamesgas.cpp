#include "itamgamesgas.hpp"

ACTION itamgamesgas::migration()
{
    require_auth(_self);
    
    settleTable settles(_self, _self.value);
    account_table accounts(_self, _self.value);
    for(auto settle = settles.begin(); settle != settles.end(); settle++)
    {
        accounts.emplace(_self, [&](auto &a) {
            a.app_id = settle->appId;
            a.settle_account = settle->settleAccount;
        });

        settlement_table settlements(_self, settle->appId);
        settlements.emplace(_self, [&](auto &s) {
            s.quantity = settle->settleAmount;
        });
    }
}

ACTION itamgamesgas::deloldsettle()
{
    require_auth(_self);
    settleTable settles(_self, _self.value);
    for(auto settle = settles.begin(); settle != settles.end(); settle = settles.erase(settle));
}

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

    name dex_contract("itamstoredex");
    token_table tokens(dex_contract, dex_contract.value);
    name transfer_action("transfer");
    
    settlement_table settlements(_self, appid);
    for(auto settle = settlements.begin(); settle != settlements.end(); settle++)
    {
        settlements.modify(settle, settle_account, [&](auto &s) {
            if(s.quantity.amount > 0)
            {
                const auto& token = tokens.get(s.quantity.symbol.code().raw(), "invalid token contract");
                
                action(
                    permission_level( _self, name("active") ),
                    token.contract_name,
                    transfer_action,
                    make_tuple( _self, settle_account, s.quantity, string("ITAM Store settlement") )
                ).send();

                s.quantity.amount = 0;
            }
        });
    }
}

ACTION itamgamesgas::transfer(uint64_t from, uint64_t to)
{
    transfer_data data = unpack_action_data<transfer_data>();
    name dex_contract("itamstoredex");

    if(data.from == dex_contract)
    {
        token_table tokens(dex_contract, dex_contract.value);
        const auto& token = tokens.get(data.quantity.symbol.code().raw(), "invalid token symbol");
        eosio_assert(token.contract_name == _code, "wrong token contract");
    }
    else if(data.from != name("itamstoreapp"))
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