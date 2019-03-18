#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include "../include/json.hpp"
#include "../include/common.hpp"
#include "../include/dispatcher.hpp"

using namespace eosio;
using namespace std;
using namespace nlohmann;

CONTRACT itamregistal : public contract {
    public:
        itamregistal(name receiver, name code, datastream<const char*> ds) : contract(receiver, code, ds) {}

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

        struct ownergroup
        {
            name owner;
            name account;

            uint64_t primary_key() const { return owner.value; }
        };
        typedef multi_index<name("ownergroups"), ownergroup> ownergroupTable;
    private:
        void assertIfBlockUser(uint64_t appId, const string& owner, name groupAccount);
        void stringToAsset(asset &result, const string& number, uint64_t precision);
        bool isValidPrecision(const string& number, uint64_t precision);
        name getGroupAccount(const string& owner, name ownerGroup);
};

EOSIO_DISPATCH_EX( itamregistal, (registboard)(score)(rank)(regachieve)(acquisition)(cnlachieve)(blockuser)(unblockuser)(delservice) )