#include "itamstoreapp.hpp"

#define DEBUG_MODE

ACTION itamstoreapp::test(uint64_t cmd)
{
    require_auth(_self);

    switch(cmd)
    {
        case 1: {
            settle_t settle(_self,_self.value);
            for(auto itr=settle.begin();itr!=settle.end();)
            {
                itr=settle.erase(itr);
            }
            break;
        }
        default: {
            break;
        }
    }

    print("test action");
}

ACTION itamstoreapp::regsellitem(uint64_t gid, uint64_t itemid, string itemname, asset eosvalue, asset itamvalue, string itemdesc)
{
    require_auth(_self);

    sitems_t sellitem(_self,_self.value);
    sellitem.emplace(_self,[&](auto& s){
        s.id = itemid;
        s.gid = gid;
        s.itemname = itemname;
        s.eosvalue = eosvalue;
        s.itamvalue = itamvalue;
        s.itemdesc = itemdesc;
    });
}

ACTION itamstoreapp::regsettle(uint64_t gid, name account)
{
    require_auth(_self);

    settle_t settle(_self, _self.value);
    settle.emplace(_self, [&](auto& s) {
        s.gid = gid;
        s.account = account;
        s.eosvalue.amount = 0;
        s.eosvalue.symbol = symbol("EOS", 4);
        s.itamvalue.amount = 0;
        s.itamvalue.symbol = symbol("ITAM", 4);
    });
}

ACTION itamstoreapp::delsellitem(uint64_t gid, uint64_t itemid)
{
    require_auth(_self);
    sitems_t sellitem(_self,_self.value);

    #ifdef DEBUG_MODE
        auto sitem_itr = sellitem.require_find(itemid, "no matched id");
    #else
        auto sitem_itr = sellitem.find(itemid);
    #endif

    sellitem.erase(sitem_itr);
}

ACTION itamstoreapp::modsellitem(uint64_t gid, uint64_t itemid, string itemname, asset eosvalue, asset itamvalue, string itemdesc)
{
    require_auth(_self);
    sitems_t sellitem(_self,_self.value);

    #ifdef DEBUG_MODE
        auto sitem_itr = sellitem.require_find(itemid, "no matched id.");
    #else
        auto sitem_itr = sellitem.find(itemid);
    #endif

    sellitem.modify(sitem_itr, _self, [&](auto& s){
        s.itemname = itemname;
        s.eosvalue = eosvalue;
        s.itamvalue = itamvalue;
        s.itemdesc = itemdesc;
    });
}

ACTION itamstoreapp::receiptrans(uint64_t gameid, uint64_t itemid, string itemname, name from, asset value, string notititle ,string notitext ,string notistr, string options)
{
    require_auth(_self);
    require_recipient(_self);
}

ACTION itamstoreapp::msettlename(uint64_t gid, name account)
{
    require_auth(_self);
    eosio_assert(is_account(account), "invalid account.");

    settle_t settle(_self,_self.value);

    #ifdef DEBUG_MODE
        auto settle_itr = settle.require_find(gid, "not valid gid");
    #else
        auto settle_itr = settle.find(gid);
    #endif
    
    settle.modify(settle_itr, _self, [&](auto& s) {
        s.account = account;
    });
}

ACTION itamstoreapp::resetsettle(uint64_t gid)
{
    require_auth(_self);
    settle_t settle(_self, _self.value);
    
    #ifdef DEBUG_MODE
        auto settle_itr = settle.require_find(gid, "not valid gid");
    #else
        auto settle_itr = settle.find(gid);
    #endif
    
    settle.modify(settle_itr, _self, [&](auto& s){
        s.eosvalue.amount = 0;
        s.itamvalue.amount = 0;
    });
}

ACTION itamstoreapp::rmsettle(uint64_t gid)
{
    require_auth(_self);
    settle_t settle(_self,_self.value);
    
    #ifdef DEBUG_MODE
        auto settle_itr = settle.require_find(gid, "not valid gid");
    #else
        auto settle_itr = settle.find(gid);
    #endif

    settle.erase(settle_itr);
}

