#include <eosiolib/eosio.hpp>
#include <string>

using namespace eosio;
using namespace std;

const char* ITAM_GROUP_ACCOUNT = "itamitamitam";

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