#include "itamstoredex.hpp"

ACTION itamstoredex::sellorder(name owner, symbol_code symbol_name, string item_id, asset quantity)
{
    token_table tokens(_self, _self.value);
    const auto& token = tokens.get(quantity.symbol.code().raw(), "invalid token symbol");
    eosio_assert(token.available_symbol.precision() == quantity.symbol.precision(), "invalid precision");
    #ifdef BETA
        eosio_assert(quantity.symbol.code().to_string() == "ITT", "test symbol does not exact");
    #endif

    uint64_t itemid = stoull(item_id, 0, 10);

    name nft_contract(NFT_CONTRACT);
    account_table accounts(nft_contract, symbol_name.raw());
    const auto& item = accounts.get(itemid, "cannot found item id");
    
    require_auth(item.owner_account);
    eosio_assert(item.owner == owner, "wrong owner");

    order_table orders(_self, symbol_name.raw());
    orders.emplace(item.owner_account, [&](auto &o) {
        o.item_id = itemid;
        o.owner = item.owner;
        o.owner_account = item.owner_account;
        o.owner_nickname = item.nickname;
        o.quantity = quantity;
    });

    action(
        permission_level( _self, name("active") ),
        nft_contract,
        name("transfernft"),
        make_tuple( owner, _self, symbol_name, item_id, string("|") + _self.to_string() )
    ).send();

    currency_table currencies(nft_contract, nft_contract.value);
    const auto& currency = currencies.get(symbol_name.raw(), "invalid nft symbol");

    SEND_INLINE_ACTION(
        *this,
        receipt,
        { { _self, name("active") } },
        {
            owner,
            item.owner_account == name(ITAM_GROUP_ACCOUNT) ? name("itam") : name("eos"),
            currency.app_id,
            itemid,
            item.nickname,
            item.group_id,
            item.item_name,
            item.options,
            item.duration,
            item.transferable,
            quantity,
            string("make_order")
        }
    );
}

ACTION itamstoredex::cancelorder(name owner, symbol_code symbol_name, string item_id)
{
    uint64_t itemid = stoull(item_id, 0, 10);
    order_table orders(_self, symbol_name.raw());
    const auto& order = orders.require_find(itemid, "not exists item id");

    eosio_assert(has_auth(order->owner_account) || has_auth(_self), "missing required authority");
    eosio_assert(order->owner == owner, "different owner");

    name nft_contract(NFT_CONTRACT);

    account_table accounts(nft_contract, symbol_name.raw());
    const auto& ordered_item = accounts.get(itemid, "cannot found item id");
    eosio_assert(ordered_item.owner_account == _self, "owner is not itamstoredex");

    action(
        permission_level( _self, name("active") ),
        nft_contract,
        name("transfernft"),
        make_tuple( _self, order->owner_account, symbol_name, item_id, order->owner_nickname + string("|") + owner.to_string() )
    ).send();

    currency_table currencies(nft_contract, nft_contract.value);
    const auto& currency = currencies.get(symbol_name.raw(), "invalid nft symbol");

    SEND_INLINE_ACTION(
        *this,
        receipt,
        { { _self, name("active") } },
        {
            owner,
            order->owner_account == name(ITAM_GROUP_ACCOUNT) ? name("itam") : name("eos"),
            currency.app_id, 
            itemid,
            order->owner_nickname,
            ordered_item.group_id,
            ordered_item.item_name,
            ordered_item.options,
            ordered_item.duration,
            ordered_item.transferable,
            order->quantity,
            string("cancel_order")
        }
    );

    orders.erase(order);
}

ACTION itamstoredex::deleteorders(symbol_code symbol_name, vector<string> item_ids)
{
    require_auth(_self);
    order_table orders(_self, symbol_name.raw());
    for(auto iter = item_ids.begin(); iter != item_ids.end(); iter++)
    {
        uint64_t item_id = stoull(*iter, 0, 10);
        const auto& order = orders.require_find(item_id, "invalid item id");
        orders.erase(order);
    }
}

ACTION itamstoredex::resetorders(symbol_code symbol_name)
{
    require_auth(_self);
    order_table orders(_self, symbol_name.raw());
    for(auto order = orders.begin(); order != orders.end(); order = orders.erase(order));
}

