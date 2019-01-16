#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>

using namespace eosio;
using namespace std;

CONTRACT itamstakeadm : public eosio::contract {
    public :
        using contract::contract;

        const uint64_t SEC_REFUND_DELAY =                10;

        ACTION test(uint64_t cmd);
        ACTION staking(name owner, asset value);
        ACTION unstaking(name owner, asset value);
        ACTION drefund(name owner, asset value);
        ACTION mrefund(name owner, asset value);

    private :
        struct account {
            asset          balance;
            
            // bit shift needs to backward compatibility 
            uint64_t primary_key()const { return balance.symbol.code().raw(); }
        };
        typedef eosio::multi_index<"accounts"_n, account> accounts_t;

        struct refund_info {
            asset           refund_amount;
            uint64_t        req_refund_ts;
        };

        struct [[eosio::table]] stake {
            name                owner;
            asset               stakebalance;

            uint64_t primary_key()const { return owner.value; }
        };
        typedef eosio::multi_index<"stakes"_n, stake> stakes_t;

        struct [[eosio::table]] refund {
            asset token_symbol;
            asset total_refund;
            std::vector<refund_info> refund_list;

            uint64_t primary_key()const { return token_symbol.symbol.code().raw(); }
        };
        typedef eosio::multi_index<"refunds"_n,refund> refunds_t;
};

ACTION itamstakeadm::test(uint64_t cmd)
{
    name owner = name("itamgameusra");

    /*
    switch(cmd)
    {
        case 1: {
            refunds_t trefund(_self,owner.value);
            for(auto itr=trefund.begin();itr!=trefund.end();)
            {
                itr=trefund.erase(itr);
            }
            break;
        }
        case 2: {
            stakes_t sstake(_self,_self.value);
            for(auto itr=sstake.begin();itr!=sstake.end();)
            {
                itr=sstake.erase(itr);
            }
            break;
        }
        default: {
            break;
        }
    }
    */
    print_f("test code");
}

ACTION itamstakeadm::staking(name owner,asset value)
{
    require_auth(owner);
    eosio_assert(is_account(owner),"invalid account");

    accounts_t from_acnt(name("itamtokenadm"), owner.value);
    const auto& from = from_acnt.get( (value.symbol.code().raw()), "no balance object found" );
    eosio_assert(from.balance.symbol == value.symbol, "invalid symbol");
    eosio_assert(from.balance.amount >= value.amount,"overdrawn balance");
    eosio_assert(value.amount > 0, "stake value must be positive");

    uint64_t stakedval = 0;
    uint64_t refundingval = 0;
    uint64_t tbal = from.balance.amount;

    stakes_t tstake(_self,_self.value);

    refunds_t trefund(_self,owner.value);
    auto ritr = trefund.find(value.symbol.code().raw());
    if(ritr != trefund.end())
    {
        refundingval = ritr->total_refund.amount;
    }

    auto sitr = tstake.find(owner.value);
    if(sitr == tstake.end())
    {
        tstake.emplace(_self,[&](auto& s){
            s.owner = owner;
            s.stakebalance = value;
        });
    }
    else
    {
	    stakedval = sitr->stakebalance.amount;
        
        eosio_assert(tbal >= (stakedval + refundingval + value.amount),"overdrawn current balance");
        tstake.modify(sitr,_self,[&](auto& s){
            s.stakebalance += value;
        });
    }
}

ACTION itamstakeadm::unstaking(name owner,asset value)
{
    require_auth(owner);
    eosio_assert(is_account(owner),"invalid account");

    stakes_t tstake(_self,_self.value);
    auto sitr = tstake.find(owner.value);

    eosio_assert(sitr != tstake.end(),"can find stake id");
    eosio_assert(sitr->stakebalance >= value, "overdrawn balance for stake");

    tstake.modify(sitr,_self,[&](auto& s){
        s.stakebalance -= value;
    });

    refunds_t trefund(_self,owner.value);
    auto ritr = trefund.find(value.symbol.code().raw());

    if(ritr == trefund.end())
    {
        trefund.emplace(_self,[&](auto& v){
            v.token_symbol = value;
            v.token_symbol.amount = 0;
            refund_info info;
            info.refund_amount = value;
            info.req_refund_ts = now();
            v.total_refund = value;
            v.refund_list.push_back(info);
        });
    }
    else
    {
        trefund.modify(ritr,_self,[&](auto& v){
            refund_info info;
            info.refund_amount = value;
            info.req_refund_ts = now();
            v.total_refund += value;
            v.refund_list.push_back(info);
        });
    }

    transaction txn{};
    txn.actions.emplace_back(
        permission_level(_self,name("active")),
        _self,
        name("drefund"),
        make_tuple(owner,value)
    );

    txn.delay_sec = SEC_REFUND_DELAY;
    txn.send(now(),owner);
}

ACTION itamstakeadm::drefund(name owner,asset value)
{
    require_auth(_self);
    refunds_t trefund(_self,owner.value);
    auto ritr = trefund.find(value.symbol.code().raw());
    eosio_assert(ritr != trefund.end(),"refund info not exist");

    trefund.modify(ritr,_self,[&](auto& v){
        uint64_t dcount = 0;
        uint64_t trows = ritr->refund_list.size();
        for(uint64_t i=0;i<trows;i++)
        {
            if(ritr->refund_list[i-dcount].req_refund_ts + SEC_REFUND_DELAY <= now())
            {
                v.total_refund.amount -= ritr->refund_list[i].refund_amount.amount;
                v.refund_list.erase(v.refund_list.begin() + (i - dcount));
                dcount++;
            }
        }
    });
}

ACTION itamstakeadm::mrefund(name owner, asset value)
{
    require_auth(_self);
    refunds_t trefund(_self,owner.value);
    auto ritr = trefund.find(value.symbol.code().raw());
    eosio_assert(ritr != trefund.end(),"refund info not exist");

    trefund.modify(ritr,_self,[&](auto& v){
        uint64_t dcount = 0;
        uint64_t trows = ritr->refund_list.size();
        for(uint64_t i=0;i<trows;i++)
        {
            if(ritr->refund_list[i-dcount].req_refund_ts + SEC_REFUND_DELAY <= now())
            {
                v.total_refund.amount -= ritr->refund_list[i - dcount].refund_amount.amount;
                v.refund_list.erase(v.refund_list.begin() + (i - dcount));
                dcount++;
            }
        }
    });
}

//EOSIO_DISPATCH( itamstoredex, (test)(stake)(unstaking)(drefund)(mrefund) )
#define EOSIO_DISPATCH_EX( TYPE, MEMBERS ) \
extern "C" { \
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
        if( code == receiver || code == name("eosio.token").value ) { \
            if(action == name("transfer").value) { \
                eosio_assert(code == name("eosio.token").value,"only eosio.token can call internal transfer"); \
            } \
            switch( action ) { \
                EOSIO_DISPATCH_HELPER( TYPE, MEMBERS ) \
            } \
         /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
        } \
    } \
} \

EOSIO_DISPATCH_EX( itamstakeadm, (test)(staking)(unstaking)(drefund)(mrefund) )