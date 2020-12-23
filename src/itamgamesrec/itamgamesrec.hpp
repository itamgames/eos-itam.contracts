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
        ACTION digitalasset(uint64_t appId, uint64_t digitalAssetId, string name, string description, string image);
        ACTION createnft(string uuid, uint64_t appId, uint64_t digitalAssetId, string gameAccount, string json, string nonce, uint64_t timestamp);
        ACTION modifynft(string uuid, uint64_t appId, uint64_t digitalAssetId, string gameAccount, string json, string nonce, uint64_t timestamp);
        ACTION deletenft(string uuid, uint64_t appId, uint64_t digitalAssetId, uint64_t nftId, string gameAccount, string json, string nonce, uint64_t timestamp);
        ACTION createorder(string uuid, uint64_t appId, uint64_t digitalAssetId, string gameAccount, string price, string nonce, uint64_t timestamp);
        ACTION modifyorder(string uuid, uint64_t appId, uint64_t digitalAssetId, string gameAccount, string price, string nonce, uint64_t timestamp);
        ACTION deleteorder(string uuid, uint64_t appId, uint64_t digitalAssetId, string gameAccount, string price, string nonce, uint64_t timestamp);
        ACTION order(string uuid, uint64_t appId, uint64_t digitalAssetId, string from, string to, string price, string nonce, uint64_t timestamp);
};

ALLOW_TRANSFER_ALL_DISPATCHER( itamgamesrec, (achievement)(leaderboard)(gamehistory)(createnft)(modifynft)(deletenft)(createorder)(modifyorder)(deleteorder)(order) )