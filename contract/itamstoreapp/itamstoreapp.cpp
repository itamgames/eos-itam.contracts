#include "itamstoreapp.hpp"

ACTION itamstoreapp::registitems(string appId, string delimitedItems)
{
    require_auth(_self);

    vector<string> parsedItems;
    split(parsedItems, delimitedItems, "|");

    const uint32_t itemParamSize = 4;
    eosio_assert(parsedItems.size() % itemParamSize == 0, "invalid params");

    uint64_t appid = stoull(appId, 0, 10);
    itemTable items(_self, appid);
    for(auto item = items.begin(); item != items.end(); item = items.erase(item));

    for(int i = 0; i < parsedItems.size(); i += itemParamSize)
    {
        items.emplace(_self, [&](auto &item) {
            item.id = stoull(parsedItems[i], 0, 10);
            item.itemName = parsedItems[i + 1];
            item.price.amount = stoull(replaceAll(parsedItems[i + 2], ".", ""), 0, 10);
            item.price.symbol = symbol(parsedItems[i + 3], 4);
        });
    }
}

ACTION itamstoreapp::modifyitem(string appId, string itemId, string itemName, asset price)
{
    require_auth(_self);

    uint64_t appid = stoull(appId, 0, 10);
    uint64_t itemid = stoull(itemId, 0, 10);

    itemTable items(_self, appid);
    const auto& item = items.require_find(itemid, "invalid item id");

    items.modify(item, _self, [&](auto& i){
        i.itemName = itemName;
        i.price = price;
    });
}

ACTION itamstoreapp::useitem(string appId, string itemId, string memo)
{
    uint64_t appid = stoull(appId, 0, 10);
    uint64_t itemid = stoull(itemId, 0, 10);

    require_auth(_self);
    
    itemTable items(_self, appid);
    items.get(itemid, "invalid item");
}

