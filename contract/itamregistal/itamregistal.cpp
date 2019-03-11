#include "itamregistal.hpp"

ACTION itamregistal::registboard(uint64_t appId, string boardList)
{
    require_auth(_self);

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

ACTION itamregistal::score(uint64_t appId, uint64_t boardId, string score, string owner, name ownerGroup, string nickname, string data)
{
    require_auth(_self);
    eosio_assert(is_account(ownerGroup), "ownerGroup is not account");
    assertIfBlockUser(appId, owner, ownerGroup);

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

ACTION itamregistal::rank(uint64_t appId, uint64_t boardId, string ranks, string period)
{
    require_auth(_self);
    leaderboardTable(_self, appId).get(boardId, "invalid board id");
}

ACTION itamregistal::regachieve(uint64_t appId, string achievementList)
{
    require_auth(_self);

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

ACTION itamregistal::acquisition(uint64_t appId, uint64_t achieveId, string owner, name ownerGroup, string data)
{
    require_auth(_self);
    eosio_assert(is_account(ownerGroup), "ownerGroup is not account");
    assertIfBlockUser(appId, owner, ownerGroup);

    achievementTable achievements(_self, appId);
    achievements.get(achieveId, "invalid achieve id");
}

ACTION itamregistal::cnlachieve(uint64_t appId, uint64_t achieveId, string owner, name ownerGroup, string reason)
{
    require_auth(_self);
    eosio_assert(is_account(ownerGroup), "ownerGroup is not account");
    assertIfBlockUser(appId, owner, ownerGroup);
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");

    achievementTable achievements(_self, appId);
    achievements.get(achieveId, "invalid achieve id");
}

ACTION itamregistal::blockuser(uint64_t appId, string owner, name ownerGroup, string reason)
{
    require_auth(_self);
    eosio_assert(is_account(ownerGroup), "ownerGroup is not account");
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");

    blockTable blocks(_self, appId);
    const auto& block = blocks.find(ownerGroup.value);

    if(block == blocks.end())
    {
        blocks.emplace(_self, [&](auto &b) {
            b.ownerGroup = ownerGroup;
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

ACTION itamregistal::unblockuser(uint64_t appId, string owner, name ownerGroup, string reason)
{
    require_auth(_self);
    eosio_assert(is_account(ownerGroup), "ownerGroup is not account");
    eosio_assert(reason.size() <= 256, "reason has more than 256 bytes");

    blockTable blocks(_self, appId);
    const auto& block = blocks.require_find(ownerGroup.value, "Already unblock");

    eosio_assert(block->owners.count(owner) > 0, "Already unblock");
    blocks.modify(block, _self, [&](auto &b) {
        b.owners.erase(owner);
    });

    if(block->owners.size() == 0)
    {
        blocks.erase(block);
    }
}

ACTION itamregistal::delservice(uint64_t appId)
{
    require_auth(_self);

    leaderboardTable boards(_self, appId);
    for(auto iter = boards.begin(); iter != boards.end(); iter = boards.erase(iter));

    achievementTable achievements(_self, appId);
    for(auto iter = achievements.begin(); iter != achievements.end(); iter = achievements.erase(iter));
}

void itamregistal::assertIfBlockUser(uint64_t appId, const string& owner, name ownerGroup)
{
    blockTable blocks(_self, appId);
    const auto& block = blocks.find(ownerGroup.value);
    eosio_assert(block == blocks.end(), "block user");
}

bool itamregistal::isValidPrecision(const string& number, uint64_t precision)
{
    vector<string> score;

    split(score, number, ".");

    // score.size == 1 : decimal point size is 0
    // score[1].size() <= precision : check decimal point of precision
    return score.size() == 1 || score[1].size() <= precision;
}

void itamregistal::stringToAsset(asset &result, const string& number, uint64_t precision)
{
    vector<string> amount;
    split(amount, number, ".");

    uint64_t decimalPointSize = amount.size() == 1 ? 0 : amount[1].size();

    result.symbol = symbol("", precision);
    result.amount = stoull(amount[0] + amount[1], 0, 10) * pow(10, precision - decimalPointSize);
}