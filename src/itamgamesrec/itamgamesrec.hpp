#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include "../include/dispatcher.hpp"

using namespace eosio;
using namespace std;

CONTRACT itamgamesrec : contract
{
    public:
        using contract::contract;

        ACTION achievement(uint64_t appId, uint64_t achievementId, string gameAccount, string data, string nonce, uint64_t timestamp);
        ACTION leaderboard(uint64_t appId, uint64_t leaderboardId, string gameAccount, string json, string nonce, uint64_t timestamp);
        ACTION gamehistory(uint64_t appId, string gameAccount, string json, string nonce, uint64_t timestamp);
        ACTION createnft(uint64_t appId, uint64_t nftId, string gameAccount, string json, string nonce, uint64_t timestamp);
        ACTION modifynft(uint64_t appId, uint64_t nftId, string gameAccount, string json, string nonce, uint64_t timestamp);
        ACTION deletenft(uint64_t appId, uint64_t nftId, string gameAccount, string json, string nonce, uint64_t timestamp);
        ACTION createorder(uint64_t appId, uint64_t nftId, string gameAccount, string price, string nonce, uint64_t timestamp);
        ACTION modifyorder(uint64_t appId, uint64_t nftId, string gameAccount, string price, string nonce, uint64_t timestamp);
        ACTION deleteorder(uint64_t appId, uint64_t nftId, string gameAccount, string price, string nonce, uint64_t timestamp);
        ACTION order(uint64_t appId, uint64_t nftId, string from, string to, string price, string nonce, uint64_t timestamp);
};

ALLOW_TRANSFER_ALL_DISPATCHER( itamgamesrec, (achievement)(leaderboard)(gamehistory)(createnft)(modifynft)(deletenft)(createorder)(modifyorder)(deleteorder)(order) )