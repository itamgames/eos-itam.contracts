#include "itamgamesrec.hpp"

ACTION itamgamesrec::achievement(uint64_t appId, uint64_t achievementId, string user, string data, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::leaderboard(uint64_t appId, uint64_t leaderboardId, string user, string data, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::history(uint64_t appId, string user, string data, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::nft(uint64_t appId, uint64_t nftId, string data, string state, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::createorder(uint64_t appId, uint64_t nftId, string user, string price, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::modifyorder(uint64_t appId, uint64_t nftId, string price, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::deleteorder(uint64_t appId, uint64_t nftId, string user, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::order(uint64_t appId, uint64_t nftId, string from, string to, uint64_t timestamp)
{
    require_auth(_self);
}