#include "itamgamesgas.hpp"

ACTION itamgamesgas::setsettle(string appId, name account)
{
    require_auth(_self);
    eosio_assert(is_account(account), "account does not exist");

    uint64_t appid = stoull(appId, 0, 10);

    settleTable settles(_self, _self.value);
    const auto& settle = settles.find(appid);

    if(settle == settles.end())
    {
        settles.emplace(_self, [&](auto &s) {
            s.appId = appid;
            s.settleAccount = account;
            s.settleAmount = asset(0, symbol("EOS", 4));
        });
    }
    else
    {
        settles.modify(settle, _self, [&](auto &s) {
            s.settleAccount = account;
        });
    }
}

ACTION itamgamesgas::claimsettle(string appId)
{
    require_auth(_self);

    settleTable settles(_self, _self.value);
    const auto& settle = settles.require_find(stoull(appId, 0, 10), "settle account not found");

    if(settle->settleAmount.amount > 0)
    {
        action(
            permission_level{_self, name("active")},
            name("eosio.token"),
            name("transfer"),
            make_tuple(
                _self,
                settle->settleAccount,
                settle->settleAmount,
                string("ITAM Store settlement")
            )
        ).send();
    };

    settles.modify(settle, _self, [&](auto &s) {
        s.settleAmount.amount = 0;
    });
}

ACTION itamgamesgas::transfer(uint64_t from, uint64_t to)
{
    transferData data = unpack_action_data<transferData>();
    if(data.from != name("itamstoreapp") && data.from != name("itamstoredex")) return;

    uint64_t appId = stoull(data.memo, 0, 10);
    settleTable settles(_self, _self.value);
    const auto& settle = settles.require_find(appId, "settle account does not exist");

    settles.modify(settle, _self, [&](auto &a) {
        a.settleAmount += data.quantity;
    });
}