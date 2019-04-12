#include <eosiolib/eosio.hpp>

using namespace eosio;

const char* GROUP_CONTRACT_NAME = "itamgamestle";
const char* ITAM_GROUP_ACCOUNT = "itamitamitam";

struct ownergroup
{
    name groupName;
    name account;

    uint64_t primary_key() const { return groupName.value; }
};
typedef multi_index<name("ownergroups"), ownergroup> ownerGroupTable;

name get_group_account(string owner, name group_name)
{
    string group_name_to_string = group_name.to_string();
    
    if(group_name_to_string == "eos")
    {
        return name(owner);
    }

    eosio_assert(group_name_to_string == "itam", "invalid group name");
    return name(ITAM_GROUP_ACCOUNT);
}