#!/bin/bash

rm -rf /var/beonpush-data/.lock
rm -rf /var/beonpush-data/*.pid

/var/beonpush-src/src/beonpushd --datadir=/var/beonpush-data \
--rpcuser=$RPCUSER \
--rpcpassword=$RPCPASSWORD \
--dbhost=$DBHOST \
--dbuser=$DBUSER \
--dbpass=$DBPASS \
--dbname=$DBNAME
