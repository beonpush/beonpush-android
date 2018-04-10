//
//  rpcswallet.cpp
//  beonpushd
//
//  Created by Sacha Vandamme on 19/02/2017.
//  Copyright © 2017 Sacha Vandamme. All rights reserved.
//


#include "rpcswallet.h"
#include "util.h"

#include <string>
#include <boost/shared_ptr.hpp>

using namespace json_spirit;
using namespace std;

MysqlWallet mysqlWallet;

//------------------------------------------------

MysqlWallet::MysqlWallet():
    poolInitialised(false),
    isImporting(false)
{

}

void StartShutdown();

boost::shared_ptr<MySQLConnection> MysqlWallet::getConnection()
{
    if(!poolInitialised)
    {
        std::string host = GetArg("-dbhost", "");
        std::string user = GetArg("-dbuser", "");
        std::string pass = GetArg("-dbpass", "");
        std::string name = GetArg("-dbname", "");
        
        if (host == "" || user == "" || name == "")
        {
            std::cout << "Please specify the database informations (dbhost, dbuser, dbpass, dbname) in " << GetDataDir() << "\n";
            StartShutdown();
            throw;
        }
        
        //Max 30 Simultanious connections
        boost::shared_ptr<MySQLConnectionFactory>mysql_connection_factory(new MySQLConnectionFactory(host, user, pass, name));
        mysql_pool = boost::shared_ptr<ConnectionPool<MySQLConnection> >(new ConnectionPool<MySQLConnection>(30, mysql_connection_factory));
        
        poolInitialised = true;
    }
    
    return mysql_pool->borrow();
}

void MysqlWallet::unborrow(boost::shared_ptr<MySQLConnection> conn)
{
    mysql_pool->unborrow(conn);
}

sql::Statement *MysqlWallet::createStatement(boost::shared_ptr<MySQLConnection> conn)
{
    //if(conn == NULL)
    //    throw JSONRPCError(RPC_DATABASE_ERROR, "The daemon could not connect to the database");

    try
    {
        return conn->sql_connection->createStatement();
    }
    catch (sql::SQLException &e) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "The daemon could not create the statement");
    }
    catch(...)
    {
        throw JSONRPCError(RPC_DATABASE_ERROR, "An unknown error occurd while creating the statement");
    }
    
    return NULL;
}

sql::PreparedStatement *MysqlWallet::prepareStatement(boost::shared_ptr<MySQLConnection> conn, std::string sql)
{
    //if(conn == NULL)
    //    throw JSONRPCError(RPC_DATABASE_ERROR, "The daemon could not connect to the database");

    try
    {
        return conn->sql_connection->prepareStatement(sql);
    }
    catch (sql::SQLException &e) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "The daemon could not prepare the statement");
    }
    catch(...)
    {
        throw JSONRPCError(RPC_DATABASE_ERROR, "An unknown error occurd while preparing the statement");
    }

    return NULL;
}

WAddressMap MysqlWallet::getAddresses()
{
    return addresses;
}

WAddress MysqlWallet::getAddress(std::string address)
{
    CBitcoinAddress addressvalidator(address);
    if(!addressvalidator.IsValid())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid address");
    
    if(!addresses.count(address))
    {
        WAddress waddr;
        waddr.address = address;
        waddr.wallet  = this;
        addresses[address] = waddr;
    }
    
    return addresses[address];
}

unsigned int MysqlWallet::loadedBlocks(boost::shared_ptr<MySQLConnection> connection)
{
    try
    {
        sql::PreparedStatement *stmt = prepareStatement(connection, "SELECT loaded_blocks FROM `global` LIMIT 1");
        sql::ResultSet *res = stmt->executeQuery();
        if(res->next())
        {
            delete stmt;
            unsigned int balance = res->getUInt("loaded_blocks");
            delete res;
            return balance;
        }
        
        delete res;
        delete stmt;
    }
    catch(sql::SQLException &e)
    {
        //ToDo log exception
    }
    
    return 0;
}

void MysqlWallet::setLoadedBlocks(boost::shared_ptr<MySQLConnection> conn, unsigned int loadedBlocks)
{
    try
    {
        sql::PreparedStatement *stmt = prepareStatement(conn, "UPDATE global SET loaded_blocks=?");
        stmt->setInt(1, loadedBlocks);
        sql::ResultSet *res = stmt->executeQuery();
        while(res->next())
        {
        }
        
        delete res;
        delete stmt;
    }
    catch(sql::SQLException &e)
    {
        //ToDo log exception
    }
}

