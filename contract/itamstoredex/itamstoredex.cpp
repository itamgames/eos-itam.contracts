#include "itamstoredex.hpp"

ACTION itamstoredex::sellorder(name owner, uint64_t token_id, symbol_code symbol_name, asset price)
{
    name asset_contract = name("itamdigasset");
    SEND_INLINE_ACTION(asset_contract,
                       transfernft,
                       {owner, "active"_n},
                       {owner, *this, symbol_name.to_string(), vector<uint64_t>{ token_id }, string("regist orderbook for sell item")});

    order_table orders(_self, symbol_name.raw());
    // eosio_assert(orders.find(token_id) == orders.end(), "exists ");
    
    ownergroup_table groups(_self, _self.value);
    const auto& group = groups.find(owner.value);
    string group_name = group != groups.end() ? group->group_name : "blockone";
    
    orders.emplace(_self, [&](auto &o) {
        o.token_id = token_id;
        o.owner = owner;
        o.price = price;
        o.group_name = name(group_name);
    });
}

ACTION itamstoredex::cancelorder(symbol_code symbol_name, uint64_t token_id)
{
    order_table orders(_self, symbol_name.raw());
    const auto& order = orders.require_find(token_id, "not exists order book");

    require_auth(order->owner);
}

ACTION itamstoredex::transfer(uint64_t from, uint64_t to)
{
    transfer_data tx_data = unpack_action_data<transfer_data>();
    if (tx_data.from == _self || tx_data.to != _self) return;

    name asset_contract = name("itamdigasset");
    SEND_INLINE_ACTION(asset_contract,
                       transfernft,
                       {});
}