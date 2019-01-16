#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;
using namespace std;

CONTRACT itamtoken : public eosio::contract {
  public:
      using contract::contract;

      ACTION create(name issuer, asset maximum_supply);
      ACTION issue(name to, asset quantity, string memo);
      ACTION transfer(name from, name to, asset quantity, string memo);

      // below is for bacward compatible. but not used. notice that it changed from symbol_name to symbol_code
      inline asset get_supply(symbol_code sym)const;
      inline asset get_balance(name owner, symbol_code sym)const;

      struct transfer_args {
         name           from;
         name           to;
         asset          quantity;
         string         memo;
      };

   private:
      struct stake {
         name                owner;
         asset               stakebalance;

         uint64_t primary_key()const { return owner.value; }
      };
      typedef eosio::multi_index<"stakes"_n, stake> stakes_t;

      struct refund_info {
         asset           refund_amount;
         uint64_t        req_refund_ts;
      };

      struct refund {
         asset token_symbol;
         asset total_refund;
         std::vector<refund_info> refund_list;

         uint64_t primary_key()const { return token_symbol.symbol.code().raw(); }
      };
      typedef eosio::multi_index<"refunds"_n,refund> refunds_t;

      struct [[eosio::table]] account {
         asset          balance;
         
         // bit shift needs to backward compatibility 
         uint64_t primary_key()const { return balance.symbol.code().raw(); }
      };
      
      struct [[eosio::table]] currency {
         asset          supply;
         asset          max_supply;
         name           issuer;

         // bit shift needs to backward compatibility 
         uint64_t primary_key()const { return supply.symbol.code().raw(); }
      };
      
      typedef eosio::multi_index<"accounts"_n, account> account_t;
      typedef eosio::multi_index<"stat"_n, currency> stat_t;

      void sub_balance(name owner, asset value);
      void add_balance(name owner, asset value, name ram_payer );
};

asset itamtoken::get_supply(symbol_code sym)const
{
   stat_t statstable(_self, sym.raw()>>8);
   const auto& st = statstable.get(sym.raw()>>8);
   return st.supply;
}

asset itamtoken::get_balance(name owner, symbol_code sym )const
{
   account_t accountstable(_self, owner.value);
   const auto& ac = accountstable.get(sym.raw() >> 8);
   return ac.balance;
}

void itamtoken::sub_balance(name owner, asset value)
{
   name stakecontract = name("itamstakeadm");
   account_t from_acnts(_self, owner.value);

   const auto& from = from_acnts.get(value.symbol.code().raw(), "no balance object found");
   eosio_assert(from.balance.amount >= value.amount, "overdrawn balance");

   uint64_t stakedval = 0;
   uint64_t refundingval = 0;

   stakes_t tstake(stakecontract,stakecontract.value);
   auto sitr = tstake.find(owner.value);
   if(sitr != tstake.end())
   {
      stakedval = sitr->stakebalance.amount;
   }

   refunds_t trefund(stakecontract,owner.value);
   auto ritr = trefund.find(value.symbol.code().raw());
   if(ritr != trefund.end())
   {
      refundingval = ritr->total_refund.amount;
   }

   eosio_assert(from.balance.amount >= (value.amount + stakedval + refundingval), "overdrawn current balance");

   if(from.balance.amount == value.amount) 
   {
      from_acnts.erase(from);
   } 
   else 
   {
      from_acnts.modify(from, owner, [&](auto& a) {
          a.balance -= value;
      });
   }
}

void itamtoken::add_balance(name owner, asset value, name ram_payer)
{
   account_t to_acnts(_self, owner.value);
   auto to = to_acnts.find(value.symbol.code().raw());

   if(to == to_acnts.end()) 
   {
      to_acnts.emplace(ram_payer, [&](auto& a) {
        a.balance = value;
      });
   } 
   else 
   {
      to_acnts.modify(to, ram_payer, [&](auto& a) {
        a.balance += value;
      });
   }
}

ACTION itamtoken::create(name issuer, asset maximum_supply)
{
   require_auth(_self);

   auto sym = maximum_supply.symbol;
   eosio_assert( sym.is_valid(), "invalid symbol name" );
   eosio_assert( maximum_supply.is_valid(), "invalid supply.");
   eosio_assert( maximum_supply.amount > 0, "max-supply must be positive.");
   stat_t statstable( _self, sym.code().raw());
   auto existing = statstable.find(sym.code().raw());
   eosio_assert(existing == statstable.end(), "token with symbol already exists.");
   
   statstable.emplace(_self,[&](auto& s) {
      s.supply.symbol = maximum_supply.symbol;
      s.max_supply    = maximum_supply;
      s.issuer        = issuer;
   });
}

ACTION itamtoken::issue(name to, asset quantity, string memo)
{
   auto sym = quantity.symbol;
   eosio_assert(sym.is_valid(), "invalid symbol name");
   eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

   auto sym_name = sym.code().raw();
   stat_t statstable(_self, sym_name);
   auto existing = statstable.find(sym_name);
   eosio_assert(existing != statstable.end(), "token with symbol does not exist, create token before issue");
   const auto& st = *existing;

   require_auth(st.issuer);
   eosio_assert(quantity.is_valid(), "invalid quantity");
   eosio_assert(quantity.amount > 0, "must issue positive quantity");

   eosio_assert(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
   eosio_assert(quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

   statstable.modify(st, _self, [&](auto& s) {
      s.supply += quantity;
   });

   add_balance(st.issuer, quantity, st.issuer);

   if(to != st.issuer) 
   {
      SEND_INLINE_ACTION(*this, transfer, {st.issuer,"active"_n}, {st.issuer, to, quantity, memo} );
   }
}

ACTION itamtoken::transfer(name from, name to, asset quantity, string memo)
{
   eosio_assert(from != to, "cannot transfer to self");
   require_auth(from);
   eosio_assert(is_account(to), "to account does not exist");
   
   auto sym = quantity.symbol.code().raw();
   stat_t statstable(_self, sym);
   const auto& st = statstable.get(sym);

   require_recipient(from);
   require_recipient(to);

   eosio_assert(quantity.is_valid(), "invalid quantity");
   eosio_assert(quantity.amount > 0, "must transfer positive quantity");
   eosio_assert(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
   eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");


   sub_balance(from, quantity);
   add_balance(to, quantity, from);
}

//EOSIO_DISPATCH( itamtoken, (create)(issue)(transfer) )
#define EOSIO_DISPATCH_EX( TYPE, MEMBERS ) \
extern "C" { \
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
      if( code == receiver || code == name("itamstoreapp").value || code == name("itamstoredex").value ) { \
         switch( action ) { \
            EOSIO_DISPATCH_HELPER( TYPE, MEMBERS ) \
         } \
         /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
      } \
   } \
} \

EOSIO_DISPATCH_EX( itamtoken, (create)(issue)(transfer) )
