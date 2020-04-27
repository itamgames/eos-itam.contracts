#include "itamgamesacc.hpp"

ACTION itamgamesacc::achievement(uint64_t appId, uint64_t achievementId, string user, string data, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesacc::leaderboard(uint64_t appId, uint64_t leaderboardId, string user, string data, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesacc::history(uint64_t appId, string user, string data, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesacc::nft(uint64_t appId, uint64_t nftId, string data, string state, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesacc::createorder(uint64_t appId, uint64_t nftId, string user, string price, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesacc::modifyorder(uint64_t appId, uint64_t nftId, string price, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesacc::deleteorder(uint64_t appId, uint64_t nftId, string user, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesacc::order(uint64_t appId, uint64_t nftId, string from, string to, uint64_t timestamp)
{
    require_auth(_self);
}