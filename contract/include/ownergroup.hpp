#include <eosiolib/eosio.hpp>

using namespace eosio;

const char* GROUP_CONTRACT_NAME = "itamgamestle";

struct ownergroup
{
    name groupName;
    name account;

    uint64_t primary_key() const { return groupName.value; }
};
typedef multi_index<name("ownergroups"), ownergroup> ownerGroupTable;

name getGroupAccount(const std::string& owner, name groupName)
{
    if(groupName.to_string() == "eos")
    {
        return name(owner);
    }

    name commonContract("itamgamestle");

    ownerGroupTable ownerGroups(commonContract, commonContract.value);
    return ownerGroups.get(groupName.value, "groupName does not exist").account;
}