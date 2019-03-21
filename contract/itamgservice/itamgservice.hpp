#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include "../include/json.hpp"
#include "../include/string.hpp"
#include "../include/dispatcher.hpp"
#include "../include/ownergroup.hpp"

using namespace eosio;
using namespace std;
using namespace nlohmann;

CONTRACT itamgservice : public contract
{
    public:
        using contract::contract;

        ACTION history(uint64_t appId, string owner, name ownerGroup, string data);
        ACTION delservice(uint64_t appId);

        // leader board
        ACTION registboard(uint64_t appId, string boardList);
        ACTION score(uint64_t appId, uint64_t boardId, string score, string owner, name ownerGroup, string nickname, string data);
        ACTION rank(uint64_t appId, uint64_t boardId, string ranks, string period);

        // achievements
        ACTION regachieve(uint64_t appId, string achievementList);
        ACTION acquisition(uint64_t appId, uint64_t achieveId, string owner, name ownerGroup, string data);
        ACTION cnlachieve(uint64_t appId, uint64_t achieveId, string owner, name ownerGroup, string reason);

        // block
        ACTION blockuser(uint64_t appId, string owner, name ownerGroup, string reason);
        ACTION unblockuser(uint64_t appId, string owner, name ownerGroup, string reason);
    private:
        // leader board
        TABLE leaderboard
        {
            uint64_t id;
            string name;
            uint64_t precision;
            string minimumScore;
            string maximumScore;

            uint64_t primary_key() const { return id; }
        };
        typedef multi_index<name("leaderboards"), leaderboard> leaderboardTable;

        TABLE achievement
        {
            uint64_t id;
            string name;

            uint64_t primary_key() const { return id; }
        };
        typedef multi_index<name("achievements"), achievement> achievementTable;

        struct blockInfo
        {
            uint64_t timestamp;
            string reason;
        };

        TABLE block
        {
            name ownerGroup;
            map<string, blockInfo> owners;

            uint64_t primary_key() const { return ownerGroup.value; }
        };
        typedef multi_index<name("blocks"), block> blockTable;

    private:
        void assertIfBlockUser(uint64_t appId, const string& owner, name groupAccount);
        void stringToAsset(asset &result, const string& number, uint64_t precision);
        bool isValidPrecision(const string& number, uint64_t precision);
};

EOSIO_DISPATCH_EX( itamgservice, (registboard)(score)(rank)(regachieve)(acquisition)(cnlachieve)(blockuser)(unblockuser)(delservice)(history) )