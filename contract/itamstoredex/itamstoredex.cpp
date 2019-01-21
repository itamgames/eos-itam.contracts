#include "itamstoredex.hpp"


ACTION itamstoredex::test(uint64_t cmd)
{
    name owner = name("itamgameusra");

    switch(cmd)
    {
        case 1: {
            items_t item(_self,owner.value);
            for(auto itr=item.begin();itr!=item.end();)
            {
                itr=item.erase(itr);
            }
            break;
        }
        default: {
            break;
        }
    }

    print_f("test action confirmed");
}

ACTION itamstoredex::createitem(uint64_t id, uint64_t gid, string itemname, string itemopt, name to)
{
    require_auth(_self);
    eosio_assert(is_account(to), "invalid account.");

    items_t item(_self, to.value);
    item.emplace(_self, [&](auto& s){
        s.id = id;
        s.gid = gid;
        s.itemname = itemname;
        s.itemopt = itemopt;
        s.istatus = ISTAT_ACTIVE;
        s.eosvalue.amount = 0;
        s.eosvalue.symbol = symbol("EOS", 4);
        s.itamvalue.amount = 0;
        s.itamvalue.symbol = symbol("ITAM", 4);
        s.owner = to;
    });
}

ACTION itamstoredex::edititem(uint64_t id, uint64_t gid, string itemname, string itemopt, name holder)
{
    require_auth(_self);
    eosio_assert(is_account(holder), "invalid account.");

    items_t item(_self,holder.value);
    auto item_itr = item.require_find(id, "no matched id.");

    eosio_assert(item_itr->gid == gid, "not valid gid");

    item.modify(item_itr, _self, [&](auto& s){
        s.itemname = itemname;
        s.itemopt = itemopt;
    });
}

ACTION itamstoredex::removeitem(uint64_t id, name holder)
{
    require_auth(_self);

    items_t items(_self, holder.value);
    
    auto item = items.require_find(id, "no matched id.");
    items.erase(item);
}

ACTION itamstoredex::sellorder(uint64_t id, name from, asset eosvalue, asset itamvalue)
{
    require_auth(from);
    
    items_t item(_self, from.value);
    items_t oitem(_self, _self.value);

    auto item_itr = item.require_find(id, "no matched id.");

    auto oitem_iter = oitem.find(id);

    eosio_assert(oitem_iter == oitem.end(), "id already exist.");

    oitem.emplace(_self, [&](auto& s){
        s.id = item_itr->id;
        s.gid = item_itr->gid;
        s.itemname = item_itr->itemname;
        s.itemopt = item_itr->itemopt;
        s.istatus = ISTAT_4SALE;
        s.eosvalue = eosvalue;
        s.itamvalue = itamvalue;
        s.owner = from;
    });

    item.erase(item_itr);
}

ACTION itamstoredex::revokeorder(uint64_t id, name from)
{
    require_auth(from);
    items_t oitem(_self, _self.value);
    items_t item(_self, from.value);

    auto oitem_iter = oitem.require_find(id, "no matched id in orderlist");

    auto item_itr = item.find(id);
    eosio_assert(item_itr == item.end(), "id already exist in users table.");

    eosio_assert(from == oitem_iter->owner, "item not owned.");

    item.emplace(_self, [&](auto& s){
        s.id = oitem_iter->id;
        s.gid = oitem_iter->gid;
        s.itemname = oitem_iter->itemname;
        s.itemopt = oitem_iter->itemopt;
        s.istatus = ISTAT_ACTIVE;
        s.eosvalue.amount = 0;
        s.itamvalue.amount = 0;
        s.owner = from;
    });
    
    oitem.erase(oitem_iter);
}

ACTION itamstoredex::receiptrans(uint64_t id, uint64_t gid, string itemname, string itemopt)
{
    require_auth(_self);
    require_recipient(_self);
}

