#include "itamitamitam.hpp"

ACTION itamitamitam::addgroup(name group_name, name group_account)
{
    require_auth(_self);
    eosio_assert(is_account(group_account), "group_account does not exist");
    
    ownergroup_table ownergroups(_self, _self.value);
    ownergroups.emplace(_self, [&](auto &o) {
        o.owner = group_name;
        o.account = group_account;
    });
}

ACTION itamitamitam::modifygroup(name owner, name group_account)
{
    require_auth(_self);
    eosio_assert(is_account(group_account), "group_account does not exist");

    ownergroup_table ownergroups(_self, _self.value);
    const auto& ownergroup = ownergroups.require_find(owner.value, "invalid group name");

    ownergroups.modify(ownergroup, _self, [&](auto &o) {
        o.account = group_account;
    });
}