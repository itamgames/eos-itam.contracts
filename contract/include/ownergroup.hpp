#include <eosiolib/eosio.hpp>

using namespace eosio;

struct ownergroup
{
    name owner;
    name account;

    uint64_t primary_key() const { return owner.value; }
};

typedef multi_index<name("ownergroups"), ownergroup> ownergroup_table;

name get_group_account(const string& owner, name owner_group)
{
    name itamaccntmgr("itamaccntmgr");
    if(owner_group.to_string() == "eos")
    {
        return name(owner);
    }

    ownergroup_table ownergroups(itamaccntmgr, itamaccntmgr.value);
    const auto& ownergroup = ownergroups.get(owner_group.value, "invalid group name");

    return ownergroup.account;
}