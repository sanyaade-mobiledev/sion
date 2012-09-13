#!/bin/sh

if [ $# -lt 2 ] 
then 
    echo "Usage: CreateDatabase <admin_user> <admin_pwd>"
    exit 1 
else
    /usr/bin/mysql --user=$1 --password=$2 --host='localhost' --execute='CREATE DATABASE SIONDatabase;'
    /usr/bin/mysql --user=$1 --password=$2 --host='localhost' --execute="GRANT ALL ON SIONDatabase.* TO SION@localhost IDENTIFIED BY 'SION';"
fi

