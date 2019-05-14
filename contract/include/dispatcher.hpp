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

#define ALLOW_TRANSFER_ALL_DISPATCHER( TYPE, MEMBERS ) \
extern "C" { \
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
        if ( code == receiver || action == name("transfer").value ) { \
            switch( action ) { \
                EOSIO_DISPATCH_HELPER( TYPE, MEMBERS ) \
            } \
        } \
    } \
}

#define ALLOW_TRANSFER_ITAM_EOS_DISPATCHER( TYPE, MEMBERS, TRANSFER_METHOD ) \
extern "C" { \
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
        if( action == name("transfer").value ) { \
            eosio_assert( code == name("eosio.token").value || code == name("itamtokenadm").value, "eosio.token or itamtokenadm can call transfer" ); \
            execute_action( name(receiver), name(code), TRANSFER_METHOD ); \
        } \
        else if ( code == receiver ) { \
            switch( action ) { \
                EOSIO_DISPATCH_HELPER( TYPE, MEMBERS ) \
            } \
        } \
    } \
}