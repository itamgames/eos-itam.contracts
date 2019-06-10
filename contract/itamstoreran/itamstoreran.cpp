#include "itamstoreran.hpp"

ACTION itamstoreran::registboard(string appId, string delimitedLeaderboards)
{
    require_auth(_self);

    const uint32_t leaderboardParamSize = 2;
    vector<string> parsedLeaderboards;
    split(parsedLeaderboards, delimitedLeaderboards, "|");
    eosio_assert(parsedLeaderboards.size() % leaderboardParamSize == 0, "invalid leaderboards");

    uint64_t appid = stoull(appId, 0, 10);
    leaderboardTable leaderboards(_self, appid);
    for(auto iter = leaderboards.begin(); iter != leaderboards.end(); iter = leaderboards.erase(iter));

    for(int i = 0; i < parsedLeaderboards.size(); i += leaderboardParamSize)
    {
        leaderboards.emplace(_self, [&](auto& b) {
            b.id = stoull(parsedLeaderboards[i], 0, 10);
            b.name = parsedLeaderboards[i + 1];
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

ACTION itamstoreran::regachieve(string appId, string delimitedAchievements)
{
    require_auth(_self);

    const uint32_t achievementParamSize = 2;
    vector<string> parsedAchievements;
    split(parsedAchievements, delimitedAchievements, "|");

    uint64_t appid = stoull(appId, 0, 10);

    achievementTable achievements(_self, appid);
    for(auto achievement = achievements.begin(); achievement != achievements.end(); achievement = achievements.erase(achievement));

    for(int i = 0; i < parsedAchievements.size(); i += achievementParamSize)
    {
        achievements.emplace(_self, [&](auto &a) {
            a.id = stoull(parsedAchievements[i], 0, 10);
            a.name = parsedAchievements[i + 1];
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