void MysqlWallet::importBlocksThread()
{
    if(isImporting)
        return;

    isImporting = true;

    if (fShutdown)
        return;
    
    try
    {
        boost::shared_ptr<MySQLConnection> connection = getConnection();
        
        int loaded = loadedBlocks(connection)+1;
        CBlockIndex *pblockindex;
        
        for(;loaded <= nBestHeight; loaded++)
        {
            if (fShutdown)
                return;
            
            //Getblock
            CBlock block;
            pblockindex = mapBlockIndex[hashBestChain];
            
            while (pblockindex->nHeight > loaded)
                pblockindex = pblockindex->pprev;
            
            if(block.ReadFromDisk(pblockindex, true))
            {
                importBlock(connection, block);
                setLoadedBlocks(connection, loaded);
            }
        }
        
        mysql_pool->unborrow(connection);
    }
    catch (Object& objError)
    {
        std::cout << "An error occured in the blockimporter: JSON error detected: " << objError[1].value_.get_str() << "\n";
    }
    catch(exception &e)
    {
        cout << "An error occured in the blockimporter, Exception: " << e.what() << "\n";
    }
    catch(...)
    {
        cout << "An error occured in the blockimporter \n";
    }
    
    isImporting = false;
}

void staticBlocksImporter(void *)
{
    mysqlWallet.importBlocksThread();
}

void MysqlWallet::importNewBlocks()
{
    addresses.clear();
 
    if(GetBoolArg("-dbmaster", false) == false)
        return;
    
    if(isImporting)
        return;
    
    NewThread(staticBlocksImporter, NULL);
    
}

void handleSqlInserts(boost::shared_ptr<MySQLConnection> connection, list<std::string> &sqllist)
{
    if(sqllist.size() == 0)
        return;

    std::ostringstream oss;
    oss << "INSERT INTO `transactions` (`txid`, `address`, `block_height`, `amount`, `spend`) VALUES\n";
    
    bool comma = false;
    
    BOOST_FOREACH(const std::string &sql, sqllist)
    {
        if(comma)
            oss << ",\n";
        else
            comma = true;
        
        oss << sql;
    }

    sqllist.clear();
    
    try
    {
        sql::Statement *stmt = mysqlWallet.createStatement( connection );
        stmt->execute(oss.str());
        delete stmt;
    }
    catch(sql::SQLException &e)
    {
        if(e.getErrorCode() != 1062)
        {
            //insert failed
            cout << "# ERR: SQLException in " << __FILE__;
            cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
            cout << "# ERR: " << e.what();
            cout << " (MySQL error code: " << e.getErrorCode();
            cout << ", SQLState: " << e.getSQLState() << " )" << endl;
        }
    }
}

void MysqlWallet::importBlock(boost::shared_ptr<MySQLConnection> connection, CBlock block)
{
    list<std::string> sqlInserts;
    
    BOOST_FOREACH(const CTransaction &tx, block.vtx)
    {
        importTransaction(connection, sqlInserts, block, tx);
        
        if (fShutdown)
            return;
    }
    
    handleSqlInserts(connection, sqlInserts);
}

