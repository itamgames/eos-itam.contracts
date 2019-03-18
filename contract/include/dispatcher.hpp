#define EOSIO_DISPATCH_EX( TYPE, MEMBERS ) \
extern "C" { \
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
        bool isAllowedTransfer = code == name("eosio.token").value; \
        if( code == receiver || isAllowedTransfer ) { \
            if( action == name("transfer").value ) { \
                eosio_assert(isAllowedTransfer, "only eosio.token can call internal transfer"); \
            } \
            switch( action ) { \
                EOSIO_DISPATCH_HELPER( TYPE, MEMBERS ) \
            } \
        } \
    } \
}