ACTION itamstoreapp::claimsettle(uint64_t gid, name from)
{
    require_auth(from);
    settle_t settle(_self,_self.value);

    #ifdef DEBUG_MODE
        auto settle_itr = settle.require_find(gid, "no matched gid.");
    #else
        auto settle_itr = settle.find(gid);
    #endif

    asset eos_settle(settle_itr->eosvalue.amount * 70 / 100, symbol("EOS", 4));
    asset itam_settle(settle_itr->itamvalue.amount * 70 / 100, symbol("ITAM", 4));

    eosio_assert(eos_settle.amount + itam_settle.amount > 0, "not enought value for settlement");
    
    string memo = "transfer from settle request";
    if(eos_settle.amount > 0)
    {
        action(
            permission_level{_self, name("active")},
            name("eosio.token"), 
            name("transfer"),
            make_tuple(
                _self,
                settle_itr->account, 
                eos_settle,
                memo
            )
        ).send();
    }
    if(itam_settle.amount > 0)
    {
        action(
            permission_level{_self, name("active")},
            name("itamtokenadm"), 
            name("transfer"),
            make_tuple(
                _self,
                settle_itr->account, 
                itam_settle,
                memo
            )
        ).send();
    }

    settle.modify(settle_itr, _self, [&](auto& s){
        s.eosvalue.amount = 0;
        s.itamvalue.amount = 0;
    });
}

void itamstoreapp::transfer(uint64_t sender, uint64_t receiver)
{
    auto transfer_data = unpack_action_data<tdata>();
    
    sitems_t sellitem(_self,_self.value);
    settle_t settle(_self,_self.value);

    if (transfer_data.from == _self || transfer_data.to != _self)
    {
        return;
    }

    st_memo msg;
    parseMemo(transfer_data.memo, &msg, "|");

    auto sitem_itr = sellitem.require_find(stoull(msg.itemid, 0, 10), "no matched id.");
    eosio_assert(sitem_itr->itemname == msg.itemname, "not valid item name");
    eosio_assert(sitem_itr->gid == stoull(msg.gid, 0, 10), "not valid gid");

    auto s_itr = settle.require_find(stoull(msg.gid, 0, 10), "not valid gid settle info");

    settle.modify(s_itr, _self, [&](auto& s){
        if(sitem_itr->eosvalue.amount > 0)
        {
            eosio_assert(sitem_itr->eosvalue == transfer_data.quantity, "invalid purchase value");
            s.eosvalue += transfer_data.quantity;
        }
        else
        {
            eosio_assert(sitem_itr->itamvalue == transfer_data.quantity, "invalid purchase value");
            s.itamvalue += transfer_data.quantity;
        }
    });

    action(
        permission_level{_self, name("active")},
        _self, 
        name("receiptrans"),
        make_tuple(
            stoull(msg.itemid,0,10),
            stoull(msg.gid,0,10), 
            msg.itemname,
            transfer_data.from,
            transfer_data.quantity,
            msg.notititle,
            msg.notitext,
            msg.notikey,
            sitem_itr->itemdesc
        )
    ).send();
}

// parseMemo - parse memo from user transfer
void itamstoreapp::parse_memo(string memo, st_memo* msg, string delimiter)
{
    string::size_type lastPos = memo.find_first_not_of(delimiter, 0);
    string::size_type pos = memo.find_first_of(delimiter, lastPos);

    string *ptr = (string*)msg;

    for(int i = 0; string::npos != pos || string::npos != lastPos; i++)
    {
        string temp = memo.substr(lastPos, pos - lastPos);

        lastPos = memo.find_first_not_of(delimiter, pos);
        pos = memo.find_first_of(delimiter, lastPos);

        ptr[i] = temp;
    }
}

//EOSIO_DISPATCH( itamstoreapp, (test)(regsellitem)(delsellitem)(modsellitem)(receiptrans)(regsettle)(msettlename)(resetsettle)(rmsettle)(claimsettle) )
#define EOSIO_DISPATCH_EX( TYPE, MEMBERS ) \
extern "C" { \
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
        bool can_call_transfer = code == name("eosio.token").value || code == name("itamtokenadm").value; \
        \
        if( code == receiver || can_call_transfer ) { \
            if(action == name("transfer").value) { \
                eosio_assert(can_call_transfer, "eosio.token or itamtokenadm can call internal transfer"); \
            } \
            switch( action ) { \
                EOSIO_DISPATCH_HELPER( TYPE, MEMBERS ) \
            } \
         /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
        } \
    } \
} \

EOSIO_DISPATCH_EX( itamstoreapp, (test)(regsellitem)(delsellitem)(modsellitem)(transfer)(receiptrans)(regsettle)(msettlename)(resetsettle)(rmsettle)(claimsettle) )