ACTION itamstoredex::transfer(uint64_t from, uint64_t to)
{
    eosio_assert(_code != _self, "self cannot call transfer action");
    transfer_data data = unpack_action_data<transfer_data>();
    if (data.from == _self || data.to != _self || data.from == name("itamgamesgas") || data.from == name("itamtestgmsv")) return;

    transfer_memo message;
    parseMemo(&message, data.memo, "|", 4);

    token_table tokens(_self, _self.value);
    const auto& token = tokens.get(data.quantity.symbol.code().raw(), "invalid token symbol");
    eosio_assert(token.contract_name == _code, "token contract does not match");

    symbol item_symbol(message.symbol_name, 0);
    uint64_t item_id = stoull(message.item_id, 0, 10);
    
    order_table orders(_self, item_symbol.code().raw());
    const auto& order = orders.require_find(item_id, "item not found in orders");
    eosio_assert(order->quantity == data.quantity, "wrong quantity");
    
    config_table configs(_self, _self.value);
    const auto& config = configs.get(_self.value, "config must be set");

    asset fees = order->quantity * config.fees_rate / 100;

    // send price - fees to item owner
    action(
        permission_level{ _self, name("active") },
        token.contract_name,
        name("transfer"),
        make_tuple(_self, order->owner_account, order->quantity - fees, order->owner.to_string())
    ).send();

    asset settle_quantity_to_vendor = fees * config.settle_rate / 100;
    asset settle_quantity_to_itam = fees - settle_quantity_to_vendor;

    name nft_contract(NFT_CONTRACT);
    currency_table currencies(nft_contract, nft_contract.value);
    const auto& currency = currencies.get(item_symbol.code().raw(), "invalid token symbol");
    #ifndef BETA
        if(settle_quantity_to_vendor.amount > 0)
        {
            action(
                permission_level{ _self, name("active") },
                token.contract_name,
                name("transfer"),
                make_tuple( _self, name(CENTRAL_SETTLE_ACCOUNT), settle_quantity_to_vendor, to_string(currency.app_id) )
            ).send();
        }

        if(settle_quantity_to_itam.amount > 0)
        {
            action(
                permission_level{ _self, name("active") },
                token.contract_name,
                name("transfer"),
                make_tuple( _self, name(ITAM_SETTLE_ACCOUNT), settle_quantity_to_itam, to_string(currency.app_id) )
            ).send();
        }
    #endif

    string owner_group_name;
    name nft_receiver;
    if(data.from == name(ITAM_GROUP_ACCOUNT))
    {
        owner_group_name = "itam";
        nft_receiver = name(message.owner);
    }
    else
    {
        owner_group_name = "eos";
        nft_receiver = name(data.from);
    }  

    string nft_memo = message.buyer_nickname + string("|") + nft_receiver.to_string();

    account_table accounts(nft_contract, item_symbol.code().raw());
    const auto& trade_item = accounts.get(item_id, "invalid item");

    action(
        permission_level( _self, name("active") ),
        nft_contract,
        name("transfernft"),
        make_tuple( _self, data.from, item_symbol.code(), message.item_id, nft_memo )
    ).send();

    SEND_INLINE_ACTION(
        *this,
        receipt,
        { { _self, name("active") } },
        {
            nft_receiver,
            name(owner_group_name), 
            currency.app_id, 
            item_id, 
            message.buyer_nickname,
            trade_item.group_id,
            trade_item.item_name,
            trade_item.options,
            trade_item.duration,
            trade_item.transferable,
            data.quantity,
            string("order_complete")
        }
    );

    orders.erase(order);
}

ACTION itamstoredex::setconfig(uint64_t fees_rate, uint64_t settle_rate)
{
    require_auth(_self);

    config_table configs(_self, _self.value);
    const auto& config = configs.find(_self.value);

    if(config == configs.end())
    {
        configs.emplace(_self, [&](auto &c) {
            c.key = _self;
            c.fees_rate = fees_rate;
            c.settle_rate = settle_rate;
        });
    }
    else
    {
        configs.modify(config, _self, [&](auto &c) {
            c.fees_rate = fees_rate;
            c.settle_rate = settle_rate;
        });
    }
}

ACTION itamstoredex::receipt(name owner, name owner_group, uint64_t app_id, uint64_t item_id, string nickname, uint64_t group_id, string item_name, string options, uint64_t duration, bool transferable, asset payment_quantity, string state)
{
    require_auth(_self);

    name account = get_group_account(owner, owner_group);
    require_recipient(_self, account);
}

ACTION itamstoredex::settoken(name contract_name, string symbol_name, uint32_t precision)
{
    require_auth(_self);

    symbol available_symbol(symbol_name, precision);

    token_table tokens(_self, _self.value);
    const auto& token = tokens.find(available_symbol.code().raw());
    if(token == tokens.end())
    {
        tokens.emplace(_self, [&](auto &t) {
            t.available_symbol = available_symbol;
            t.contract_name = contract_name;
        });
    }
    else
    {
        tokens.modify(token, _self, [&](auto &t) {
            t.contract_name = contract_name;
        });
    }    
}

ACTION itamstoredex::deletetoken(string symbol_name, uint32_t precision)
{
    require_auth(_self);

    symbol available_symbol(symbol_name, precision);

    token_table tokens(_self, _self.value);
    const auto& token = tokens.require_find(available_symbol.code().raw(), "invalid token symbol");
    tokens.erase(token);
}