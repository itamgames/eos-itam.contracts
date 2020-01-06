#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include "../include/json.hpp"
#include "../include/dispatcher.hpp"
#include "../include/ownergroup.hpp"

using namespace eosio;
using namespace std;
using namespace nlohmann;

CONTRACT itamstoreran : public contract
{
    public:
        using contract::contract;

        ACTION delservice(string appId);
        ACTION registboard(string appId, string boardList);
        ACTION score(string appId, string boardId, string score, name owner, name ownerGroup, string nickname, string data);
        ACTION rank(string appId, string boardId, string ranks, string period);
        ACTION regachieve(string appId, string achievementList);
        ACTION acquisition(string appId, string achieveId, name owner, name ownerGroup, string data);
        ACTION cnlachieve(string appId, string achieveId, name owner, name ownerGroup, string reason);
        ACTION history(string appId, name owner, name ownerGroup, string data);
    private:
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
};

EOSIO_DISPATCH_EX( itamstoreran, (registboard)(score)(rank)(regachieve)(acquisition)(cnlachieve)(delservice)(history) )