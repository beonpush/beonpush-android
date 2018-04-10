//
//  SWalletTransaction.hpp
//  beonpushd
//
//  Created by Sacha Vandamme on 15/02/2017.
//  Copyright © 2017 Sacha Vandamme. All rights reserved.
//

#ifndef SWalletTransaction_hpp
#define SWalletTransaction_hpp

#include <boost/unordered_map.hpp>
#include <map>
#include "bitcoinrpc.h"
#include "main.h"
#include "base58.h"

using namespace json_spirit;
using namespace std;

class SKeyTransaction
{
public:
    
    SKeyTransaction();
    Object ToObject() const;
    unsigned int Confirmations() const;
    
    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(hash);
        READWRITE(n);
        READWRITE(amount);
        READWRITE(is_vout);
        READWRITE(blockhash);
        READWRITE(blocktime);
        READWRITE(blockheight);
    )

public:
    
    int nVersion;
    uint256 hash;
    unsigned int n; //transaction n
    int64_t amount;
    bool is_vout;

    uint256 blockhash;
    unsigned int blocktime;
    unsigned int blockheight;
    
};

typedef map<uint256, SKeyTransaction> SKeyTransactionMap;

#endif /* SWalletTransaction_hpp */
