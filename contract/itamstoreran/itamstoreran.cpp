#include "itamstoreran.hpp"

ACTION itamstoreran::registboard(string appId, string boardList)
{
    require_auth(_self);
    auto parsedBoardList = json::parse(boardList);

    uint64_t appid = stoull(appId, 0, 10);
    uint64_t leader_board_id = 0;
    leaderboardTable leaderBoards(_self, appid);
    
    #ifdef BETA
        for(auto iter = leaderBoards.begin(); iter != leaderBoards.end(); iter = leaderBoards.erase(iter));
    #endif

    for(int i = 0; i < parsedBoardList.size(); i++)
    {
        leader_board_id = stoull(string(parsedBoardList[i]["id"]), 0, 10);
        const auto& leaderBoard = leaderBoards.find(leader_board_id);
        if(leaderBoard == leaderBoards.end())
        {
            leaderBoards.emplace(_self, [&](auto& b) {
                b.id = leader_board_id;
                b.name = parsedBoardList[i]["name"];
            });
        }
        else
        {
            leaderBoards.modify(leaderBoard, _self, [&](auto& b) {
                b.name = parsedBoardList[i]["name"];
            });
        }
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
    auto parsedAchievements = json::parse(achievementList);
    
    uint64_t appid = stoull(appId, 0, 10);
    uint64_t achievement_id = 0;
    achievementTable achievements(_self, appid);

    #ifdef BETA
        for(auto iter = achievements.begin(); iter != achievements.end(); iter = achievements.erase(iter));
    #endif

    for(int i = 0; i < parsedAchievements.size(); i++)
    {
        achievement_id = stoull(string(parsedAchievements[i]["id"]), 0, 10);
        const auto& achievement = achievements.find(achievement_id);
        if(achievement == achievements.end())
        {
            achievements.emplace(_self, [&](auto& a) {
                a.id = stoull(string(parsedAchievements[i]["id"]), 0, 10);
                a.name = parsedAchievements[i]["name"];
            });
        }
        else
        {
            achievements.modify(achievement, _self, [&](auto& a) {
                a.name = parsedAchievements[i]["name"];
            });
        }
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