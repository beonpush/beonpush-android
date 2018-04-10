//
//  rpcwebwallet.cpp
//  fameshares
//
//  Created by Sacha Vandamme on 14/04/16.
//  Copyright © 2016 Sacha Vandamme. All rights reserved.
//

#include "rpcwebwallet.h"

using namespace json_spirit;
using namespace std;

#define webwallet_memory_time 60 * 48

static WebWallet uni_wallet;


PKeySummary::PKeySummary(std::string addr)
{
    strAddress = addr;
    loaded_blocks = 0;
    cache_balance = 0;
    Using();
}

PKeySummary::~PKeySummary()
{
    //ToDo
}

std::string PKeySummary::GetAddress()
{
    return strAddress;
}

SKeyTransactionMap PKeySummary::GetTransactions()
{
    Using();
    return transactions;
}

SKeyTransactionMap PKeySummary::GetUnspent()
{
    Using();
    return unspent;
}

Value PKeySummary::GetBalance(int min_confirm, bool using_retain)
{
    if(using_retain)
        Using();
    
    std::ostringstream oss;
    if(min_confirm <= 0)
    {
        oss << cache_balance;
        return Value(oss.str());
    }
    
    int64_t balance = 0;
    BOOST_FOREACH(const SKeyTransactionMap::value_type &v, unspent)
    {
        if(v.second.Confirmations() >= min_confirm)
        {
            balance += v.second.amount;
        }
    }
    
    oss << balance;
    return Value(oss.str());
}

size_t PKeySummary::GetMemoryUsage()
{
    return sizeof(PKeySummary) + (sizeof(SKeyTransaction) * transactions.size())
    + (sizeof(std::string) * transactions.size())
    + (sizeof(SKeyTransaction) * unspent.size())
    + (sizeof(std::string) * unspent.size());
}

bool PKeySummary::CanBeRemoved()
{
    return (GetTime() - used_time) > (webwallet_memory_time * 60);
}

unsigned int PKeySummary::LoadedBlocks()
{
    return loaded_blocks;
}

void PKeySummary::Using()
{
    used_time = GetTime();
    
    if(nBestHeight > loaded_blocks+1)
    {
        //Update unspents confirmations
        /*unsigned int add_conf = nBestHeight + 1 - loaded_blocks;
        BOOST_FOREACH(PKeyTransactionMap::value_type &v, unspent)
        {
            v.second.confirmations += add_conf;
        }
        
        BOOST_FOREACH(PKeyTransactionMap::value_type &v, transactions)
        {
            v.second.confirmations += add_conf;
        }*/
        
        //Import new block
        //CBlockIndex* bestBlock = mapBlockIndex[hashBestChain];;
        //CBlockIndex* pblockindex;
        
        for(int i=loaded_blocks; i <= nBestHeight; i++)
        {
            CBlock block = uni_wallet.getBlock(i);
            ImportBlock(block, mapBlockIndex[block.GetHash()]);
            loaded_blocks = i;

            /*CBlock block;
             pblockindex = bestBlock;
             
             while (pblockindex->nHeight > i)
             pblockindex = pblockindex->pprev;
             
             if(block.ReadFromDisk(pblockindex, true))
             {
             loaded_blocks = i;
             ImportBlock(block, pblockindex);
             }*/
        }
        
        if(loaded_blocks > nBestHeight)
        {
            loaded_blocks = nBestHeight+1;
        }
    }
    
    //Remove mempool transactions from unspended list
    typedef map<uint256, CTransaction> txmap;
    BOOST_FOREACH(const txmap::value_type& e, mempool.mapTx)
    {
        CTransaction tx = e.second;
        BOOST_FOREACH(const CTxIn &txin, tx.vin)
        {
            CHashWriter writer(SER_GETHASH, 0);
            writer << txin.prevout.hash << txin.prevout.n;
            uint256 xhash       = writer.GetHash();
            
            if(unspent.count(xhash))
            {
                cache_balance -= unspent[xhash].amount;
                unspent.erase(xhash);
            }
        }
    }
}

