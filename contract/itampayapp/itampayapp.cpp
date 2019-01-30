#include "itampayapp.hpp"

ACTION itampayapp::test()
{
    configTable configs(_self, _self.value);
    for(auto iter = configs.begin(); iter != configs.end();)
    {
        iter = configs.erase(iter);
    }
}

ACTION itampayapp::registapp(uint64_t appId, name owner, asset amount, string params)
{
    require_auth(_self);

    eosio_assert(amount.symbol == symbol("EOS", 4) || amount.symbol == symbol("ITAM", 4), "Invalid symbol");

    action(
        permission_level{_self, name("active")},
        name("itamstoreapp"),
        name("regsellitem"),
        make_tuple(params)
    ).send();

    if(amount.amount == 0) return;

    appTable apps(_self, _self.value);
    const auto& app = apps.find(appId);
    if(app == apps.end())
    {
        apps.emplace(_self, [&](auto &a) {
            a.appId = appId;
            a.owner = owner;
            a.amount = amount;
        });

        settleTable settles(_self, _self.value);
        settles.emplace(_self, [&](auto &s) {
            s.appId = appId;
            s.account = owner;
            s.eos.symbol = symbol("EOS", 4);
            s.eos.amount = 0;
            s.itam.symbol = symbol("ITAM", 4);
            s.itam.amount = 0;
        });
    }
    else
    {
        apps.modify(app, _self, [&](auto &a) {
            a.owner = owner;
            a.amount = amount;
        });
    }
}

ACTION itampayapp::removeapp(uint64_t appId)
{
    require_auth(_self);

    appTable apps(_self, _self.value);
    const auto& app = apps.require_find(appId, "Invalid App Id");
    apps.erase(app);
    
    settleTable settles(_self, _self.value);
    const auto &settle = settles.require_find(appId, "Can't find settle states");
    eosio_assert(settle->eos.amount == 0 && settle->itam.amount == 0, "Can't remove app becuase it's not settled");
    settles.erase(settle);

    // TODO: remove itamstoreapp's items
}

ACTION itampayapp::refundapp(uint64_t appId, name buyer)
{
    appTable apps(_self, _self.value);
    const auto& app = apps.get(appId, "Invalid App Id");

    require_auth(_self);

    pendingTable pendings(_code, appId);
    const auto& pending = pendings.require_find(buyer.value, "Can't find refundable customer");

    configTable configs(_self, _self.value);
    const auto& config = configs.get(_self.value, "Can't find refundable day");

    uint64_t refundableTimestamp = pending->timestamp + (config.refundableDay * SECOND_OF_DAY);

    eosio_assert(refundableTimestamp >= now(), "Refundable day has passed");

    action(
        permission_level{_self, name("active")},
        name(symbol("EOS", 4) == app.amount.symbol ? "eosio.token" : "itamtokenadm"),
        name("transfer"),
        make_tuple(_self, buyer, app.amount, string("refund pay app"))
    ).send();

    pendings.erase(pending);
}

ACTION itampayapp::modifysettle(uint64_t appId, name newAccount)
{
    settleTable settles(_self, _self.value);
    const auto& settle = settles.require_find(appId, "Can't find settle table");

    require_auth(settle->account);
    eosio_assert(settle->account != newAccount, "Can't change self");

    settles.modify(settle, _self, [&](auto &s) {
        s.account = newAccount;
    });
}

ACTION itampayapp::claimsettle(uint64_t appId)
{
    require_auth(_self);

    settleTable settles(_self, _self.value);
    const auto& settle = settles.require_find(appId, "Can't find settle table");

    if(settle->eos.amount > 0)
    {
        action(
            permission_level{_self, name("active")},
            name("eosio.token"),
            name("transfer"),
            make_tuple(
                _self,
                settle->account,
                settle->eos,
                "pay app settle"
            )
        ).send();
    };

    if(settle->itam.amount > 0)
    {
        action(
            permission_level{_self, name("active")},
            name("itamtokenadm"),
            name("transfer"),
            make_tuple(
                _self,
                settle->account,
                settle->eos,
                "pay app settle"
            )
        ).send();
    }

    settles.modify(settle, _self, [&](auto &s) {
        s.eos.amount = 0;
        s.itam.amount = 0;
    });
}