void MysqlWallet::importTransaction(boost::shared_ptr<MySQLConnection> connection, list<std::string> &sql, CBlock block, CTransaction tx)
{
    typedef boost::unordered_map<std::string, CAmount> TmpMap;
    TmpMap addressBalances;
    
    BOOST_FOREACH(const CTxIn &txin, tx.vin)
    {
        if(txin.prevout.hash == 0)
            continue;

        CTransaction txout;
        uint256 hashBlock = 0;
        if (GetTransaction(txin.prevout.hash, txout, hashBlock))
        {
            if(txout.vout.size() <= txin.prevout.n)
                continue;
            
            const CTxOut out = txout.vout[txin.prevout.n];
            
            txnouttype type;
            std::vector<CTxDestination> addresses;
            int nRequired;
            
            if (!ExtractDestinations(out.scriptPubKey, type, addresses, nRequired))
                continue;
            
            BOOST_FOREACH(const CTxDestination& addr, addresses)
            {
                std::string bitcoinaddress = CBitcoinAddress(addr).ToString();
                //Update map balance..
                
                if(addressBalances.count(bitcoinaddress))
                    addressBalances[bitcoinaddress] -= out.nValue;
                else
                    addressBalances[bitcoinaddress] = -out.nValue;
                
                //Update unspents..
                try
                {
                    std::ostringstream oss;
                    oss << "UPDATE transactions SET spend='1' WHERE txid='" << txin.prevout.hash.GetHex() << "' AND address='" << bitcoinaddress << "';\n";
                    
                    sql::Statement *stmt = mysqlWallet.createStatement(connection);
                    stmt->execute(oss.str());
                    delete stmt;
                }
                catch(sql::SQLException &e)
                {
                    //insert failed
                    cout << "# ERR: SQLException in " << __FILE__;
                    cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
                    cout << "# ERR: " << e.what();
                    cout << " (MySQL error code: " << e.getErrorCode();
                    cout << ", SQLState: " << e.getSQLState() << " )" << endl;
                }
            }
        }
        
        if (fShutdown)
            return;
    }
    
    //-----------------------------------------------------------------------------------------------------------------
    
    BOOST_FOREACH(const CTxOut &txout, tx.vout)
    {
        if (fShutdown)
            return;
        
        txnouttype type;
        std::vector<CTxDestination> addresses;
        int nRequired;
        
        if (!ExtractDestinations(txout.scriptPubKey, type, addresses, nRequired))
            continue;
        
        BOOST_FOREACH(const CTxDestination& addr, addresses)
        {
            std::string bitcoinaddress = CBitcoinAddress(addr).ToString();
            
            if(addressBalances.count(bitcoinaddress))
                addressBalances[bitcoinaddress] += txout.nValue;
            else
                addressBalances[bitcoinaddress] = txout.nValue;
        }
    }
    
    //-----------------------------------------------------------------------------------------------------------------
    
    CBlockIndex *index = mapBlockIndex[block.GetHash()];

    BOOST_FOREACH(const TmpMap::value_type &addr, addressBalances)
    {
        if (fShutdown)
            return;
        
        std::ostringstream oss;
        oss << "('" << tx.GetHash().GetHex() << "', '" << addr.first << "', " << index->nHeight << ", " << addr.second << ", '0')";
        
        sql.push_back(oss.str());
        
        if(sql.size() > 2000)
            handleSqlInserts(connection, sql);
    }
}


//------------------------------------------------


WAddress::WAddress()
{
    wallet = NULL;
}

CAmount WAddress::getBalance(unsigned int min_confirm)
{
    boost::shared_ptr<MySQLConnection> connection = wallet->getConnection();
    
    sql::PreparedStatement *stmt = wallet->prepareStatement(connection, (min_confirm == 0) ?
                        "SELECT balance FROM addresses WHERE address=? LIMIT 1" :
                        "SELECT COALESCE(SUM(amount), 0) as 'balance' FROM `transactions` WHERE address=? and block_height<=?");
    
    try
    {
        stmt->setString(1, address);
        
        if(min_confirm > 0)
        {
            stmt->setInt(2, nBestHeight-min_confirm);
        }
        
        sql::ResultSet *res = stmt->executeQuery();
        if(res->next())
        {
            CAmount balance = res->getInt64("balance");
            
            delete stmt;
            delete res;
            return balance;
        }
        
        delete res;
        wallet->unborrow(connection);
    }
    catch(sql::SQLException &e)
    {
        throw JSONRPCError(RPC_DATABASE_ERROR, e.what());
    }
    catch(...)
    {
        throw JSONRPCError(RPC_DATABASE_ERROR, "The daemon could not access to the database data");
    }
    
    delete stmt;
    return 0;
}

sql::ResultSet *WAddress::queryTxStatement(std::string additional, unsigned int start, unsigned int limit, unsigned int min_confirm)
{
    std::string sql = "SELECT txid, amount, block_height FROM `transactions` WHERE " + additional + " address=?";
    
    if(min_confirm > 0)
    {
        sql += " AND block_height<=?";
    }
    
    sql += " LIMIT ?, ?";
    
    boost::shared_ptr<MySQLConnection> connection = wallet->getConnection();
    sql::PreparedStatement *stmt = wallet->prepareStatement(connection, sql);
    
    try
    {
        stmt->setString(1, address);
        
        if(min_confirm > 0)
        {
            stmt->setInt(2, nBestHeight-min_confirm);
            stmt->setInt(3, start);
            stmt->setInt(4, limit);
        }
        else
        {
            stmt->setInt(2, start);
            stmt->setInt(3, limit);
        }
        
        sql::ResultSet *res = stmt->executeQuery();
        delete stmt;
        return res;
        
        wallet->unborrow(connection);
    }
    catch(...)
    {
        delete stmt;
        throw JSONRPCError(RPC_DATABASE_ERROR, "The daemon could not access to the database data");
    }
}

