//
//  SWallet.cpp
//  beonpushd
//
//  Created by Sacha Vandamme on 15/02/2017.
//  Copyright © 2017 Sacha Vandamme. All rights reserved.
//

#include "SWallet.hpp"

swallet = new SWallet();

SWallet::SWallet()
{
    mainPartition = new SWalletPartition("mainPartition");
}

SWalletKey *SWallet::GetKey(char *addr)
{
    return GetPartition(addr)->GetKey(addr);
}

SWalletPartition *SWallet::GetPartition(char *addr)
{
    return mainPartition;
}

//SWalletTransaction GetTransaction(char *hash);

size_t SWallet::GetMemoryUsage()
{
    return mainPartition.GetMemoryUsage() + sizeof(SWallet);
}


void SWallet::ImportBlocks()
{
    //..
}