void PKeySummary::ImportBlock(CBlock block, CBlockIndex *index)
{
    BOOST_FOREACH(const CTransaction &tx, block.vtx)
    {
        unsigned int n = 0;
        BOOST_FOREACH(const CTxIn &txin, tx.vin)
        {
            if(txin.prevout.hash.ToString() == "0000000000000000000000000000000000000000000000000000000000000000")
            {
                continue;
            }
            
            CHashWriter writer(SER_GETHASH, 0);
            writer << txin.prevout.hash << txin.prevout.n;
            uint256 xhash       = writer.GetHash();
            
            if(unspent.count(xhash))
            {
                SKeyTransaction transaction;
                cache_balance -= unspent[xhash].amount;
                
                transaction.blockhash = *(index->phashBlock);
                transaction.blocktime = block.nTime;
                transaction.blockheight = index->nHeight;
                transaction.hash    = tx.GetHash();
                transaction.is_vout = false;
                transaction.n       = n;
                transaction.amount  = -unspent[xhash].amount;
                
                unspent.erase(xhash);
                writer << 'I';
                transactions[writer.GetHash()] = transaction;
            }
            n++;
        }
        
        n = 0;
        BOOST_FOREACH(const CTxOut &txout, tx.vout)
        {
            txnouttype type;
            std::vector<CTxDestination> addresses;
            int nRequired;
            
            if (!ExtractDestinations(txout.scriptPubKey, type, addresses, nRequired))
            {
                continue;
            }
            
            BOOST_FOREACH(const CTxDestination& addr, addresses)
            {
#warning verify transaction amount is correct with multiple addresses
                if(CBitcoinAddress(addr).ToString() == strAddress)
                {
                    cache_balance += txout.nValue;
                    SKeyTransaction transaction;
                    
                    transaction.blockhash = *(index->phashBlock);
                    transaction.blocktime = block.nTime;
                    transaction.blockheight = index->nHeight;
                    transaction.hash    = tx.GetHash();
                    transaction.is_vout = true;
                    transaction.n       = n;
                    transaction.amount  = txout.nValue;
                    
                    //Add to transactions & unspents
                    CHashWriter writer(SER_GETHASH, 0);
                    writer << transaction.hash << transaction.n;
                    uint256 xhash       = writer.GetHash();
                    
                    transactions[xhash] = transaction;
                    unspent[xhash]      = transaction;
                }
            }
            
            n++;
        }
    }
}

//------------------------------------------------------------------------------------


WebWallet::WebWallet()
{
    cacheLoadedBlocks = 0;
    boost::thread(boost::bind(&WebWallet::AutoMemoryThread, this));
}

Value WebWallet::GetTransactions(std::string strAddr, int start, int limit, unsigned int min_conf)
{
    SKeyTransactionMap map = Summary(strAddr)->GetTransactions();
    
    Array result;
    int i = 0, x = 0;
    BOOST_FOREACH(const SKeyTransactionMap::value_type &v, map)
    {
        if(v.second.Confirmations() >= min_conf)
        {
            if(i >= start)
            {
                result.push_back(v.second.ToObject());
                
                x++;
                if(x >= limit)
                {
                    break;
                }
            }
            
            i++;
        }
    }
    
    return result;
}

Value WebWallet::GetUnspent(std::string strAddr, int start, int limit, unsigned int min_conf)
{
    SKeyTransactionMap map = Summary(strAddr)->GetUnspent();
    
    Array result;
    int i = 0, x = 0;
    BOOST_FOREACH(const SKeyTransactionMap::value_type &v, map)
    {
        if(i >= start)
        {
            if(v.second.Confirmations() >= min_conf/* && v.second.spended == false*/)
            {
                result.push_back(v.second.ToObject());
                
                x++;
                if(x >= limit)
                {
                    break;
                }
            }
        }
    }
    
    return result;
}

Value WebWallet::GetBalance(std::string strAddr, int min_conf)
{
    return Summary(strAddr)->GetBalance(min_conf);
}

PKeySummary *WebWallet::Summary(std::string strAddr)
{
    if(!KeysCache.count(strAddr))
    {
        KeysCache[strAddr] = new PKeySummary(strAddr);
    }
    
    return KeysCache[strAddr];
}

PKeySummaryMap WebWallet::Keys()
{
    return KeysCache;
}