WTransactionList WAddress::getTransactions(unsigned int start, unsigned int limit, unsigned int min_confirm)
{
    WTransactionList list;
    sql::ResultSet *res = queryTxStatement("", start, limit, min_confirm);
    
    try
    {
        while(res->next())
        {
            std::string txid = res->getString("txid");
            WTransaction transaction;
            transaction.loadTxId(address, txid);
            
            if(transaction.blockhash != 0)
            {
                try
                {
                    if(transaction.amount != res->getInt64("amount") || transaction.blockheight != res->getInt("block_height"))
                    {
                        std::string sql = "UPDATE transactions SET block_height=?, amount=? WHERE txid=?";
                        
                        boost::shared_ptr<MySQLConnection> connection = wallet->getConnection();
                        sql::PreparedStatement *stmt = wallet->prepareStatement(connection, sql);
                        stmt->setUInt(1, transaction.blockheight);
                        stmt->setInt64(2, transaction.amount);
                        stmt->setString(3, transaction.txid.GetHex());
                        sql::ResultSet *res = stmt->executeQuery();
                        
                        while(res->next())
                        {
                        }
                        
                        delete stmt;
                        delete res;
                        wallet->unborrow(connection);
                    }
                }
                catch(sql::SQLException &e)
                {
                    //Update failed
                    /*cout << "# ERR: SQLException in " << __FILE__;
                    cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
                    cout << "# ERR: " << e.what();
                    cout << " (MySQL error code: " << e.getErrorCode();
                    cout << ", SQLState: " << e.getSQLState() << " )" << endl;*/
                }
                
                
                list.push_back(transaction);
            }
        }
    }
    catch(sql::SQLException &e)
    {
        delete res;
        throw JSONRPCError(RPC_DATABASE_ERROR, "The daemon could not access to the database data");
    }

    delete res;
    return list;
}

WTransactionList WAddress::getUnspents(unsigned int start, unsigned int limit, unsigned int min_confirm)
{
    list<uint256> memPoolUseds;
    
    vector<uint256> vtxid;
    mempool.queryHashes(vtxid);

    BOOST_FOREACH(const uint256& hash, vtxid)
    {
        CTransaction tx = mempool.lookup(hash);
        
        //Foreach input => add prevout has to memPoolUseds
        BOOST_FOREACH(const CTxIn &txin, tx.vin)
        {
            if(txin.prevout.hash == 0)
                continue;
            
            CHashWriter hasher(0,0);
            hasher << txin.prevout.hash;
            hasher << txin.prevout.n;

            memPoolUseds.push_back(hasher.GetHash());
            //memPoolUseds.push_back(txin.prevout.hash);
        }
    }
    
    //----------------------------------------------------------------------
    
    WTransactionList list;
    while(list.size() < limit)
    {
        sql::ResultSet *res = queryTxStatement("spend='0' AND", start, limit, min_confirm);
        
        try
        {
            unsigned int resCount = 0;

            while(res->next())
            {
                resCount++;
                std::string txid = res->getString("txid");
                uint256 hash = WTransaction::touint(txid);

                WTransaction transaction;
                transaction.loadTxId(address, txid, true);
                
                CHashWriter hasher(0,0);
                hasher << transaction.txid;
                hasher << transaction.n;
                
                bool found = (std::find(memPoolUseds.begin(), memPoolUseds.end(), hasher.GetHash()) != memPoolUseds.end());

                if(!found)
                {
                    //WTransaction transaction;
                    //transaction.loadTxId(address, txid, true);
                    list.push_back(transaction);
                }
            }
            
            if(resCount < limit)
                break;
            
            start += limit;
        }
        catch(...)
        {
            delete res;
            throw JSONRPCError(RPC_DATABASE_ERROR, "The daemon could not access to the database data");
        }
        
        delete res;
    }
    
    return list;
}

//----------------------------------------------------------------

bool CTxOutContainsAddress(const CTxOut &txout, std::string address)
{
    txnouttype type;
    std::vector<CTxDestination> addresses;
    int nRequired;
    
    if (!ExtractDestinations(txout.scriptPubKey, type, addresses, nRequired))
    {
        return false;
    }
    
    BOOST_FOREACH(const CTxDestination& addr, addresses)
    {
        if(CBitcoinAddress(addr).ToString() == address)
        {
            return true;
        }
    }
    
    return false;
}