ACTION itampayapp::setconfig(uint64_t ratio, uint64_t refundableDay)
{
    require_auth(_self);

    configTable configs(_self, _self.value);
    const auto& config = configs.find(_self.value);

    if(config == configs.end())
    {
        configs.emplace(_self, [&](auto &c) {
            c.key = _self;
            c.ratio = ratio;
            c.refundableDay = refundableDay;
        });
    }
    else
    {
        configs.modify(config, _self, [&](auto &c) {
            c.ratio = ratio;
            c.refundableDay = refundableDay;
        });
    }
}

ACTION itampayapp::receipt(uint64_t appId, asset amount, name owner, name buyer)
{
    require_auth(_self);
    require_recipient(_self, buyer);
}

ACTION itampayapp::defconfirm(uint64_t appId)
{
    confirm(appId);
}

ACTION itampayapp::menconfirm(uint64_t appId)
{
    confirm(appId);
}

void itampayapp::confirm(uint64_t appId)
{
    require_auth(_self);

    uint64_t currentTimestamp = now();

    configTable configs(_self, _self.value);
    const auto& config = configs.get(_self.value, "Can't find refundable day");

    pendingTable pendings(_self, appId);
    for(auto iter = pendings.begin(); iter != pendings.end(); iter++)
    {
        uint64_t maximumRefundable = iter->timestamp + (config.refundableDay * SECOND_OF_DAY);

        if(maximumRefundable < currentTimestamp)
        {
            pendings.erase(iter);
        }
    }
}

void itampayapp::transfer(uint64_t from, uint64_t to)
{
    auto data = unpack_action_data<transferData>();

    if (data.from == _self || data.to != _self)
    {
        return;
    }

    uint64_t appId = stoull(data.memo, 0, 10);

    appTable apps(_self, _self.value);
    const auto& app = apps.require_find(appId, "Invalid App Id");

    eosio_assert(data.quantity == app->amount, "Wrong Asset");

    configTable configs(_self, _self.value);
    const auto& config = configs.get(_self.value, "Can't find refundable day");

    pendingTable pendings(_self, appId);
    pendings.emplace(_self, [&](auto &p) {
        p.buyer = data.from;
        p.settleAmount = data.quantity * config.ratio / 100;
        p.timestamp = now();
    });

    action(
        permission_level{_self, name("active")},
        _self,
        name("receipt"),
        make_tuple(appId, app->amount, app->owner, data.from)
    ).send();

    transaction txn;
    txn.actions.emplace_back(
        permission_level(_self, name("active")),
        _self,
        name("defconfirm"),
        make_tuple(appId)
    );

    txn.delay_sec = config.refundableDay * SECOND_OF_DAY;
    txn.send(now(), _self);
}

#define EOSIO_DISPATCH_EX( TYPE, MEMBERS ) \
extern "C" { \
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
        bool can_call_transfer = code == name("eosio.token").value || code == name("itamtokenadm").value; \
        if( code == receiver || can_call_transfer ) { \
            if(action == name("transfer").value) { \
                eosio_assert(can_call_transfer, "eosio.token or itamtokenadm can call internal transfer"); \
            } \
            switch( action ) { \
                EOSIO_DISPATCH_HELPER( TYPE, MEMBERS ) \
            } \
        } \
    } \
} \

EOSIO_DISPATCH_EX( itampayapp, (test)(registapp)(removeapp)(refundapp)(modifysettle)(claimsettle)(setconfig)(menconfirm)(defconfirm)(transfer)(receipt) )