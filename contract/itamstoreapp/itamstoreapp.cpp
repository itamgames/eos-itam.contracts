#include "itamstoreapp.hpp"

ACTION itamstoreapp::registitems(string params)
{
    require_auth(_self);

    auto parsedParams = json::parse(params);
    
    auto iter = parsedParams.find("appId");
    if(iter == parsedParams.end()) return;
    uint64_t appId = *iter;

    iter = parsedParams.find("items");
    if(iter == parsedParams.end()) return;
    auto registItems = *iter;
    
    itemTable items(_self, appId);

    for(auto item = items.begin(); item != items.end(); item = items.erase(item));

    symbol eosSymbol = symbol("EOS", 4);
    symbol itamSymbol = symbol("ITAM", 4);

    for(int i = 0; i < registItems.size(); i++)
    {
        items.emplace(_self, [&](auto &item) {
            item.itemId = registItems[i]["itemId"];
            item.itemName = registItems[i]["itemName"];
            item.eos.amount = stoull(replaceAll(registItems[i]["eos"], ".", ""), 0, 10);
            item.eos.symbol = eosSymbol;
            item.itam.amount = stoull(replaceAll(registItems[i]["itam"], ".", ""), 0, 10);
            item.itam.symbol = itamSymbol;
        });
    }
}

ACTION itamstoreapp::modifyitem(uint64_t appId, uint64_t itemId, string itemName, asset eos, asset itam)
{
    require_auth(_self);
    itemTable items(_self, appId);

    auto item = items.require_find(itemId, "Invalid item id");

    items.modify(item, _self, [&](auto& i){
        i.itemName = itemName;
        i.eos = eos;
        i.itam = itam;
    });
}

ACTION itamstoreapp::deleteitems(string params)
{
    require_auth(_self);
    auto parsedParams = json::parse(params);

    uint64_t appId = parsedParams["appId"];
    auto deleteItems = parsedParams["items"];

    itemTable items(_self, appId);

    for(int i = 0; i < deleteItems.size(); i++)
    {
        auto item = items.require_find(deleteItems[i]["itemId"], "Invalid item id");
        items.erase(item);
    }
}

ACTION itamstoreapp::refunditem(uint64_t appId, uint64_t itemId, name buyer)
{
    require_auth(_self);

    itemTable items(_self, appId);
    const auto& item = items.get(itemId, "Invalid item id");

    pendingTable pendings(_code, appId);
    const auto& pending = pendings.require_find(buyer.value, "refundable customer not found");

    const auto& config = configs.get(_self.value, "refundable day must be set first");

    auto& pendingList = pending->pendingList;
    for(int i = 0; i < pendingList.size(); i++)
    {
        if(pendingList[i].appId == appId && pendingList[i].itemId == itemId)
        {
            uint64_t refundableTimestamp = pendingList[i].timestamp + (config.refundableDay * SECONDS_OF_DAY);
            eosio_assert(refundableTimestamp >= now(), "Refundable day has passed");

            action(
                permission_level{_self, name("active")},
                name(item.eos.amount > 0 ? "eosio.token" : "itamtokenadm"),
                name("transfer"),
                make_tuple(_self, buyer, item.eos.amount > 0 ? item.eos : item.itam, string("refund pay app"))
            ).send();

            pendings.modify(pending, _self, [&](auto &p) {
                p.pendingList.erase(pendingList.begin() + i);
            });

            if(pendingList.size() == 0)
            {
                pendings.erase(pending);
            }

            return;
        }
    }

    eosio_assert(false, "can't find item");
}

ACTION itamstoreapp::useitem(uint64_t appId, uint64_t itemId, string memo)
{
    require_auth(_self);
    
    itemTable items(_self, appId);
    items.get(itemId, "invalid item");
}

ACTION itamstoreapp::registapp(uint64_t appId, name owner, asset amount, string params)
{
    require_auth(_self);

    eosio_assert(amount.symbol == symbol("EOS", 4) || amount.symbol == symbol("ITAM", 4), "Invalid symbol");

    action(
        permission_level{_self, name("active")},
        _self,
        name("registitems"),
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
    }
    else
    {
        apps.modify(app, _self, [&](auto &a) {
            a.owner = owner;
            a.amount = amount;
        });
    }
}

