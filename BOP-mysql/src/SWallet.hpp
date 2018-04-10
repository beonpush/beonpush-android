//
//  SWallet.hpp
//  beonpushd
//
//  Created by Sacha Vandamme on 15/02/2017.
//  Copyright © 2017 Sacha Vandamme. All rights reserved.
//

#ifndef SWallet_hpp
#define SWallet_hpp

#include <stdio.h>
#include "SWalletPartition.hpp"

class SWallet
{
public:
    SWallet();
    
    SWalletKey *GetKey(char *addr);
    SWalletPartition *GetPartition(char *addr);
    
    //SWalletTransaction GetTransaction(char *hash);
    
    size_t GetMemoryUsage();
    
    //RebuildPartitions();

    //GetKeys();
    //Value GetTransactions(std::string strAddr, int start=0, int limit=10, unsigned int min_conf=0);
    //Value GetUnspent(std::string strAddr, int start=0, int limit=10, unsigned int min_conf=0);
    //Value GetBalance(std::string strAddr, int min_conf=0);
    
protected:
    void ImportBlocks();
    
private:
    unsigned int loadedBlocks;
    SWalletPartition *mainPartition;
    
};

extern SWallet *swallet;

#endif /* SWallet_hpp */
