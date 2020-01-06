// 1 day == 24 hours == 1440 minutes == 86400 seconds
#ifdef testnet
    const int SECONDS_OF_DAY = 60;
#else
    const int SECONDS_OF_DAY = 86400;
#endif