size_t WebWallet::GetMemoryUsage()
{
    size_t size = sizeof(WebWallet);
    BOOST_FOREACH(const PKeySummaryMap::value_type &v, KeysCache)
    {
        size += v.second->GetMemoryUsage() + sizeof(v.first);
    }
    
    return size;
}

//--------

CBlock WebWallet::getBlock(int height)
{
    if(blocksCache.count(height))
        return blocksCache[height];
    
    CBlockIndex* pblockindex = mapBlockIndex[hashBestChain];
    int nHeight = pblockindex->nHeight;
    
    if(pblockindex == NULL)
    {
        return CBlock();
    }
    
    while(pblockindex->nHeight >= cacheLoadedBlocks)
    {
        CBlock block;
        if(block.ReadFromDisk(pblockindex, true))
        {
            blocksCache[pblockindex->nHeight] = block;
        }
        else
        {
            return block;
        }
        
        pblockindex = pblockindex->pprev;
        
        if(pblockindex == NULL)
            break;
    }
    
    cacheLoadedBlocks = nHeight;
    
    if(blocksCache.count(height))
        return blocksCache[height];

    return CBlock();
}

void WebWallet::AutoMemoryThread()
{
    boost::posix_time::time_duration interval(boost::posix_time::seconds(15));
    while(1)
    {
        boost::this_thread::sleep(interval);
        
        BOOST_FOREACH(const PKeySummaryMap::value_type &v, KeysCache)
        {
            if(v.second->CanBeRemoved())
            {
                KeysCache.erase(v.first);
            }
        }
    }
}

//------------------------------------------------------------------------------------


/*Value publickey_balance(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
                            "publickey_balance (\"address\")\n"
                            "\n....\n"
                            "\nArguments:\n"
                            "1. \"address\"    (string, required) The public key that will be watched.\n");
    
    return uni_wallet.GetBalance(params[0].get_str());
}*/

/*Value publickey_transactions(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 4)
        throw runtime_error(
                            "publickey_transactions ( \"address\" count from)\n"
                            "\n....\n"
                            "\nArguments:\n"
                            "1. \"address\"    (string, required) The public key that will be watched.\n"
                            "2. count          (numeric, optional, default=10) The number of transactions to return\n"
                            "3. from           (numeric, optional, default=0) The number of transactions to skip\n"
                            "4. min_conf           (numeric, optional, default=0) The minimum number of confirmations\n");
    
     int count = (params.size() > 1 ? params[1].get_int() : 10);
     int start = (params.size() > 2 ? params[2].get_int() : 0);
     int min_confirm = (params.size() > 3 ? params[3].get_int() : 0);
    
    if(min_confirm < 0)
        throw runtime_error("Confirmations cannot be less than 0");
    
     std::string address    = params[0].get_str();
     
     return uni_wallet.GetTransactions(address, start, count, min_confirm);
}

Value publickey_unspents(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 4)
        throw runtime_error(
                            "publickey_transactions ( \"address\" count from)\n"
                            "\n....\n"
                            "\nArguments:\n"
                            "1. \"address\"    (string, required) The public key that will be watched.\n"
                            "2. count          (numeric, optional, default=10) The number of transactions to return\n"
                            "3. from           (numeric, optional, default=0) The number of transactions to skip\n"
                            "4. min_conf           (numeric, optional, default=0) The minimum number of confirmations\n");
    
    int count = (params.size() > 1 ? params[1].get_int() : 10);
    int start = (params.size() > 2 ? params[2].get_int() : 0);
    int min_confirm = (params.size() > 3 ? params[3].get_int() : 0);
    
    if(min_confirm < 0)
        throw runtime_error("Confirmations cannot be less than 0");

    return uni_wallet.GetUnspent(params[0].get_str(), start, count, min_confirm);
}*/

/*Value publickeys_cache(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
                            "publickeys_cache\n"
                            "\n....\n");
    
    Object result;
    result.push_back(Pair("KeysCount", (int) uni_wallet.Keys().size()));
    result.push_back(Pair("CacheSize", (int) uni_wallet.GetMemoryUsage()));
    
    Object keys;
    BOOST_FOREACH(const PKeySummaryMap::value_type &v, uni_wallet.Keys())
    {
        keys.push_back(Pair(v.first, v.second->GetBalance(0, false)));
    }
    
    result.push_back(Pair("KeyBalances", keys));
    return result;
}*/
