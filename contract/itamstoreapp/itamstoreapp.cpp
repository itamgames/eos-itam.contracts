#include "itamstoreapp.hpp"

ACTION itamstoreapp::registitems(string params)
{
    require_auth(_self);

    auto parsedParams = json::parse(params);
    
    auto iter = parsedParams.find("appId");
    if(iter == parsedParams.end()) return;
    
    string appid = *iter;
    uint64_t appId = stoull(appid, 0, 10);

    iter = parsedParams.find("items");
    if(iter == parsedParams.end()) return;
    auto registItems = *iter;
    
    itemTable items(_self, appId);
    for(auto item = items.begin(); item != items.end(); item = items.erase(item));

    for(int i = 0; i < registItems.size(); i++)
    {
        vector<string> itemAsset;
        split(itemAsset, registItems[i]["price"], " ");

        items.emplace(_self, [&](auto &item) {
            string itemId = registItems[i]["itemId"];
            item.id = stoull(itemId, 0, 10);
            item.itemName = registItems[i]["itemName"];
            item.price.amount = stoull(replaceAll(itemAsset[0], ".", ""), 0, 10);
            item.price.symbol = symbol(itemAsset[1], 4);
        });
    }
}

ACTION itamstoreapp::modifyitem(string appId, string itemId, string itemName, asset price)
{
    require_auth(_self);

    uint64_t appid = stoull(appId, 0, 10);
    uint64_t itemid = stoull(itemId, 0, 10);

    itemTable items(_self, appid);
    const auto& item = items.require_find(itemid, "Invalid item id");

    items.modify(item, _self, [&](auto& i){
        i.itemName = itemName;
        i.price = price;
    });
}

ACTION itamstoreapp::deleteitems(string params)
{
    require_auth(_self);
    auto parsedParams = json::parse(params);

    uint64_t appId = stoull(string(parsedParams["appId"]), 0, 10);
    auto deleteItems = parsedParams["items"];

    itemTable items(_self, appId);

    for(int i = 0; i < deleteItems.size(); i++)
    {
        auto item = items.require_find(stoull(string(deleteItems[i]["itemId"]), 0, 10), "Invalid item id");
        items.erase(item);
    }
}

ACTION itamstoreapp::useitem(string appId, string itemId, string memo)
{
    uint64_t appid = stoull(appId, 0, 10);
    uint64_t itemid = stoull(itemId, 0, 10);

    require_auth(_self);
    
    itemTable items(_self, appid);
    items.get(itemid, "invalid item");
}

ACTION itamstoreapp::registapp(string appId, name owner, asset price, string params)
{
    require_auth(_self);

    uint64_t appid = stoull(appId, 0, 10);

    SEND_INLINE_ACTION(
        *this,
        registitems,
        { { _self, name("active") } },
        { params }
    );

    if(price.amount == 0) return;

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
    paymentTable payments(_self, appId);
    const auto& payment = payments.find(owner.value);
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
            if(payment != payments.end())
            {
                const vector<paymentInfo>& progress = payment->progress;

                for(auto iter = progress.begin(); iter != progress.end(); iter++)
                {
                    if(iter->itemId == 0)
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
        const auto& item = items.get(itemId, "Invalid item id");

        eosio_assert(data.quantity == item.price, "Invalid purchase value");
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
        paymentInfo info{ itemId, data.quantity, data.quantity * config.ratio / 100, currentTimestamp };

        if(payment == payments.end())
        {
            payments.emplace(_self, [&](auto &p) {
                p.owner = owner;
                p.ownerGroup = ownerGroup;
                p.progress.push_back(info);
            });
        }
        else
        {
            payments.modify(payment, _self, [&](auto &p) {
                p.progress.push_back(info);
            });
        }
        
        transaction tx;
        tx.actions.emplace_back(
            permission_level(_self, name("active")),
            _self,
            name("defconfirm"),
            make_tuple(appId, memo.owner)
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
ACTION itamstoreapp::refundapp(string appId, name owner)
{
    uint64_t appid = stoull(appId, 0, 10);

    appTable apps(_self, _self.value);
    const auto& app = apps.get(appid, "Invalid App Id");

    refund(appid, NULL, owner);
}

ACTION itamstoreapp::refunditem(string appId, string itemId, name owner)
{
    uint64_t appid = stoull(appId, 0, 10);
    uint64_t itemid = stoull(itemId, 0, 10);

    itemTable items(_self, appid);
    const auto& item = items.get(itemid, "Invalid Item Id");

    refund(appid, itemid, owner);
}

void itamstoreapp::refund(uint64_t appId, uint64_t itemId, const name& owner)
{
    require_auth(_self);
    const auto& config = configs.get(_self.value, "refundable day must be set first");

    paymentTable payments(_self, appId);
    const auto& payment = payments.require_find(owner.value, "payment not found");

    name eosAccount = get_group_account(owner, payment->ownerGroup);

    payments.modify(payment, _self, [&](auto &p) {
        vector<paymentInfo>& progress = p.progress;
        for(auto info = progress.begin(); info != progress.end(); info++)
        {
            if(info->itemId == itemId)
            {
                uint64_t refundableTimestamp = info->timestamp + (config.refundableDay * SECONDS_OF_DAY);
                eosio_assert(refundableTimestamp >= now(), "refundable day has passed");

                action(
                    permission_level { _self, name("active") },
                    name("eosio.token"),
                    name("transfer"),
                    make_tuple( _self, eosAccount, info->paymentAmount, owner.to_string() )
                ).send();

                progress.erase(info);
                return;
            }
        }

        eosio_assert(false, "refund failed");
    });

    if(payment->progress.size() == 0)
    {
        payments.erase(payment);
    }
}

ACTION itamstoreapp::defconfirm(uint64_t appId, name owner)
{
    confirm(appId, owner);
}

ACTION itamstoreapp::menconfirm(string appId, name owner)
{
    uint64_t appid = stoull(appId, 0, 10);

    confirm(appid, owner);
}

void itamstoreapp::confirm(uint64_t appId, const name& owner)
{
    require_auth(_self);

    paymentTable payments(_self, appId);
    const auto& payment = payments.find(owner.value);
    if(payment == payments.end()) return;

    payments.modify(payment, _self, [&](auto &p) {
        uint64_t currentTimestamp = now();
        const auto& config = configs.get(_self.value, "refundable day must be set first");

        vector<paymentInfo>& progress = p.progress;

        for(auto pending = progress.begin(); pending != progress.end();)
        {
            uint64_t refundableTimestamp = pending->timestamp + (config.refundableDay * SECONDS_OF_DAY);
            if(refundableTimestamp < currentTimestamp)
            {
                asset settleQuantityToVendor = pending->settleAmount;
                asset settleQuantityToITAM = pending->paymentAmount - settleQuantityToVendor;

                if(settleQuantityToVendor.amount > 0)
                {
                    action(
                        permission_level{ _self, name("active") },
                        name("eosio.token"),
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
                        name("eosio.token"),
                        name("transfer"),
                        make_tuple(
                            _self,
                            name(ITAM_SETTLE_ACCOUNT),
                            settleQuantityToITAM,
                            to_string(appId)
                        )
                    ).send();
                }
                pending = progress.erase(pending);
            }
            else
            {
                pending++;
            }
        }
    });

    if(payment->progress.size() == 0)
    {
        payments.erase(payment);
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