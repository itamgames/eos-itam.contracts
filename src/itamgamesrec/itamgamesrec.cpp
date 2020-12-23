#include "itamgamesrec.hpp"

ACTION itamgamesrec::achievement(uint64_t appId, uint64_t achievementId, string gameAccount, string json, string nonce, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::leaderboard(uint64_t appId, uint64_t leaderboardId, string gameAccount, string json, string nonce, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::gamehistory(uint64_t appId, string gameAccount, string json, string nonce, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::digitalasset(uint64_t appId, uint64_t digitalAssetId, string name, string description, string image)
{
    require_auth(_self);
}

ACTION itamgamesrec::createnft(string uuid, uint64_t appId, uint64_t digitalAssetId, string gameAccount, string json, string nonce, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::modifynft(string uuid, uint64_t appId, uint64_t digitalAssetId, string gameAccount, string json, string nonce, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::deletenft(string uuid, uint64_t appId, uint64_t digitalAssetId, uint64_t nftId, string gameAccount, string json, string nonce, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::createorder(string uuid, uint64_t appId, uint64_t digitalAssetId, string gameAccount, string price, string nonce, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::modifyorder(string uuid, uint64_t appId, uint64_t digitalAssetId, string gameAccount, string price, string nonce, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::deleteorder(string uuid, uint64_t appId, uint64_t digitalAssetId, string gameAccount, string price, string nonce, uint64_t timestamp)
{
    require_auth(_self);
}

ACTION itamgamesrec::order(string uuid, uint64_t appId, uint64_t digitalAssetId, string from, string to, string price, string nonce, uint64_t timestamp)
{
    require_auth(_self);
}