#include "itamgservice.hpp"

ACTION itamgservice::registboard(string appId, string boardList)
{
    require_auth(_self);
    uint64_t appid = stoull(appId, 0, 10);

    leaderboardTable boards(_self, appid);
    for(auto iter = boards.begin(); iter != boards.end(); iter = boards.erase(iter));

    auto parsedBoardList = json::parse(boardList);

    for(int i = 0; i < parsedBoardList.size(); i++)
    {
        eosio_assert(isValidPrecision(parsedBoardList[i]["minimumScore"], parsedBoardList[i]["precision"]), "invalid precision of minimum score");
        eosio_assert(isValidPrecision(parsedBoardList[i]["maximumScore"], parsedBoardList[i]["precision"]), "invalid precision of maximum score");

        boards.emplace(_self, [&](auto &b) {
            b.id = stoull(string(parsedBoardList[i]["id"]), 0, 10);
            b.name = parsedBoardList[i]["name"];
            b.precision = parsedBoardList[i]["precision"];
            b.minimumScore = parsedBoardList[i]["minimumScore"];
            b.maximumScore = parsedBoardList[i]["maximumScore"];
        });
    }
}

ACTION itamgservice::score(string appId, string boardId, string score, string owner, name ownerGroup, string nickname, string data)
{
    uint64_t appid = stoull(appId, 0, 10);
    uint64_t boardid = stoull(boardId, 0, 10);
    require_auth(_self);
    name groupAccount = get_group_account(owner, ownerGroup);
    eosio_assert(is_account(groupAccount), "ownerGroup is not valid");
    assertIfBlockUser(appid, owner, groupAccount);

    leaderboardTable boards(_self, appid);
    const auto& board = boards.get(boardid, "invalid board id");

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

ACTION itamgservice::rank(string appId, string boardId, string ranks, string period)
{
    require_auth(_self);
    uint64_t appid = stoull(appId, 0, 10);
    uint64_t boardid = stoull(boardId, 0, 10);
    leaderboardTable(_self, appid).get(boardid, "invalid board id");
}

ACTION itamgservice::regachieve(string appId, string achievementList)
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

ACTION itamgservice::acquisition(string appId, string achieveId, string owner, name ownerGroup, string data)
{
    require_auth(_self);
    
    uint64_t appid = stoull(appId, 0, 10);
    uint64_t achieveid = stoull(achieveId, 0, 10);

    name groupAccount = get_group_account(owner, ownerGroup);
    eosio_assert(is_account(groupAccount), "ownerGroup is not valid");
    assertIfBlockUser(appid, owner, groupAccount);

    achievementTable achievements(_self, appid);
    achievements.get(achieveid, "invalid achieve id");
}

ACTION itamgservice::cnlachieve(string appId, string achieveId, string owner, name ownerGroup, string reason)
{
    require_auth(_self);
    uint64_t appid = stoull(appId, 0, 10);
    uint64_t achieveid = stoull(achieveId, 0, 10);

    name groupAccount = get_group_account(owner, ownerGroup);
    eosio_assert(is_account(ownerGroup), "ownerGroup is not valid");
    assertIfBlockUser(appid, owner, groupAccount);
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");

    achievementTable achievements(_self, appid);
    achievements.get(achieveid, "invalid achieve id");
}

ACTION itamgservice::blockuser(string appId, string owner, name ownerGroup, string reason)
{
    require_auth(_self);
    uint64_t appid = stoull(appId, 0, 10);
    name groupAccount = get_group_account(owner, ownerGroup);
    eosio_assert(is_account(groupAccount), "ownerGroup is not valid");
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");

    blockTable blocks(_self, appid);
    const auto& block = blocks.find(groupAccount.value);

    if(block == blocks.end())
    {
        blocks.emplace(_self, [&](auto &b) {
            b.ownerGroup = groupAccount;
            b.owners[owner] = blockInfo { now(), reason };
        });
    }
    else
    {
        eosio_assert(block->owners.count(owner) == 0, "Already block user");
        blocks.modify(block, _self, [&](auto &b) {
            b.owners[owner] = blockInfo { now(), reason };
        });
    }
}

ACTION itamgservice::unblockuser(string appId, string owner, name ownerGroup, string reason)
{
    require_auth(_self);
    uint64_t appid = stoull(appId, 0, 10);
    name groupAccount = get_group_account(owner, ownerGroup);    
    eosio_assert(is_account(groupAccount), "ownerGroup is not valid");
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");

    blockTable blocks(_self, appid);
    const auto& block = blocks.require_find(groupAccount.value, "Already unblock");

    eosio_assert(block->owners.count(owner) > 0, "Already unblock");
    blocks.modify(block, _self, [&](auto &b) {
        b.owners.erase(owner);
    });

    if(block->owners.size() == 0)
    {
        blocks.erase(block);
    }
}

ACTION itamgservice::delservice(string appId)
{
    require_auth(_self);

    uint64_t appid = stoull(appId, 0, 10);

    leaderboardTable boards(_self, appid);
    for(auto iter = boards.begin(); iter != boards.end(); iter = boards.erase(iter));

    achievementTable achievements(_self, appid);
    for(auto iter = achievements.begin(); iter != achievements.end(); iter = achievements.erase(iter));
}

ACTION itamgservice::history(string appId, string owner, name ownerGroup, string data)
{
    require_auth(_self);
    uint64_t appid = stoull(appId, 0, 10);
    name groupAccount = get_group_account(owner, ownerGroup);
    require_recipient(groupAccount);
}

void itamgservice::assertIfBlockUser(uint64_t appId, const string& owner, name groupAccount)
{
    blockTable blocks(_self, appId);
    const auto& block = blocks.find(groupAccount.value);
    if(block == blocks.end())
    {
        return;
    }

    eosio_assert(block->owners.count(owner) == 0, "block user");
}

bool itamgservice::isValidPrecision(const string& number, uint64_t precision)
{
    vector<string> score;

    split(score, number, ".");

    // score.size == 1 : decimal point size is 0
    // score[1].size() <= precision : check decimal point of precision
    return score.size() == 1 || score[1].size() <= precision;
}

void itamgservice::stringToAsset(asset &result, const string& number, uint64_t precision)
{
    vector<string> amount;
    split(amount, number, ".");

    uint64_t decimalPointSize = amount.size() == 1 ? 0 : amount[1].size();

    result.symbol = symbol("", precision);
    result.amount = stoull(amount[0] + amount[1], 0, 10) * pow(10, precision - decimalPointSize);
}