#include "itamstoreran.hpp"

ACTION itamstoreran::registboard(string appId, string boardList)
{
    require_auth(_self);
    uint64_t appid = stoull(appId, 0, 10);

    leaderboardTable boards(_self, appid);
    for(auto iter = boards.begin(); iter != boards.end(); iter = boards.erase(iter));

    auto parsedBoardList = json::parse(boardList);

    for(int i = 0; i < parsedBoardList.size(); i++)
    {
        boards.emplace(_self, [&](auto &b) {
            b.id = stoull(string(parsedBoardList[i]["id"]), 0, 10);
            b.name = parsedBoardList[i]["name"];
        });
    }
}

ACTION itamstoreran::score(string appId, string boardId, string score, name owner, name ownerGroup, string nickname, string data)
{
    require_auth(_self);
    
    uint64_t appid = stoull(appId, 0, 10);
    uint64_t boardid = stoull(boardId, 0, 10);

    name ownerAccount = get_group_account(owner, ownerGroup);
    eosio_assert(is_account(ownerAccount), "ownerAccount is not valid");

    leaderboardTable boards(_self, appid);
    const auto& board = boards.get(boardid, "invalid board id");
}

ACTION itamstoreran::rank(string appId, string boardId, string ranks, string period)
{
    require_auth(_self);

    uint64_t appid = stoull(appId, 0, 10);
    uint64_t boardid = stoull(boardId, 0, 10);
    leaderboardTable(_self, appid).get(boardid, "invalid board id");
}

ACTION itamstoreran::regachieve(string appId, string achievementList)
{
    require_auth(_self);

    uint64_t appid = stoull(appId, 0, 10);

    achievementTable achievements(_self, appid);
    for(auto iter = achievements.begin(); iter != achievements.end(); iter = achievements.erase(iter));

    auto parsedAchievements = json::parse(achievementList);

    for(int i = 0; i < parsedAchievements.size(); i++)
    {
        achievements.emplace(_self, [&](auto &a) {
            a.id = stoull(string(parsedAchievements[i]["id"]), 0, 10);
            a.name = parsedAchievements[i]["name"];
        });
    }
}

ACTION itamstoreran::acquisition(string appId, string achieveId, name owner, name ownerGroup, string data)
{
    require_auth(_self);
    
    uint64_t appid = stoull(appId, 0, 10);
    uint64_t achieveid = stoull(achieveId, 0, 10);

    name ownerAccount = get_group_account(owner, ownerGroup);
    eosio_assert(is_account(ownerAccount), "ownerAccount is not valid");

    achievementTable achievements(_self, appid);
    achievements.get(achieveid, "invalid achieve id");
}

ACTION itamstoreran::cnlachieve(string appId, string achieveId, name owner, name ownerGroup, string reason)
{
    require_auth(_self);
    uint64_t appid = stoull(appId, 0, 10);
    uint64_t achieveid = stoull(achieveId, 0, 10);

    name ownerAccount = get_group_account(owner, ownerGroup);
    eosio_assert(is_account(ownerAccount), "ownerAccount is not valid");
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");

    achievementTable achievements(_self, appid);
    achievements.get(achieveid, "invalid achieve id");
}

ACTION itamstoreran::delservice(string appId)
{
    require_auth(_self);

    uint64_t appid = stoull(appId, 0, 10);

    leaderboardTable boards(_self, appid);
    for(auto iter = boards.begin(); iter != boards.end(); iter = boards.erase(iter));

    achievementTable achievements(_self, appid);
    for(auto iter = achievements.begin(); iter != achievements.end(); iter = achievements.erase(iter));
}

ACTION itamstoreran::history(string appId, name owner, name ownerGroup, string data)
{
    require_auth(_self);
    name ownerAccount = get_group_account(owner, ownerGroup);
    require_recipient(ownerAccount);
}