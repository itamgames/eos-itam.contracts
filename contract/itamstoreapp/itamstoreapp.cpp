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
        case 2: {
            sellItemTable sellItems(_self,_self.value);
            for(auto itr=sellItems.begin();itr!=sellItems.end();)
            {
                itr=sellItems.erase(itr);
            }
            break;
        }
        default: {
            break;
        }
    }

    print("test action");
}

ACTION itamstoreapp::regsellitem(string params)
{
    eosio_assert(has_auth(_self) || has_auth("itampayapp"_n), "Missing required authority");

    auto parsedParams = json::parse(params);

    if(parsedParams.count("appId") == 0 || parsedParams.count("items") == 0) return;

    uint64_t appId = parsedParams["appId"];
    auto items = parsedParams["items"];
    
    sellItemTable sellItems(_self, appId);

    for(auto iter = sellItems.begin(); iter != sellItems.end();)
    {
        iter = sellItems.erase(iter);
    }

    for(int i = 0; i < items.size(); i++)
    {
        sellItems.emplace(_self, [&](auto &s) {
            s.itemId = items[i]["itemId"];
            s.itemName = items[i]["itemName"];
            s.eos.amount = stoull(items[i]["eos"].get<std::string>(), 0, 10) * 10000;
            s.eos.symbol = symbol("EOS", 4);
            s.itam.amount = stoull(items[i]["itam"].get<std::string>(), 0, 10) * 10000;
            s.itam.symbol = symbol("ITAM", 4);
        });
    }
}

ACTION itamstoreapp::mdelsellitem(string jsonstr)
{
    require_auth(_self);
    sellItemTable sellItems(_self, _self.value);
    auto j = json::parse(jsonstr);

    for(int i = 0; i < j.size(); i++)
    {
        #ifdef DEBUG_MODE
            auto sellItem = sellItems.require_find(stoull(j[i]["itemId"].get<std::string>(),0,10), "no matched id");
            eosio_assert(sellItem->itemId == stoull(j[i]["itemId"].get<std::string>(), 0, 10), "not valid appId");
        #else
            auto sellItem = sellItems.find(stoull(j[i]["itemId"].get<std::string>(),0,10));
        #endif
        sellItems.erase(sellItem);
    }
}

ACTION itamstoreapp::regsettle(uint64_t appId, name account)
{
    require_auth(_self);

    settle_t settle(_self, _self.value);
    settle.emplace(_self, [&](auto& s) {
        s.appId = appId;
        s.account = account;
        s.eosvalue.amount = 0;
        s.eosvalue.symbol = symbol("EOS", 4);
        s.itamvalue.amount = 0;
        s.itamvalue.symbol = symbol("ITAM", 4);
    });
}

ACTION itamstoreapp::delsellitem(uint64_t appId, uint64_t itemid)
{
    require_auth(_self);
    sellItemTable sellItems(_self, _self.value);

    #ifdef DEBUG_MODE
        auto sellItem = sellItems.require_find(itemid, "no matched id");
    #else
        auto sellItem = sellItems.find(itemid);
    #endif

    sellItems.erase(sellItem);
}

ACTION itamstoreapp::modsellitem(uint64_t appId, uint64_t itemid, string itemname, asset eosvalue, asset itamvalue)
{
    require_auth(_self);
    sellItemTable sellItems(_self, _self.value);

    auto sellItem = sellItems.require_find(itemid, "no matched id.");

    sellItems.modify(sellItem, _self, [&](auto& s){
        s.itemName = itemname;
        s.eos = eosvalue;
        s.itam = itamvalue;
    });
}

ACTION itamstoreapp::receiptrans(uint64_t appId, uint64_t itemId, string itemName, string packageName, string token, name from, asset price)
{
    require_auth(_self);
    require_recipient(from);
    require_recipient(_code);
}

ACTION itamstoreapp::msettlename(uint64_t appId, name account)
{
    require_auth(_self);
    eosio_assert(is_account(account), "invalid account.");

    settle_t settle(_self,_self.value);

    #ifdef DEBUG_MODE
        auto settle_itr = settle.require_find(appId, "not valid appId");
    #else
        auto settle_itr = settle.find(appId);
    #endif
    
    settle.modify(settle_itr, _self, [&](auto& s) {
        s.account = account;
    });
}

ACTION itamstoreapp::resetsettle(uint64_t appId)
{
    require_auth(_self);
    settle_t settle(_self, _self.value);
    
    #ifdef DEBUG_MODE
        auto settle_itr = settle.require_find(appId, "not valid appId");
    #else
        auto settle_itr = settle.find(appId);
    #endif
    
    settle.modify(settle_itr, _self, [&](auto& s){
        s.eosvalue.amount = 0;
        s.itamvalue.amount = 0;
    });
}

ACTION itamstoreapp::rmsettle(uint64_t appId)
{
    require_auth(_self);
    settle_t settle(_self,_self.value);
    
    #ifdef DEBUG_MODE
        auto settle_itr = settle.require_find(appId, "not valid appId");
    #else
        auto settle_itr = settle.find(appId);
    #endif

    settle.erase(settle_itr);
}

ACTION itamstoreapp::claimsettle(uint64_t appId, name from)
{
    require_auth(from);
    settle_t settle(_self,_self.value);

    #ifdef DEBUG_MODE
        auto settle_itr = settle.require_find(appId, "no matched appId.");
    #else
        auto settle_itr = settle.find(appId);
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

    if (transfer_data.from == _self || transfer_data.to != _self)
    {
        return;
    }

    st_memo msg;
    parse_memo(transfer_data.memo, &msg, "|");

    uint64_t appId = stoull(msg.appId, 0, 10);
    uint64_t itemId = stoull(msg.itemId, 0, 10);

    sellItemTable sellItems(_self, appId);
    settle_t settle(_self, _self.value);
    auto sellItem = sellItems.require_find(itemId, "no matched item id");
    eosio_assert(sellItem->itemName == msg.itemName, "not valid item name");

    auto s_itr = settle.require_find(appId, "not valid appId settle info");

    settle.modify(s_itr, _self, [&](auto& s){
        if(sellItem->eos.amount > 0)
        {
            eosio_assert(sellItem->eos == transfer_data.quantity, "invalid purchase value");
            s.eosvalue += transfer_data.quantity;
        }
        else
        {
            eosio_assert(sellItem->itam == transfer_data.quantity, "invalid purchase value");
            s.itamvalue += transfer_data.quantity;
        }
    });

    action(
        permission_level{_self, name("active")},
        _self, 
        name("receiptrans"),
        make_tuple(
            appId,
            itemId, 
            msg.itemName,
            msg.packageName,
            msg.token,
            transfer_data.from,
            transfer_data.quantity
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

//EOSIO_DISPATCH( itamstoreapp, (test)(regsellitem)(delsellitem)(modsellitem)(receiptrans)(regsettle)(msettlename)
// (resetsettle)(rmsettle)(claimsettle)(mregsellitem)(mdelsellitem) ) 
#define EOSIO_DISPATCH_EX( TYPE, MEMBERS ) \
extern "C" { \
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
        bool can_call_transfer = code == name("eosio.token").value || code == name("itamtokenadm").value; \
        \
        if( code == receiver || can_call_transfer || code == name("itampayapp").value ) { \
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

EOSIO_DISPATCH_EX( itamstoreapp, (test)(regsellitem)(delsellitem)(modsellitem)(transfer)(receiptrans)(regsettle)(msettlename)
                                    (resetsettle)(rmsettle)(claimsettle)(mdelsellitem) )
