//
//  SWalletKey.cpp
//  beonpushd
//
//  Created by Sacha Vandamme on 15/02/2017.
//  Copyright © 2017 Sacha Vandamme. All rights reserved.
//

#include "SWalletKey.hpp"

SWalletKey::SWalletKey(char *addr)
{
    this->addr = addr;
}

void SWalletKey::AddTransaction(SWalletTransaction *transaction)
{
    //Remove from unspents
    //Update balance
}

size_t SWalletKey::GetMemoryUsage()
{
    return sizeof(SWalletKey) + (sizeof(SWalletTransaction) * transactions.size());
}

std::string SWalletKey::GetAddress()
{
    return addr;
}

SWalletTransactionMap GetTransactions()
{
    return transactions;
}

SWalletTransactionMap GetUnspents()
{
    return unspents;
}

size_t GetBalance(int min_confirm=0)
{
    
}