void WTransaction::loadTxId(std::string address, std::string strtxid, bool onlyOut)
{
    txid = WTransaction::touint(strtxid);
    
    //Check if from mempool..?
    
    CTransaction tx;
    uint256 hashBlock = 0;
    if (GetTransaction(txid, tx, hashBlock))
    {
        blockhash = hashBlock;

        CBlockIndex* index = mapBlockIndex[blockhash];
        blockheight = index->nHeight;
        blocktime   = index->nTime;

        loadTransaction(address, tx, onlyOut);
    }
}

void WTransaction::loadTransaction(std::string address, CTransaction tx, bool onlyOut)
{
    this->amount = 0;
    this->is_vout = false;
    
    unsigned int n = 0;
    
    if(!onlyOut)
    BOOST_FOREACH(const CTxIn &txin, tx.vin)
    {
        if(txin.prevout.hash.ToString() == "0000000000000000000000000000000000000000000000000000000000000000")
        {
            continue;
        }
        
        CTransaction txout;
        uint256 hashBlock = 0;
        if (GetTransaction(txin.prevout.hash, txout, hashBlock))
        {
            if(txout.vout.size() <= txin.prevout.n)
                continue;
            
            const CTxOut out = txout.vout[txin.prevout.n];
            
            if(CTxOutContainsAddress(out, address))
            {
                this->amount -= out.nValue;
                this->n       = n;
            }
        }
        
        n++;
    }
    
    //Vout always win vs vin
    n = 0;
    BOOST_FOREACH(const CTxOut &txout, tx.vout)
    {
        if(CTxOutContainsAddress(txout, address))
        {
            this->amount += txout.nValue;
            this->is_vout = true;
            this->n       = n;
        }

        n++;
    }
}

uint256 WTransaction::touint(std::string strhash)
{
    uint256 hash;
    hash.SetHex(strhash);
    return hash;
}


//----------------------------------------------------------------

Value WTransactionListValue(WTransactionList list)
{
    Array result;

    BOOST_FOREACH(const WTransactionList::value_type &v, list)
    {
        result.push_back(v.toObject());
    }

    return result;
}

Value publickey_transactions(const Array& params, bool fHelp)
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
    
    WAddress address = mysqlWallet.getAddress(params[0].get_str());
    WTransactionList list = address.getTransactions(start, count, min_confirm);
    return WTransactionListValue(list);
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

    WAddress address = mysqlWallet.getAddress(params[0].get_str());
    WTransactionList list = address.getUnspents(start, count, min_confirm);
    return WTransactionListValue(list);
}

Value publickey_balance(const Array& params, bool fHelp)
{
    if (fHelp || params.size() == 0 || params.size() > 2)
        throw runtime_error(
                            "publickey_balance (\"address\")\n"
                            "\nArguments:\n"
                            "1. \"address\"    (string, required) The public key that will be watched.\n"
                            "2. \"minimum confirmations\"    (numeric, optional) The minimum required confirmations dafault 0.\n");
    
    int min_confirm = params.size() > 1 ? params[1].get_int() : 0;
    CAmount balance = mysqlWallet.getAddress(params[0].get_str()).getBalance(min_confirm);
    
    std::ostringstream oss;
    oss << balance;
    return Value(oss.str());
}


Value publickeys_cache(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
                            "publickeys_cache\n"
                            "\n....\n");
    
    Object result;
    result.push_back(Pair("KeysCount", (int) mysqlWallet.getAddresses().size()));
    //result.push_back(Pair("CacheSize", (int) uni_wallet.GetMemoryUsage()));
    
    Array keys;
    BOOST_FOREACH(const WAddressMap::value_type &v, mysqlWallet.getAddresses())
    {
        keys.push_back(v.first);
    }
    
    result.push_back(Pair("Keys", keys));
    return result;
}

Value keydbconnected(const Array& params, bool fHelp)
{
    return true;
    //return mysqlWallet.isConnected();
}

Value validateaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
                            "validateaddress <beonpushaddress>\n"
                            "Return information about <beonpushaddress>.");
    
    CBitcoinAddress address(params[0].get_str());
    bool isValid = address.IsValid();
    
    Object ret;
    ret.push_back(Pair("isvalid", isValid));
    if (isValid)
    {
        CTxDestination dest = address.Get();
        string currentAddress = address.ToString();
        ret.push_back(Pair("address", currentAddress));
    }

    return ret;
}
