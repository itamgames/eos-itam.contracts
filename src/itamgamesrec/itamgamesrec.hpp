#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include "../include/dispatcher.hpp"

using namespace eosio;
using namespace std;

CONTRACT itamgamesrec : contract
{
    public:
        using contract::contract;

        ACTION achievement(uint64_t appId, uint64_t achievementId, string gameAccount, string data, uint64_t nonce);
        ACTION leaderboard(uint64_t appId, uint64_t leaderboardId, string gameAccount, string json, uint64_t nonce);
        ACTION history(uint64_t appId, string gameAccount, string json, uint64_t nonce);
        ACTION nft(uint64_t appId, uint64_t nftId, string gameAccount, string json, string state, uint64_t nonce);
        ACTION createorder(uint64_t appId, uint64_t nftId, string gameAccount, string price, uint64_t nonce);
        ACTION modifyorder(uint64_t appId, uint64_t nftId, string price, uint64_t nonce);
        ACTION deleteorder(uint64_t appId, uint64_t nftId, string gameAccount, uint64_t nonce);
        ACTION order(uint64_t appId, uint64_t nftId, string from, string to, uint64_t nonce);
};

ALLOW_TRANSFER_ALL_DISPATCHER( itamgamesrec, (achievement)(leaderboard)(history)(nft)(createorder)(modifyorder)(deleteorder)(order) )