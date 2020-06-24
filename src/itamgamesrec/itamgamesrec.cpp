#include "itamgamesrec.hpp"

ACTION itamgamesrec::achievement(uint64_t appId, uint64_t achievementId, string gameAccount, string json, uint64_t nonce, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::leaderboard(uint64_t appId, uint64_t leaderboardId, string gameAccount, string json, uint64_t nonce, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::gamehistory(uint64_t appId, string gameAccount, string json, uint64_t nonce, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::createnft(uint64_t appId, uint64_t nftId, string gameAccount, string json, uint64_t nonce, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::modifynft(uint64_t appId, uint64_t nftId, string gameAccount, string json, uint64_t nonce, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::deletenft(uint64_t appId, uint64_t nftId, string gameAccount, string json, uint64_t nonce, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::createorder(uint64_t appId, uint64_t nftId, string gameAccount, string price, uint64_t nonce, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::modifyorder(uint64_t appId, uint64_t nftId, string gameAccount, string price, uint64_t nonce, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::deleteorder(uint64_t appId, uint64_t nftId, string gameAccount, uint64_t nonce, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::order(uint64_t appId, uint64_t nftId, string from, string to, string price, uint64_t nonce, uint64_t timestamp)
{
    require_auth(_self);
}