ACTION itamstoreapp::registapp(string appId, name owner, asset price)
{
    require_auth(_self);

    uint64_t appid = stoull(appId, 0, 10);
    eosio_assert(price.amount > 0, "amount must be positive.");

    appTable apps(_self, _self.value);
    const auto& app = apps.find(appid);
    if(app == apps.end())
    {
        apps.emplace(_self, [&](auto& a) {
            a.id = appid;
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

ACTION itamstoreapp::deleteapp(string appId)
{
    uint64_t appid = stoull(appId, 0, 10);

    require_auth(_self);

    appTable apps(_self, _self.value);
    const auto& app = apps.require_find(appid, "Invalid App Id");
    apps.erase(app);
    
    itemTable items(_self, appid);
    for(auto item = items.begin(); item != items.end(); item = items.erase(item));
}

ACTION itamstoreapp::transfer(uint64_t from, uint64_t to)
{
    transferData data = unpack_action_data<transferData>();
    if (data.from == _self || data.to != _self || data.from == name("itamgamesgas")) return;
    
    memoData memo;
    parseMemo(&memo, data.memo, "|", 5);

    uint64_t appId = stoull(memo.appId, 0, 10);
    uint64_t itemId = 0;

    name owner = name(memo.owner);
    name ownerGroup = name(memo.ownerGroup);
    name groupAccount = get_group_account(owner, ownerGroup);
    eosio_assert(groupAccount == data.from, "different owner group");
#ifndef BETA
    pendingTable pendings(_self, appId);
    const auto& pending = pendings.find(groupAccount.value);
    const auto& config = configs.get(_self.value, "refundable day must be set first");
#endif
    if(memo.category == "app")
    {
        appTable apps(_self, _self.value);
        const auto& app = apps.get(appId, "Invalid app id");

        eosio_assert(data.quantity == app.price, "Invalid purchase value");
        SEND_INLINE_ACTION(
            *this,
            receiptapp,
            { { _self, name("active") } },
            { appId, data.from, owner, ownerGroup, data.quantity }
        );
#ifndef BETA
        if(pending != pendings.end())
        {
            map<string, vector<pendingInfo>> infos = pending->infos;
            vector<pendingInfo> ownerPendings = infos[memo.owner];

            for(auto iter = ownerPendings.begin(); iter != ownerPendings.end(); iter++)
            {
                if(iter->appId == appId && iter->itemId == 0)
                {
                    eosio_assert(false, "Already purchased account");
                }
            }
        }
#endif
    }
    else if(memo.category == "item")
    {
        itemId = stoull(memo.itemId, 0, 10);

        itemTable items(_self, appId);
        const auto& item = items.get(itemId, "invalid item id");

        eosio_assert(data.quantity == item.price, "invalid purchase value");
        SEND_INLINE_ACTION(
            *this,
            receiptitem,
            { { _self, name("active") } },
            { appId, itemId, item.itemName, data.from, owner, ownerGroup, data.quantity }
        );
    }
    else eosio_assert(false, "invalid category");
#ifndef BETA
    uint64_t currentTimestamp = now();
    pendingInfo info{ appId, itemId, data.quantity, data.quantity * config.ratio / 100, currentTimestamp };

    if(pending == pendings.end())
    {
        pendings.emplace(_self, [&](auto &p) {
            p.groupAccount = groupAccount;
            p.infos[memo.owner].push_back(info);
        });
    }
    else
    {
        pendings.modify(pending, _self, [&](auto &p) {
            p.infos[memo.owner].push_back(info);
        });
    }
        
    transaction tx;
    tx.actions.emplace_back(
        permission_level(_self, name("active")),
        _self,
        name("defconfirm"),
        make_tuple(appId, memo.owner, ownerGroup)
    );
        
    tx.delay_sec = (config.refundableDay * SECONDS_OF_DAY) + 1;
    tx.send(now(), _self);
#endif
}

ACTION itamstoreapp::receiptapp(uint64_t appId, name from, name owner, name ownerGroup, asset quantity)
{
    require_auth(_self);
    require_recipient(_self, from);
}

ACTION itamstoreapp::receiptitem(uint64_t appId, uint64_t itemId, string itemName, name from, name owner, name ownerGroup, asset quantity)
{
    require_auth(_self);
    require_recipient(_self, from);
}

#ifndef BETA
ACTION itamstoreapp::refundapp(string appId, name owner, name ownerGroup)
{
    uint64_t appid = stoull(appId, 0, 10);

    appTable apps(_self, _self.value);
    const auto& app = apps.get(appid, "Invalid App Id");

    refund(appid, NULL, owner, ownerGroup);
}

ACTION itamstoreapp::refunditem(string appId, string itemId, name owner, name ownerGroup)
{
    uint64_t appid = stoull(appId, 0, 10);
    uint64_t itemid = stoull(itemId, 0, 10);

    itemTable items(_self, appid);
    const auto& item = items.get(itemid, "Invalid Item Id");

    refund(appid, itemid, owner, ownerGroup);
}

void itamstoreapp::refund(uint64_t appId, uint64_t itemId, const name& owner, const name& ownerGroup)
{
    require_auth(_self);
    const auto& config = configs.get(_self.value, "refundable day must be set first");

    name groupAccount = get_group_account(owner, ownerGroup);

    pendingTable pendings(_self, appId);
    const auto& pending = pendings.require_find(groupAccount.value, "owner group not found");

    string owner_str = owner.to_string();
    name eosio_token("eosio.token");
    pendings.modify(pending, _self, [&](auto &p) {
        for(auto info = p.infos[owner_str].begin(); info != p.infos[owner_str].end(); info++)
        {
            if(info->appId == appId && info->itemId == itemId)
            {
                uint64_t refundableTimestamp = info->timestamp + (config.refundableDay * SECONDS_OF_DAY);
                eosio_assert(refundableTimestamp >= now(), "refundable day has passed");

                action(
                    permission_level { _self, name("active") },
                    name(eosio_token),
                    name("transfer"),
                    make_tuple( _self, groupAccount, info->paymentAmount, owner_str )
                ).send();

                if(p.infos[owner_str].size() == 1)
                {
                    p.infos.erase(owner_str);
                }
                else
                {
                    p.infos[owner_str].erase(info);
                }
                return;
            }
        }

        eosio_assert(false, "refund fail");
    });
}

ACTION itamstoreapp::defconfirm(uint64_t appId, name owner, name ownerGroup)
{
    confirm(appId, owner, ownerGroup);
}

ACTION itamstoreapp::menconfirm(string appId, name owner, name ownerGroup)
{
    uint64_t appid = stoull(appId, 0, 10);

    confirm(appid, owner, ownerGroup);
}

void itamstoreapp::confirm(uint64_t appId, const name& owner, const name& ownerGroup)
{
    require_auth(_self);

    const auto& config = configs.get(_self.value, "refundable day must be set first");
    
    name groupAccount = get_group_account(owner, ownerGroup);

    pendingTable pendings(_self, appId);
    const auto& pending = pendings.require_find(groupAccount.value, "invalid owner group");

    pendings.modify(pending, _self, [&](auto &p) {
        string owner_str = owner.to_string();
        auto pendingInfosIter = p.infos.find(owner_str);
        if(pendingInfosIter == p.infos.end()) return;

        vector<pendingInfo>& pendingInfos = pendingInfosIter->second;
        uint64_t currentTimestamp = now();
        name eosio_token("eosio.token");
        for(auto iter = pendingInfos.begin(); iter != pendingInfos.end();)
        {
            uint64_t refundableTimestamp = iter->timestamp + (config.refundableDay * SECONDS_OF_DAY);
            if(refundableTimestamp < currentTimestamp)
            {
                asset settleQuantityToVendor = iter->settleAmount;
                asset settleQuantityToITAM = iter->paymentAmount - settleQuantityToVendor;

                if(settleQuantityToVendor.amount > 0)
                {
                    action(
                        permission_level{ _self, name("active") },
                        name(eosio_token),
                        name("transfer"),
                        make_tuple(
                            _self,
                            name(CENTRAL_SETTLE_ACCOUNT),
                            settleQuantityToVendor,
                            to_string(appId)
                        )
                    ).send();
                }

                if(settleQuantityToITAM.amount > 0)
                {
                    action(
                        permission_level{ _self, name("active") },
                        name(eosio_token),
                        name("transfer"),
                        make_tuple(
                            _self,
                            name(ITAM_SETTLE_ACCOUNT),
                            settleQuantityToITAM,
                            to_string(appId)
                        )
                    ).send();
                }
                iter = pendingInfos.erase(iter);
            } else iter++;
        }

        // delete owner
        if(pendingInfos.size() == 0)
        {
            p.infos.erase(owner_str);
        }
    });

    // delete group account row
    if(pending->infos.size() == 0)
    {
        pendings.erase(pending);
    }
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
#endif