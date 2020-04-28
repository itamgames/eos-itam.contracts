#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include "../include/dispatcher.hpp"

using namespace eosio;
using namespace std;

CONTRACT itamgamesacc : contract
{
    public:
        using contract::contract;

        ACTION achievement(uint64_t appId, uint64_t achievementId, string user, string data, uint64_t timestamp);
        ACTION leaderboard(uint64_t appId, uint64_t leaderboardId, string user, string data, uint64_t timestamp);
        ACTION history(uint64_t appId, string user, string data, uint64_t timestamp);
        ACTION nft(uint64_t appId, uint64_t nftId, string data, string state, uint64_t timestamp);
        ACTION createorder(uint64_t appId, uint64_t nftId, string user, string price, uint64_t timestamp);
        ACTION modifyorder(uint64_t appId, uint64_t nftId, string price, uint64_t timestamp);
        ACTION deleteorder(uint64_t appId, uint64_t nftId, string user, uint64_t timestamp);
        ACTION order(uint64_t appId, uint64_t nftId, string from, string to, uint64_t timestamp);
};

ALLOW_TRANSFER_ALL_DISPATCHER( itamgamesacc, (achievement)(leaderboard)(history)(nft)(createorder)(modifyorder)(deleteorder)(order) )