ACTION itamstoreapp::deleteapp(uint64_t appId)
{
    require_auth(_self);

    appTable apps(_self, _self.value);
    const auto& app = apps.require_find(appId, "Invalid App Id");
    apps.erase(app);
    
    settleTable settles(_self, _self.value);
    const auto &settle = settles.require_find(appId, "Can't find settle states");
    eosio_assert(settle->eos.amount == 0 && settle->itam.amount == 0, "Can't remove app becuase it's not settled");
    settles.erase(settle);

    itemTable items(_self, appId);
    for(auto item = items.begin(); item != items.end(); item = items.erase(item));
}

ACTION itamstoreapp::refundapp(uint64_t appId, name buyer)
{
    require_auth(_self);

    appTable apps(_self, _self.value);
    const auto& app = apps.get(appId, "Invalid App Id");

    pendingTable pendings(_self, appId);
    const auto& pending = pendings.require_find(buyer.value, "Can't find refundable customer");

    const auto& config = configs.get(_self.value, "refundable day must be set first");

    auto& pendingList = pending->pendingList;
    for(int i = 0; i < pendingList.size(); i++)
    {
        if(pendingList[i].appId == appId && pendingList[i].itemId == 0)
        {
            uint64_t refundableTimestamp = pendingList[i].timestamp + (config.refundableDay * SECONDS_OF_DAY);
            eosio_assert(refundableTimestamp >= now(), "Refundable day has passed");

            action(
                permission_level{_self, name("active")},
                name(symbol("EOS", 4) == app.amount.symbol ? "eosio.token" : "itamtokenadm"),
                name("transfer"),
                make_tuple(_self, buyer, app.amount, string("refund pay app"))
            ).send();

            pendings.modify(pending, _self, [&](auto &p) {
                p.pendingList.erase(pendingList.begin() + i);
            });

            // TODO: test
            if(pendingList.size() == 0)
            {
                pendings.erase(pending);
            }

            return;
        }
    }

    eosio_assert(false, "can't find app");
}

ACTION itamstoreapp::transfer(uint64_t from, uint64_t to)
{
    transferData data = unpack_action_data<transferData>();
    if (data.from == _self || data.to != _self) return;
    
    memoData memo;
    parseMemo(&memo, data.memo, "|");

    if(memo.category == "app")
    {
        transferApp(data, memo);
    }
    else if(memo.category == "item")
    {
        transferItem(data, memo);
    }
    else
    {
        eosio_assert(false, "invalid category");
    }
    

    transaction tx;
    tx.actions.emplace_back(
        permission_level(_self, name("active")),
        _self,
        name("defconfirm"),
        make_tuple(memo.appId)
    );

    const auto& config = configs.get(_self.value, "refundable day must be set first");
    
    tx.delay_sec = config.refundableDay * SECONDS_OF_DAY;
    tx.send(now(), _self);
}

void itamstoreapp::transferApp(transferData& txData, memoData& memo)
{
    uint64_t appId = stoull(memo.appId, 0, 10);
    appTable apps(_self, _self.value);
    const auto& app = apps.get(appId, "Invalid app id");

    eosio_assert(txData.quantity == app.amount, "Invalid purchase value");

    action(
        permission_level{_self, name("active")},
        _self, 
        name("receiptapp"),
        make_tuple(appId, txData.from, txData.quantity)
    ).send();

    setPendingTable(appId, NULL, txData.from, txData.quantity);
}

void itamstoreapp::transferItem(transferData& txData, memoData& memo)
{
    uint64_t appId = stoull(memo.appId, 0, 10);
    uint64_t itemId = stoull(memo.itemId, 0, 10);

    itemTable items(_self, appId);
    const auto& item = items.get(itemId, "Invalid item id");

    asset amount = item.eos.amount > 0 ? item.eos : item.itam;

    eosio_assert(txData.quantity == amount, "Invalid purchase value");

    action(
        permission_level{_self, name("active")},
        _self, 
        name("receiptitem"),
        make_tuple(appId, itemId, item.itemName, memo.token, txData.from, txData.quantity)
    ).send();

    setPendingTable(appId, itemId, txData.from, txData.quantity);
}

