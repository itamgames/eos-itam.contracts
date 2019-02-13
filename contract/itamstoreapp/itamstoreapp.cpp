#include "itamstoreapp.hpp"

ACTION itamstoreapp::registitems(string params)
{
    require_auth(_self);

    auto parsedParams = json::parse(params);

    if(parsedParams.count("appId") == 0 || parsedParams.count("items") == 0) return;

    uint64_t appId = parsedParams["appId"];
    auto registItems = parsedParams["items"];
    
    itemTable items(_self, appId);

    for(auto item = items.begin(); item != items.end(); item = items.erase(item));

    symbol eosSymbol = symbol("EOS", 4);
    symbol itamSymbol = symbol("ITAM", 4);

    for(int i = 0; i < registItems.size(); i++)
    {
        items.emplace(_self, [&](auto &item) {
            item.itemId = registItems[i]["itemId"];
            item.itemName = registItems[i]["itemName"];
            item.eos.amount = stoull(registItems[i]["eos"].get<std::string>(), 0, 10) * 10000;
            item.eos.symbol = eosSymbol;
            item.itam.amount = stoull(registItems[i]["itam"].get<std::string>(), 0, 10) * 10000;
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

ACTION itamstoreapp::registboard(uint64_t appId, name owner, string boardList)
{
    require_auth(_self);
    eosio_assert(is_account(owner), "Invalid owner");

    leaderboardTable boards(_self, appId);
    for(auto iter = boards.begin(); iter != boards.end(); iter = boards.erase(iter));

    auto parsedBoardList = json::parse(boardList);

    for(int i = 0; i < parsedBoardList.size(); i++)
    {
        eosio_assert(isValidPrecision(parsedBoardList[i]["minimumScore"], parsedBoardList[i]["precision"]), "invalid precision of minimum score");
        eosio_assert(isValidPrecision(parsedBoardList[i]["maximumScore"], parsedBoardList[i]["precision"]), "invalid precision of maximum score");

        boards.emplace(_self, [&](auto &b) {
            b.id = parsedBoardList[i]["id"];
            b.name = parsedBoardList[i]["name"];
            b.precision = parsedBoardList[i]["precision"];
            b.minimumScore = parsedBoardList[i]["minimumScore"];
            b.maximumScore = parsedBoardList[i]["maximumScore"];
        });
    }
}

ACTION itamstoreapp::score(uint64_t appId, uint64_t boardId, string score, name user, string data)
{
    require_auth(user);
    assertIfBlockUser(user, appId);

    leaderboardTable boards(_self, appId);
    const auto& board = boards.get(boardId, "invalid board id");

    eosio_assert(isValidPrecision(score, board.precision), "invalid precision of score");

    asset scoreOfAdd;
    stringToAsset(scoreOfAdd, score, board.precision);

    asset minimumScore;
    stringToAsset(minimumScore, board.minimumScore, board.precision);
    eosio_assert(scoreOfAdd >= minimumScore, "score must be lower than minimum score");

    asset maximumScore;
    stringToAsset(maximumScore, board.maximumScore, board.precision);
    eosio_assert(scoreOfAdd <= maximumScore, "score must be bigger than maximum score");
}

ACTION itamstoreapp::regachieve(uint64_t appId, name owner, string achievementList)
{
    require_auth(_self);
    eosio_assert(is_account(owner), "Invalid owner");

    achievementTable achievements(_self, appId);
    for(auto iter = achievements.begin(); iter != achievements.end(); iter = achievements.erase(iter));

    auto parsedAchievements = json::parse(achievementList);

    for(int i = 0; i < parsedAchievements.size(); i++)
    {
        achievements.emplace(_self, [&](auto &a) {
            a.id = parsedAchievements[i]["id"];
            a.name = parsedAchievements[i]["name"];
        });
    }
}

ACTION itamstoreapp::acquisition(uint64_t appId, uint64_t achieveId, name user, string data)
{
    require_auth(user);
    assertIfBlockUser(user, appId);

    achievementTable achievements(_self, appId);
    achievements.get(achieveId, "invalid achieve id");
}

ACTION itamstoreapp::cnlachieve(uint64_t appId, uint64_t achieveId, name user, string reason)
{
    require_auth(user);
    assertIfBlockUser(user, appId);

    achievementTable achievements(_self, appId);
    achievements.get(achieveId, "invalid achieve id");
}

ACTION itamstoreapp::blockuser(uint64_t appId, name user, string reason)
{
    require_auth(_self);

    blockTable blocks(_self, appId);
    const auto& block = blocks.find(user.value);

    eosio_assert(block == blocks.end(), "Already block user");

    blocks.emplace(_self, [&](auto &b) {
        b.user = user;
        b.timestamp = now();
        b.reason = reason;
    });
}

ACTION itamstoreapp::unblockuser(uint64_t appId, name user)
{
    require_auth(_self);

    blockTable blocks(_self, appId);
    const auto& block = blocks.require_find(user.value, "Already unblock");

    blocks.erase(block);
}

ACTION itamstoreapp::transfer(uint64_t from, uint64_t to)
{
    transferData data = unpack_action_data<transferData>();
    if (data.from == _self || data.to != _self) return;

    auto params = json::parse(data.memo);
    uint64_t appId = params["appId"];
    string category = params["category"];

    if(category == "app")
    {
        transferApp(appId, data, params);
    }
    else if(category == "item")
    {
        transferItem(appId, data, params);
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
        make_tuple(appId)
    );

    const auto& config = configs.get(_self.value, "refundable day must be set first");
    

    tx.delay_sec = config.refundableDay * SECONDS_OF_DAY;
    tx.send(now(), _self);
}

template <typename T>
void itamstoreapp::transferApp(uint64_t appId, transferData& data, T params)
{
    appTable apps(_self, _self.value);
    const auto& app = apps.get(appId, "Invalid app id");

    eosio_assert(data.quantity == app.amount, "Invalid purchase value");

    action(
        permission_level{_self, name("active")},
        _self, 
        name("receiptapp"),
        make_tuple(appId, data.from, data.quantity)
    ).send();

    setPendingTable(appId, NULL, data.from, data.quantity);
}

template <typename T>
void itamstoreapp::transferItem(uint64_t appId, transferData& data, T params)
{
    uint64_t itemId = params["itemId"];
    itemTable items(_self, appId);
    const auto& item = items.get(itemId, "Invalid item id");

    string pushToken = params["token"];
    asset amount = item.eos.amount > 0 ? item.eos : item.itam;

    eosio_assert(data.quantity == amount, "Invalid purchase value");

    action(
        permission_level{_self, name("active")},
        _self, 
        name("receiptitem"),
        make_tuple(appId, itemId, item.itemName, pushToken, data.from, data.quantity)
    ).send();

    setPendingTable(appId, itemId, data.from, data.quantity);
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

void itamstoreapp::assertIfBlockUser(name user, uint64_t appId)
{
    blockTable blocks(_self, appId);
    const auto& block = blocks.find(user.value);
    eosio_assert(block == blocks.end(), "block user");
}

bool itamstoreapp::isValidPrecision(string number, uint64_t precision)
{
    vector<string> score;

    split(score, number, ".");

    // score.size == 1 : decimal point size is 0
    // score[1].size() <= precision : check decimal point of precision
    return score.size() == 1 || score[1].size() <= precision;
}

void itamstoreapp::stringToAsset(asset &result, string number, uint64_t precision)
{
    vector<string> amount;
    split(amount, number, ".");

    uint64_t decimalPointSize = amount.size() == 1 ? 0 : amount[1].size();

    result.symbol = symbol("", precision);
    result.amount = stoull(amount[0] + amount[1], 0, 10) * pow(10, precision - decimalPointSize);
}