void itamstoredex::transfer(uint64_t sender, uint64_t receiver)
{
    if(sender == _self.value)
    {
        return;
    }

    auto transfer_data = unpack_action_data<tdata>();

    st_memo msg;

    parseMemo(transfer_data.memo, &msg, "|");
    uint64_t itemid = stoull(msg.itemid, 0, 10);

    items_t item(_self, sender);
    items_t oitem(_self, _self.value);

    auto oitem_iter = oitem.require_find(itemid, "no matched id");
    eosio_assert(oitem_iter->owner != transfer_data.from, "owner can't buy owned item. use revoke instead.");

    auto item_iter = item.find(itemid);
    eosio_assert(item_iter == item.end(), "id already exist.");

    symbol eos_symbol = symbol("EOS", 4);
    symbol itam_symbol = symbol("ITAM", 4);
    asset give_back_asset;
    name contract_name;

    if(transfer_data.quantity.symbol == eos_symbol)
    {
        eosio_assert(transfer_data.quantity.amount == oitem_iter->eosvalue.amount, "Wrong EOS Amount");
        
        give_back_asset = oitem_iter->eosvalue;
        give_back_asset.amount = oitem_iter->eosvalue.amount * 99 / 100;

        contract_name.value = name("eosio.token").value;
    }
    else if(transfer_data.quantity.symbol == itam_symbol)
    {
        eosio_assert(transfer_data.quantity.amount == oitem_iter->itamvalue.amount, "Wrong ITAM Amount");
        
        give_back_asset = oitem_iter->itamvalue;
        give_back_asset.amount = oitem_iter->itamvalue.amount * 99 / 100;

        contract_name.value = name("itamtokenadm").value;
    }
    else
    {
        eosio_assert(false, "Invalid symbol");
    }

    action(
        permission_level{_self, name("active")},
        contract_name,
        name("transfer"),
        make_tuple(
            _self,
            oitem_iter->owner, 
            give_back_asset,
            string("asset exchange request")
        )
    ).send();

    item.emplace(_self, [&](auto& s){
        s.id = oitem_iter->id;
        s.gid = oitem_iter->gid;
        s.itemname = oitem_iter->itemname;
        s.itemopt = oitem_iter->itemopt;
        s.istatus = ISTAT_ACTIVE;
        s.eosvalue = oitem_iter->eosvalue;
        s.itamvalue = oitem_iter->itamvalue;
        s.owner.value = sender;
    });

    action(
        permission_level{_self, name("active")},
        _self, 
        name("receiptrans"),
        make_tuple(
            oitem_iter->id,
            oitem_iter->gid, 
            oitem_iter->itemname,
            oitem_iter->itemopt
        )
    ).send();

    oitem.erase(oitem_iter);
}

// parseMemo - parse memo from user transfer
void itamstoredex::parseMemo(string memo, st_memo* msg, string delimiter)
{
    string::size_type lastPos = memo.find_first_not_of(delimiter, 0);
    string::size_type pos = memo.find_first_of(delimiter, lastPos);

    string *ptr = (string*)msg;

    for(uint64_t i = 0; string::npos != pos || string::npos != lastPos; i++)
    {
        string temp = memo.substr(lastPos, pos - lastPos);

        lastPos = memo.find_first_not_of(delimiter, pos);
        pos = memo.find_first_of(delimiter, lastPos);

        ptr[i] = temp;
    }
}

#define EOSIO_DISPATCH_EX( TYPE, MEMBERS ) \
extern "C" { \
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
        bool can_call_transfer = code == name("eosio.token").value || code == name("itamtokenadm").value; \
        if( code == receiver || can_call_transfer ) { \
            if(action == name("transfer").value) { \
                eosio_assert(can_call_transfer, "only eosio.token can call internal transfer"); \
            } \
            switch( action ) { \
                EOSIO_DISPATCH_HELPER( TYPE, MEMBERS ) \
            } \
         /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
        } \
    } \
} \

EOSIO_DISPATCH_EX( itamstoredex, (test)(createitem)(edititem)(removeitem)(sellorder)(transfer)(receiptrans)(revokeorder) )
