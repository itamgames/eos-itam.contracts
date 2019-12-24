#define PAY_CONTRACT itamgamespay
#define GAS_CONTRACT "itamgamesgas"

#if defined(MAINNET)
    #define DEX_CONTRACT itamstoredex
    #define DEX_CONTRACT_BETA itamtestsdex
    #define APP_CONTRACT itamstoreapp
    #define NFT_CONTRACT itamstorenft
    #define TOKEN_CONTRACT itamtokenadm
#elif defined(MAINNET_BETA)
    #define DEX_CONTRACT itamtestsdex
    #define APP_CONTRACT itamtestsapp
    #define NFT_CONTRACT itamtestsnft
    #define TOKEN_CONTRACT itamtokenadm
#elif defined(TESTNET)
    #define DEX_CONTRACT itamgamesdex
    #define DEX_CONTRACT_BETA itamtestsdex
    #define APP_CONTRACT itamgamesapp
    #define NFT_CONTRACT itamstorenft
    #define TOKEN_CONTRACT itamtokenaaa
#elif defined(TESTNET_BETA)
    #define DEX_CONTRACT itamtestsdex
    #define APP_CONTRACT itamtestsapp
    #define NFT_CONTRACT itamtestsnft
    #define TOKEN_CONTRACT itamtokenaaa
#endif