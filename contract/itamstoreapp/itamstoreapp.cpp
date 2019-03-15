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
    for(int i = 0; i < registItems.size(); i++)
    {
        items.emplace(_self, [&](auto &item) {
            item.itemId = registItems[i]["itemId"];
            item.itemName = registItems[i]["itemName"];
            item.price.amount = stoull(replaceAll(registItems[i]["price"], ".", ""), 0, 10);
            item.price.symbol = eosSymbol;
        });
    }
}

ACTION itamstoreapp::modifyitem(uint64_t appId, uint64_t itemId, string itemName, asset price)
{
    require_auth(_self);

    itemTable items(_self, appId);
    const auto& item = items.require_find(itemId, "Invalid item id");

    items.modify(item, _self, [&](auto& i){
        i.itemName = itemName;
        i.price = price;
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

ACTION itamstoreapp::refundapp(uint64_t appId, string owner, name ownerGroup)
{
    appTable apps(_self, _self.value);
    const auto& app = apps.get(appId, "Invalid App Id");

    refund(appId, NULL, owner, ownerGroup, app.price);
}

ACTION itamstoreapp::refunditem(uint64_t appId, uint64_t itemId, string owner, name ownerGroup)
{
    itemTable items(_self, appId);
    const auto& item = items.get(itemId, "Invalid Item Id");

    refund(appId, itemId, owner, ownerGroup, item.price);
}

void itamstoreapp::refund(uint64_t appId, uint64_t itemId, string owner, name ownerGroup, asset refund)
{
    require_auth(_self);
    const auto& config = configs.get(_self.value, "refundable day must be set first");

    pendingTable pendings(_code, appId);
    const auto& pending = pendings.require_find(ownerGroup.value, "refundable customer not found");
    auto& pendingList = pending->pendingList;
    for(int i = 0; i < pendingList.size(); i++)
    {
        pendingInfo info = pendingList[i];
        if(info.appId == appId && info.itemId == itemId)
        {
            uint64_t refundableTimestamp = pendingList[i].timestamp + (config.refundableDay * SECONDS_OF_DAY);
            eosio_assert(refundableTimestamp >= now(), "Refundable day has passed");

            name groupAccount = getGroupAccount(owner, ownerGroup);
            action(
                permission_level{_self, name("active")},
                name("eosio.token"),
                name("transfer"),
                make_tuple(_self, groupAccount, refund, string("refund pay app"))
            ).send();

            if(pendingList.size() == 1)
            {
                pendings.erase(pending);
            }
            else
            {
                pendings.modify(pending, _self, [&](auto &p) {
                    p.pendingList.erase(pendingList.begin() + i);
                });
            }
            return;
        }
    }

    eosio_assert(false, "can't find pending product");
}

ACTION itamstoreapp::useitem(uint64_t appId, uint64_t itemId, string memo)
{
    require_auth(_self);
    
    itemTable items(_self, appId);
    items.get(itemId, "invalid item");
}

ACTION itamstoreapp::registapp(uint64_t appId, name owner, asset price, string params)
{
    require_auth(_self);
    eosio_assert(price.symbol == symbol("EOS", 4), "only eos symbol available");

    SEND_INLINE_ACTION(
        *this,
        registitems,
        { { _self, name("active") } },
        { params }
    );

    if(price.amount == 0) return;

    appTable apps(_self, _self.value);
    const auto& app = apps.find(appId);
    if(app == apps.end())
    {
        apps.emplace(_self, [&](auto& a) {
            a.appId = appId;
            a.owner = owner;
            a.price = price;
        });
    }
    else
    {
        apps.modify(app, _self, [&](auto& a) {
            a.owner = owner;
            a.price = price;
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
    const auto& settle = settles.require_find(appId, "Can't find settle states");
    eosio_assert(settle->settleAmount.amount == 0, "Can't remove app becuase it's not settled");
    settles.erase(settle);

    itemTable items(_self, appId);
    for(auto item = items.begin(); item != items.end(); item = items.erase(item));
}

ACTION itamstoreapp::transfer(uint64_t from, uint64_t to)
{
    transferData data = unpack_action_data<transferData>();
    if (data.from == _self || data.to != _self) return;
    
    memoData memo;
    parseMemo(&memo, data.memo, "|");

    uint64_t appId = stoull(memo.appId, 0, 10);
    uint64_t itemId = 0;

    name ownerGroup = name(memo.ownerGroup);
    eosio_assert(getGroupAccount(memo.owner, ownerGroup) == data.from, "different owner group");

    if(memo.category == "app")
    {
        appTable apps(_self, _self.value);
        const auto& app = apps.get(appId, "Invalid app id");

        eosio_assert(data.quantity == app.price, "Invalid purchase value");
        SEND_INLINE_ACTION(
            *this,
            receiptapp,
            { { _self, name("active") } },
            { appId, data.from, memo.owner, ownerGroup, data.quantity }
        );
    }
    else if(memo.category == "item")
    {
        itemId = stoull(memo.itemId, 0, 10);

        itemTable items(_self, appId);
        const auto& item = items.get(itemId, "Invalid item id");

        eosio_assert(data.quantity == item.price, "Invalid purchase value");
        SEND_INLINE_ACTION(
            *this,
            receiptitem,
            { { _self, name("active") } },
            { appId, itemId, item.itemName, data.from, memo.owner, ownerGroup, data.quantity }
        );
    }
    else eosio_assert(false, "invalid category");

    const auto& config = configs.get(_self.value, "refundable day must be set first");

    pendingTable pendings(_self, appId);
    const auto& pending = pendings.find(ownerGroup.value);

    pendingInfo info{appId, itemId, memo.owner, data.quantity * config.ratio / 100, now()};
    if(pending == pendings.end())
    {
        pendings.emplace(_self, [&](auto &p) {
            p.ownerGroup = ownerGroup;
            p.pendingList.push_back(info);
        });
    }
    else
    {
        pendings.modify(pending, _self, [&](auto &p) {
            p.pendingList.push_back(info);
        });
    }

    transaction tx;
    tx.actions.emplace_back(
        permission_level(_self, name("active")),
        _self,
        name("defconfirm"),
        make_tuple(memo.appId)
    );
    
    tx.delay_sec = config.refundableDay * SECONDS_OF_DAY;
    tx.send(now(), _self);
}

ACTION itamstoreapp::receiptapp(uint64_t appId, name from, string owner, name ownerGroup, asset quantity)
{
    require_auth(_self);
    require_recipient(_self, from);
}

ACTION itamstoreapp::receiptitem(uint64_t appId, uint64_t itemId, string itemName, name from, string owner, name ownerGroup, asset quantity)
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
                        s.settleAmount += info->settleAmount;
                    });
                    info = p.pendingList.erase(info);
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

    if(settle->settleAmount.amount > 0)
    {
        action(
            permission_level{_self, name("active")},
            name("eosio.token"),
            name("transfer"),
            make_tuple(
                _self,
                settle->account,
                settle->settleAmount,
                string("ITAM Store settlement")
            )
        ).send();
    };

    settles.modify(settle, _self, [&](auto &s) {
        s.settleAmount.amount = 0;
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
            s.settleAmount = asset(0, symbol("EOS", 4));
        });
    }
    else
    {
        settles.modify(settle, _self, [&](auto &s) {
            s.account = account;
        });
    }
}

name itamstoreapp::getGroupAccount(const string& owner, name ownerGroup)
{
    if(ownerGroup.to_string() == "eos")
    {
        return name(owner);
    }

    name digitalAssetName = name("itamdigasset");
    ownergroupTable ownergroups(digitalAssetName, digitalAssetName.value);
    return ownergroups.get(ownerGroup.value, "invalid owner group").account;
}