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

    symbol eosSymbol = symbol("EOS", 4);
    for(int i = 0; i < registItems.size(); i++)
    {
        items.emplace(_self, [&](auto &item) {
            string itemId = registItems[i]["itemId"];
            item.id = stoull(itemId, 0, 10);
            item.itemName = registItems[i]["itemName"];
            item.price.amount = stoull(replaceAll(registItems[i]["price"], ".", ""), 0, 10);
            item.price.symbol = eosSymbol;
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

ACTION itamstoreapp::refundapp(string appId, string owner, name ownerGroup)
{
    uint64_t appid = stoull(appId, 0, 10);

    appTable apps(_self, _self.value);
    const auto& app = apps.get(appid, "Invalid App Id");

    refund(appid, NULL, owner, ownerGroup);
}

ACTION itamstoreapp::refunditem(string appId, string itemId, string owner, name ownerGroup)
{
    uint64_t appid = stoull(appId, 0, 10);
    uint64_t itemid = stoull(itemId, 0, 10);

    itemTable items(_self, appid);
    const auto& item = items.get(itemid, "Invalid Item Id");

    refund(appid, itemid, owner, ownerGroup);
}

void itamstoreapp::refund(uint64_t appId, uint64_t itemId, string owner, name ownerGroup)
{
    require_auth(_self);
    const auto& config = configs.get(_self.value, "refundable day must be set first");

    name groupAccount = get_group_account(owner, ownerGroup);

    pendingTable pendings(_self, appId);
    const auto& pending = pendings.require_find(groupAccount.value, "owner group not found");

    pendings.modify(pending, _self, [&](auto &p) {
        for(auto info = p.infos[owner].begin(); info != p.infos[owner].end(); info++)
        {
            if(info->appId == appId && info->itemId == itemId)
            {
                uint64_t refundableTimestamp = info->timestamp + (config.refundableDay * SECONDS_OF_DAY);
                eosio_assert(refundableTimestamp >= now(), "refundable day has passed");

                string category = itemId == NULL ? "app" : "item";
                string transferMemo = "Refund " + category + ", appId: " + to_string(appId) + ", itemId: " + to_string(itemId);
                action(
                    permission_level { _self, name("active") },
                    name("eosio.token"),
                    name("transfer"),
                    make_tuple( _self, groupAccount, info->paymentAmount, transferMemo )
                ).send();

                if(p.infos[owner].size() == 1)
                {
                    p.infos.erase(owner);
                }
                else
                {
                    p.infos[owner].erase(info);
                }
                return;
            }
        }

        eosio_assert(false, "refund fail");
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

ACTION itamstoreapp::registapp(string appId, name owner, asset price, string params)
{
    uint64_t appid = stoull(appId, 0, 10);

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
    
    settleTable settles(_self, _self.value);
    const auto& settle = settles.find(appid);
    
    if(settle != settles.end())
    {
        eosio_assert(settle->settleAmount.amount == 0, "Can't remove app becuase it's not settled");
        settles.erase(settle);
    }

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

    name ownerGroup = name(memo.ownerGroup);
    name groupAccount = get_group_account(memo.owner, ownerGroup);
    eosio_assert(groupAccount == data.from, "different owner group");

    pendingTable pendings(_self, appId);
    const auto& pending = pendings.find(groupAccount.value);
    const auto& config = configs.get(_self.value, "refundable day must be set first");

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

ACTION itamstoreapp::defconfirm(uint64_t appId, string owner, name ownerGroup)
{
    confirm(appId, owner, ownerGroup);
}

ACTION itamstoreapp::menconfirm(string appId, string owner, name ownerGroup)
{
    uint64_t appid = stoull(appId, 0, 10);

    confirm(appid, owner, ownerGroup);
}

void itamstoreapp::confirm(uint64_t appId, const string& owner, name ownerGroup)
{
    require_auth(_self);

    const auto& config = configs.get(_self.value, "refundable day must be set first");
    
    settleTable settles(_self, _self.value);
    const auto& settle = settles.find(appId);

    name groupAccount = get_group_account(owner, ownerGroup);

    pendingTable pendings(_self, appId);
    const auto& pending = pendings.require_find(groupAccount.value, "invalid owner group");

    map<string, vector<pendingInfo>> infos = pending->infos;
    vector<pendingInfo> pendingInfos = infos[owner];
    
    uint64_t currentTimestamp = now();
    
    // HACK: can't get vector's iterator after erase item
    int deleteCount = 0;
    for(int i = 0; i < pendingInfos.size(); i++)
    {
        pendingInfo& info = pendingInfos[i - deleteCount];
        uint64_t refundableTimestamp = info.timestamp + (config.refundableDay * SECONDS_OF_DAY);

        if(refundableTimestamp < currentTimestamp)
        {
            asset settleAmountToITAM;

            if(settle != settles.end())
            {
                settleAmountToITAM = info.paymentAmount - info.settleAmount;
                settles.modify(settle, _self, [&](auto &s) {
                    s.settleAmount += info.settleAmount;
                });
            }
            else
            {
                settleAmountToITAM = info.paymentAmount;
            }
            
            action(
                permission_level{_self, name("active")},
                name("eosio.token"),
                name("transfer"),
                make_tuple(
                    _self,
                    name(ITAM_SETTLE_ACCOUNT),
                    settleAmountToITAM,
                    string("ITAM Store settlement")
                )
            ).send();

            pendings.modify(pending, _self, [&](auto &p) {
                p.infos[owner].erase(p.infos[owner].begin() + (i - deleteCount));
            });

            deleteCount += 1;
        }
    }
}

ACTION itamstoreapp::claimsettle(string appId)
{
    uint64_t appid = stoull(appId, 0, 10);

    require_auth(_self);

    settleTable settles(_self, _self.value);
    const auto& settle = settles.require_find(appid, "settle account not found");

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

ACTION itamstoreapp::setsettle(string appId, name account)
{
    uint64_t appid = stoull(appId, 0, 10);

    require_auth(_self);
    eosio_assert(is_account(account), "account does not exist");

    settleTable settles(_self, _self.value);
    const auto& settle = settles.find(appid);

    if(settle == settles.end())
    {
        settles.emplace(_self, [&](auto &s) {
            s.appId = appid;
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