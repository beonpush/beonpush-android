//
//  SWalletTransaction.cpp
//  beonpushd
//
//  Created by Sacha Vandamme on 15/02/2017.
//  Copyright © 2017 Sacha Vandamme. All rights reserved.
//

#include "SKeyTransaction.hpp"

SKeyTransaction::SKeyTransaction()
{
    nVersion = 1;
}

Object SKeyTransaction::ToObject() const
{
    Object result;
    
    result.push_back(Pair("block_hash", blockhash.GetHex()));
    result.push_back(Pair("block_time", (int64_t) blocktime));
    result.push_back(Pair("block_height", (int) blockheight));
    result.push_back(Pair("txid", hash.GetHex()));
    result.push_back(Pair(is_vout ? "vout" : "vin",  (int) n));
    result.push_back(Pair("amount", amount));
    result.push_back(Pair("confirmations", (int) Confirmations()));

    return result;
}

unsigned int SKeyTransaction::Confirmations() const
{
    return nBestHeight - blockheight;
}
