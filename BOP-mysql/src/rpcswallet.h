//
//  rpcswallet.h
//  beonpushd
//
//  Created by Sacha Vandamme on 19/02/2017.
//  Copyright © 2017 Sacha Vandamme. All rights reserved.
//

#ifndef rpcswallet_h
#define rpcswallet_h

#include "bitcoinrpc.h"
#include "main.h"
#include "base58.h"
#include "MySQLConnection.h"

#include <boost/fusion/container/list.hpp>
#include <boost/unordered_map.hpp>
#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

typedef int64_t CAmount;
using namespace active911;

class MysqlWallet;

struct WTransaction
{
    WTransaction()
    {
        txid = 0;
        n = 0;
        amount = 0;
        is_vout = false;
        blockhash = 0;
        blocktime = 0;
        blockheight = 0;
    }
    
    uint256 txid;
    unsigned int n; //transaction n
    int64_t amount;
    bool is_vout;

    uint256 blockhash;
    unsigned int blocktime;
    unsigned int blockheight;
    
    void loadTxId(std::string address, std::string txid, bool onlyOut = false);
    void loadTransaction(std::string address, CTransaction tx, bool onlyOut = false);
    static uint256 touint(std::string);
    
    int confirmations() const
    {
        return nBestHeight - blockheight;
    }
    
    json_spirit::Object toObject() const
    {
        json_spirit::Object result;
        
        result.push_back(json_spirit::Pair("block_hash", blockhash.GetHex()));
        result.push_back(json_spirit::Pair("block_time", (int64_t) blocktime));
        result.push_back(json_spirit::Pair("block_height", (int) blockheight));
        result.push_back(json_spirit::Pair("txid", txid.GetHex()));
        result.push_back(json_spirit::Pair(is_vout ? "vout" : "vin",  (int) n));
        result.push_back(json_spirit::Pair("amount", amount));
        result.push_back(json_spirit::Pair("confirmations", (int) confirmations()));
        
        return result;
    }
};

typedef list<WTransaction> WTransactionList;

class WAddress
{
public:
    WAddress();
    
    CAmount getBalance(unsigned int min_confirm=0);
    WTransactionList getTransactions(unsigned int start, unsigned int limit, unsigned int min_confirm=0);
    WTransactionList getUnspents(unsigned int start, unsigned int limit, unsigned int min_confirm=0);

protected:
    sql::ResultSet *queryTxStatement(std::string additional, unsigned int start, unsigned int limit, unsigned int min_confirm);
    //WTransaction transformResult();
    
public:
    std::string address;
    MysqlWallet *wallet;
};

typedef boost::unordered_map<std::string, WAddress> WAddressMap;


class MysqlWallet
{
public:
    MysqlWallet();

    boost::shared_ptr<MySQLConnection> getConnection();
    void unborrow(boost::shared_ptr<MySQLConnection>);

    sql::Statement *createStatement(boost::shared_ptr<MySQLConnection>);
    sql::PreparedStatement *prepareStatement(boost::shared_ptr<MySQLConnection>, std::string sql);
    
    WAddressMap getAddresses();
    WAddress getAddress(std::string address);
    
    void importNewBlocks();
    void importBlocksThread();

protected:
    void connect();

    unsigned int loadedBlocks(boost::shared_ptr<MySQLConnection>);
    void setLoadedBlocks(boost::shared_ptr<MySQLConnection>, unsigned int loadedBlocks);
    void importBlock(boost::shared_ptr<MySQLConnection>, CBlock);
    void importTransaction(boost::shared_ptr<MySQLConnection>, list<std::string> &sql, CBlock block, CTransaction tx);
    
private:
    
    boost::shared_ptr<ConnectionPool<MySQLConnection> > mysql_pool;
    bool poolInitialised;

    WAddressMap addresses;
    bool isImporting;
};

extern MysqlWallet mysqlWallet;



#endif /* rpcswallet_h */
