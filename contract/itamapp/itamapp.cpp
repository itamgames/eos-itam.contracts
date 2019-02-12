#include "itamapp.hpp"

ACTION itamapp::registitems(string params)
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

ACTION itamapp::modifyitem(uint64_t appId, uint64_t itemId, string itemName, asset eos, asset itam)
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

ACTION itamapp::deleteitems(string params)
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

ACTION itamapp::refunditem(uint64_t appId, uint64_t itemId, name buyer)
{
    require_auth(_self);

    itemTable items(_self, appId);
    const auto& item = items.get(itemId, "Invalid item id");

    pendingTable pendings(_code, appId);
    const auto& pending = pendings.require_find(buyer.value, "Can't find refundable customer");

    configTable configs(_self, _self.value);
    const auto& config = configs.get(_self.value, "Can't find refundable day");

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

ACTION itamapp::registapp(uint64_t appId, name owner, asset amount, string params)
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

ACTION itamapp::deleteapp(uint64_t appId)
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

ACTION itamapp::refundapp(uint64_t appId, name buyer)
{
    require_auth(_self);

    appTable apps(_self, _self.value);
    const auto& app = apps.get(appId, "Invalid App Id");

    pendingTable pendings(_self, appId);
    const auto& pending = pendings.require_find(buyer.value, "Can't find refundable customer");

    configTable configs(_self, _self.value);
    const auto& config = configs.get(_self.value, "Can't find refundable day");

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

ACTION itamapp::setsettle(uint64_t appId, name account)
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

ACTION itamapp::claimsettle(uint64_t appId)
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

ACTION itamapp::defconfirm(uint64_t appId)
{
    confirm(appId);
}

ACTION itamapp::menconfirm(uint64_t appId)
{
    confirm(appId);
}

void itamapp::confirm(uint64_t appId)
{
    require_auth(_self);

    uint64_t currentTimestamp = now();

    configTable configs(_self, _self.value);
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

ACTION itamapp::setconfig(uint64_t ratio, uint64_t refundableDay)
{
    require_auth(_self);

    configTable configs(_self, _self.value);
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

ACTIONM itamapp::registboard(uint64_t appId, name owner, string boardList)
{
    require_auth(_self);
    eosio_assert(is_account(owner), "Invalid owner");

    leaderboardTable boards(_self, appId);
    for(auto iter = boards.begin(); iter != boards.end(); iter = boards.erase(iter));

    auto& boardList = json::parse(boardList);

    for(const auto& board : boardList)
    {
        eosio_assert(isValidPrecision(board->minimumScore, board->precision), "invalid precision of minimum score");
        eosio_assert(isValidPrecision(board->maximumScore, board->precision), "invalid precision of maximum score");

        boards.emplace(_self, [&](auto &b) {
            b.id = board->id;
            b.name = board->name;
            b.precision = board->precision;
            b.minimumScore = board->minimumScore;
            b.maximumScore = board->maximumScore;
        });
    }
}

ACTION itamapp::score(uint64_t boardId, uint64_t appId, string score, name user, string data)
{
    require_auth(user);
    assertIfBlockUser(user);

    leaderboardTable boards(_self, appId);
    const auto& board = boards.get(boardId, "invalid board id");

    eosio_assert(isValidPrecision(score, board.precision), "invalid precision of score");

    asset scoreOfAdd;
    stringToAsset(score, scoreOfAdd, board.precision);

    asset minimumScore;
    stringToAsset(minimumScore, board.minimumScore, board.precision);
    eosio_assert(scoreOfAdd >= minimumScore, "score must be lower than minimum score");

    asset maximumScore;
    stringToAsset(maximumScore, board.maximumScore, board.precision);
    eosio_assert(scoreOfAdd >= maximumScore, "score must be bigger than maximum score");
}


ACTION itamapp::transfer(uint64_t from, uint64_t to)
{
    transferData data = unpack_action_data<transferData>();
    if (data.from == _self || data.to != _self) return;

    auto params = json::parse(data.memo);
    uint64_t appId = params["appId"];
    string category = params["category"];
    
    uint64_t itemId = NULL;
    asset amount;
    if(category == "app")
    {
        appTable apps(_self, _self.value);
        const auto& app = apps.get(appId, "Invalid app id");
        amount = app.amount;

        action(
            permission_level{_self, name("active")},
            _self, 
            name("receiptapp"),
            make_tuple(appId, data.from, data.quantity)
        ).send();
    }
    else if(category == "item")
    {
        itemId = params["itemId"];
        itemTable items(_self, appId);
        const auto &item = items.get(itemId, "Invalid item id");

        string itemName = params["itemName"];
        eosio_assert(item.itemName == itemName, "Wrong item name");

        string packageName = params["packageName"];
        string pushToken = params["token"];
        amount = item.eos.amount > 0 ? item.eos : item.itam;

        action(
            permission_level{_self, name("active")},
            _self, 
            name("receiptitem"),
            make_tuple(appId, itemId, itemName, packageName, pushToken, data.from, data.quantity)
        ).send();
    }
    else
    {
        eosio_assert(false, "Invalid category");
    }

    eosio_assert(data.quantity == amount, "Invalid purchase value");
    
    configTable configs(_self, _self.value);
    const auto& config = configs.get(_self.value, "Can't find refundable day");

    pendingTable pendings(_self, appId);
    const auto& pending = pendings.find(data.from.value);

    pendingInfo info{appId, itemId, data.quantity * config.ratio / 100, now()};
    if(pending == pendings.end())
    {
        pendings.emplace(_self, [&](auto &p) {
            p.buyer = data.from;
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
        make_tuple(appId)
    );

    tx.delay_sec = config.refundableDay * SECONDS_OF_DAY;
    tx.send(now(), _self);
}

ACTION itamapp::receiptapp(uint64_t appId, name from, asset quantity)
{
    require_auth(_self);
    require_recipient(_self, from);
}

ACTION itamapp::receiptitem(uint64_t appId, uint64_t itemId, string itemName, string packageName, string token, name from, asset quantity)
{
    require_auth(_self);
    require_recipient(_self, from);
}

void itamapp::assertIfBlockUser(name user)
{
    blockTable blocks(_self, appId);
    const auto& block = blocks.find(user.value);
    eosio_assert(block == blocks.end(), "block user");
}

bool itamapp::isValidPrecision(string number, uint64_t precision)
{
    vector<string> score;

    split(score, number, ".");

    // score.size == 1 : decimal point size is 0
    // score[1].size() <= precision : check decimal point of precision
    return score.size == 1 || score[1].size() <= precision;
}

void itamapp::stringToAsset(asset &result, string number, uint64_t precision)
{
    vector<string> amount;
    split(amount, number, ".");

    uint64_t decimalPointSize = amount.size() == 1 ? 0 : amount[1].size();

    result.symbol = symbol("", precision);
    result.amount = stoull(amount[0] + amount[1], 0, 10) * pow(10, precision - decimalPointSize);
}