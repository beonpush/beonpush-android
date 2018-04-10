//
//  SWalletKey.hpp
//  beonpushd
//
//  Created by Sacha Vandamme on 15/02/2017.
//  Copyright © 2017 Sacha Vandamme. All rights reserved.
//

#ifndef SWalletKey_hpp
#define SWalletKey_hpp

#include <stdio.h>
#include "SKeyTransaction.hpp"


class SWalletKey
{
public:
    SWalletKey(char *addr);
    void AddTransaction(SWalletTransaction *transaction);

    size_t GetMemoryUsage();
    std::string GetAddress();

    SKeyTransactionMap GetTransactions();
    SKeyTransactionMap GetUnspents();
    size_t GetBalance(int min_confirm=0);

    IMPLEMENT_SERIALIZE
    (
        nVersion = 1;
        READWRITE(addr);
        READWRITE(cache_balance);
        READWRITE(unspents); //ToDo recalculate unspents & cachebalance ! & check unspents the where removed from mempool but are still not spend after restart ..?
        READWRITE(transactions);
    )

    //ToDo: Get only unspents and last 10-20 transactions, load all the transactions only if needed

    //ReadFromDisk..
    //SaveToDisk..
    //CanBeRemoved() ? Check if the key is still in use or can be removed of the ram

private:
    SKeyTransactionMap transactions;
    SKeyTransactionMap unspents;
    
    std::string addr;
    size_t cache_balance;
};

#endif /* SWalletKey_hpp */