ACTION itamstoreapp::receiptapp(uint64_t appId, name from, asset quantity)
{
    require_auth(_self);
    require_recipient(_self, from);
}

ACTION itamstoreapp::receiptitem(uint64_t appId, uint64_t itemId, string itemName, string token, name from, asset quantity)
{
    require_auth(_self);
    require_recipient(_self, from);
}

ACTION itamstoreapp::defconfirm(uint64_t appId)
{
    confirm(appId);
}

ACTION itamstoreapp::menconfirm(uint64_t appId)
{
    confirm(appId);
}

void itamstoreapp::confirm(uint64_t appId)
{
    require_auth(_self);

    uint64_t currentTimestamp = now();

    const auto& config = configs.get(_self.value, "Can't find refundable day");

    settleTable settles(_self, _self.value);
    const auto& settle = settles.require_find(appId, "Settlement account not found");

    symbol eosSymbol = symbol("EOS", 4);
    pendingTable pendings(_self, appId);
    for(auto pending = pendings.begin(); pending != pendings.end();)
    {
        auto& pendingList = pending->pendingList;
        pendings.modify(pending, _self, [&](auto &p) {
            for(auto info = pendingList.begin(); info != pendingList.end();)
            {
                uint64_t maximumRefundable = info->timestamp + (config.refundableDay * SECONDS_OF_DAY);

                if(maximumRefundable < currentTimestamp)
                {
                    settles.modify(settle, _self, [&](auto &s) {
                        if(info->settleAmount.symbol == eosSymbol)
                        {
                            s.eos += info->settleAmount;
                        }
                        else
                        {
                            s.itam += info->settleAmount;
                        }
                    });
                    p.pendingList.erase(info);
                }
                else
                {
                    info++;
                }
            }
        });

        pending = pendingList.size() == 0 ? pendings.erase(pending) : ++pending;
    }
}

ACTION itamstoreapp::claimsettle(uint64_t appId)
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
                string("ITAM Store settlement")
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
                string("ITAM Store settlement")
            )
        ).send();
    }

    settles.modify(settle, _self, [&](auto &s) {
        s.eos.amount = 0;
        s.itam.amount = 0;
    });
}

ACTION itamstoreapp::setconfig(uint64_t ratio, uint64_t refundableDay)
{
    require_auth(_self);

    const auto& config = configs.find(_self.value);

    if(config == configs.end())
    {
        configs.emplace(_self, [&](auto& c) {
            c.key = _self;
            c.ratio = ratio;
            c.refundableDay = refundableDay;
        });
    }
    else
    {
        configs.modify(config, _self, [&](auto& c) {
            c.ratio = ratio;
            c.refundableDay = refundableDay;
        });
    }
}

ACTION itamstoreapp::setsettle(uint64_t appId, name account)
{
    require_auth(_self);

    settleTable settles(_self, _self.value);
    const auto& settle = settles.find(appId);

    if(settle == settles.end())
    {
        settles.emplace(_self, [&](auto &s) {
            s.appId = appId;
            s.account = account;
            s.eos = asset(0, symbol("EOS", 4));
            s.itam = asset(0, symbol("ITAM", 4));
        });
    }
    else
    {
        settles.modify(settle, _self, [&](auto &s) {
            s.account = account;
        });
    }
}

void itamstoreapp::setPendingTable(uint64_t appId, uint64_t itemId, name from, asset quantity)
{
    const auto& config = configs.get(_self.value, "refundable day must be set first");

    pendingTable pendings(_self, appId);
    const auto& pending = pendings.find(from.value);

    pendingInfo info{appId, itemId, quantity * config.ratio / 100, now()};
    if(pending == pendings.end())
    {
        pendings.emplace(_self, [&](auto &p) {
            p.buyer = from;
            p.pendingList.push_back(info);
        });
    }
    else
    {
        pendings.modify(pending, _self, [&](auto &p) {
            p.pendingList.push_back(info);
        });
    }
}