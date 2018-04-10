//
//  rpcwebwallet.h
//  beonpushd
//
//  Created by Sacha Vandamme on 15/02/2017.
//  Copyright © 2017 Sacha Vandamme. All rights reserved.
//

#ifndef rpcwebwallet_h
#define rpcwebwallet_h

#include <boost/unordered_map.hpp>
#include <map>
#include "bitcoinrpc.h"
#include "main.h"
#include "base58.h"

#include "SKeyTransaction.hpp"

using namespace json_spirit;
using namespace std;


//------------------------------------------------------------------------------------


class PKeySummary
{
public:
    PKeySummary(std::string addr);
    ~PKeySummary();
    
    std::string GetAddress();
    SKeyTransactionMap GetTransactions();
    SKeyTransactionMap GetUnspent();
    Value GetBalance(int min_confirm=0, bool using_retain = true);
    size_t GetMemoryUsage();
    bool CanBeRemoved();
    unsigned int LoadedBlocks();
    
protected:
    void Using();
    void ImportBlock(CBlock block, CBlockIndex *index);
    
    std::string strAddress;
    unsigned int used_time;
    unsigned int loaded_blocks;
    
    SKeyTransactionMap transactions;
    SKeyTransactionMap unspent;
    size_t cache_balance;
};

typedef boost::unordered_map<std::string, PKeySummary*> PKeySummaryMap;
typedef boost::unordered_map<int, CBlock> BlocksMap;


//------------------------------------------------------------------------------------

class WebWallet
{
public:
    WebWallet();
    Value GetTransactions(std::string strAddr, int start=0, int limit=10, unsigned int min_conf=0);
    Value GetUnspent(std::string strAddr, int start=0, int limit=10, unsigned int min_conf=0);
    Value GetBalance(std::string strAddr, int min_conf=0);
    PKeySummary *Summary(std::string strAddr);
    PKeySummaryMap Keys();
    size_t GetMemoryUsage();
    
    //--------
    
    CBlock getBlock(int height);

protected:
    void AutoMemoryThread();

    PKeySummaryMap KeysCache;
    BlocksMap blocksCache;
    int cacheLoadedBlocks;
    
};



#endif /* rpcwebwallet_h */
