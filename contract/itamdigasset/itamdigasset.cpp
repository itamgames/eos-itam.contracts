#include "itamdigasset.hpp"

ACTION itamdigasset::create(name issuer, string name, uint64_t appId, string itemStructs)
{
    require_auth(_self);

    symbol gameSymbol(name, 0);
    
    eosio_assert(gameSymbol.is_valid(), "Invalid symbol");
    eosio_assert(is_account(issuer), "Invalid issuer");

    currencyTable currencies(_self, _self.value);
    const auto& currency = currencies.find(gameSymbol.code().raw());
    eosio_assert(currency == currencies.end(), "Already token created");

    auto parsedItemStructs = json::parse(itemStructs);

    currencies.emplace(_self, [&](auto &c) {
        c.issuer = issuer;
        c.supply = asset(0, gameSymbol);
        c.appId = appId;

        for(int i = 0; i < parsedItemStructs.size(); i++)
        {
            auto parsedItemStruct = parsedItemStructs[i];
            string category = parsedItemStruct["category"];
            vector<string> fields = parsedItemStruct["fields"];

            c.itemStructs.push_back(itemStruct{category, fields});
        }
    });
}

ACTION itamdigasset::issue(name to, asset quantity, uint64_t itemId, string itemName, bool fungible, string itemDetail, string memo)
{
    eosio_assert(quantity.amount > 0, "must issue positive quantity");
    eosio_assert(!(fungible == false && quantity.amount > 1), "invalid fungible value");
    eosio_assert(quantity.symbol.precision() == 0, "precision must be set 0");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    const auto& currency = currencies.require_find(quantity.symbol.code().raw(), "invalid symbol");
    eosio_assert(has_auth(currency->issuer) || has_auth(name("itamstoreapp")), "Not allowed account");

    currencies.modify(currency, _self, [&](auto &c) {
        c.supply += quantity;
    });

    validateItemDetail(currency->itemStructs, itemDetail);
    addBalance(currency->issuer, currency->issuer, quantity, itemId, itemName, fungible, itemDetail);

    if(to != currency->issuer)
    {
        SEND_INLINE_ACTION(*this, transfer, {currency->issuer,"active"_n}, {currency->issuer, to, quantity, itemId, memo});
    }
}

ACTION itamdigasset::transfer(name from, name to, asset quantity, uint64_t itemId, string memo)
{
    require_auth(from);

    eosio_assert(from != to, "cannot transfer to self");
    eosio_assert(is_account(to), "to account does not exist");
    eosio_assert(quantity.amount > 0, "must transfer positive quantity");
    eosio_assert(quantity.symbol.precision() == 0, "precision must be set 0");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    accountTable accounts(_self, from.value);
    const auto& fromAccount = accounts.require_find(quantity.symbol.code().raw(), "not enough balance");
    
    map<uint64_t, item> items = fromAccount->items;
    item fromItem = items[itemId];
    
    addBalance(to, from, quantity, itemId, fromItem.itemName, fromItem.fungible, fromItem.option);
    subBalance(from, quantity, itemId);
    
    require_recipient(from);
    require_recipient(to);
}

ACTION itamdigasset::burn(name user, asset quantity, uint64_t itemId)
{
    require_auth(_self);
    eosio_assert(quantity.amount > 0, "quantity must be positive");
    subBalance(user, quantity, itemId);
}

void itamdigasset::addBalance(name owner, name ramPayer, asset quantity, uint64_t itemId, const string& itemName, bool fungible, const string& data)
{
    accountTable accounts(_self, owner.value);
    const auto& account = accounts.find(quantity.symbol.code().raw());

    bool isAlreadyItem = account->items.count(itemId) > 0;

    eosio_assert(!(fungible == false && isAlreadyItem), "Already issued item id");
    item i { itemName, fungible, (uint64_t)quantity.amount, data };

    if(account == accounts.end())
    {
        accounts.emplace(ramPayer, [&](auto &a) {
            a.balance = quantity;
            a.items[itemId] = i;
        });
    }
    else
    {
        accounts.modify(account, ramPayer, [&](auto &a) {
            a.balance += quantity;

            if(isAlreadyItem)
            {
                a.items[itemId].count += quantity.amount;
            }
            else
            {
                a.items[itemId] = i;
            }
        });
    }
}

void itamdigasset::subBalance(name to, asset quantity, uint64_t itemId)
{
    accountTable accounts(_self, to.value);
    const auto& account = accounts.require_find(quantity.symbol.code().raw(), "not enough balance");
    map<uint64_t, item> items = account->items;
    
    eosio_assert(items.count(itemId) > 0, "no item object found");
    eosio_assert(items[itemId].count >= quantity.amount, "quantity must below than items count");

    accounts.modify(account, _self, [&](auto &a) {
        a.balance -= quantity;
        if(a.items.count(itemId) > 0 && a.items[itemId].count - quantity.amount == 0)
        {
            a.items.erase(itemId);
        }
        else
        {
            a.items[itemId].count -= quantity.amount;
        }
    });
}

void itamdigasset::validateItemDetail(const vector<itemStruct>& itemStructs, const string& itemDetail)
{
    auto parsedItemDetail = json::parse(itemDetail);

    string category = parsedItemDetail["category"];
    auto requestedFields = parsedItemDetail["fields"];
    vector<string> requireFields = getItemStruct(itemStructs, category);

    eosio_assert(requestedFields.size() == requireFields.size(), "invalid item detail");
    for(auto iter = requireFields.begin(); iter != requireFields.end(); iter++)
    {
        eosio_assert(requestedFields.count(*iter) > 0, "invalid item detail");
    }
}

vector<string> itamdigasset::getItemStruct(const vector<itemStruct>& itemStructs, const string& category)
{
    for(auto iter = itemStructs.begin(); iter != itemStructs.end(); iter++)
    {
        if(iter->category == category)
        {
            return iter->fields;
        }
    }

    eosio_assert(false, "invalid category");
    
    vector<string> temp;
    return temp;
}