//
//  SWalletPartition.hpp
//  beonpushd
//
//  Created by Sacha Vandamme on 15/02/2017.
//  Copyright © 2017 Sacha Vandamme. All rights reserved.
//

#ifndef SWalletPartition_hpp
#define SWalletPartition_hpp

#include <stdio.h>
#include "SWalletKey.hpp"

class SWalletPartition
{
    
public:
    SWalletPartition(char *name);
    
    SWalletKey *GetKey(char *addr);

    size_t GetMemoryUsage();
    
    //AddTransaction(SWalletTransaction *);
    
protected:
    void Load();
    void Save();

    //Keys
    
};

#endif /* SWalletPartition_hpp */
