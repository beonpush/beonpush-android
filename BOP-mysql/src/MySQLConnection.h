//
//  MySQLConnection.h
//  beonpushd
//
//  Created by Sacha Vandamme on 20/02/2017.
//  Copyright © 2017 Sacha Vandamme. All rights reserved.
//

#ifndef MySQLConnection_h
#define MySQLConnection_h


#include "ConnectionPool.h"
#include <string>
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/statement.h>

using boost::shared_ptr;

namespace active911 {
    
    
    class MySQLConnection : public Connection {
        
    public:
        
        
        ~MySQLConnection() {
            
            if(this->sql_connection) {
                
                _DEBUG("MYSQL Destruct");
                
                this->sql_connection->close();
                this->sql_connection.reset(); 	// Release and destruct
                
            }
            
        };
        
        boost::shared_ptr<sql::Connection> sql_connection;
        int a;
    };
    
    
    class MySQLConnectionFactory : public ConnectionFactory {
        
    public:
        MySQLConnectionFactory(std::string server, std::string username, std::string password, std::string dbname) {
            
            this->server=server;
            this->username=username;
            this->password=password;
            this->dbname=dbname;
        };
        
        // Any exceptions thrown here should be caught elsewhere
        boost::shared_ptr<Connection> create() {
            
            // Get the driver
            sql::Driver *driver;
            driver=get_driver_instance();
            
            // Create the connection
            boost::shared_ptr<MySQLConnection>conn(new MySQLConnection());
            
            // Connect
            conn->sql_connection=boost::shared_ptr<sql::Connection>(driver->connect(this->server,this->username,this->password));
            
            conn->sql_connection->setSchema(dbname);
            
            return boost::static_pointer_cast<Connection>(conn);
        };
        
    private:
        string server;
        string username;
        string password;
        string dbname;
    };
    
    
    
    
    
}

#endif /* MySQLConnection_h */
