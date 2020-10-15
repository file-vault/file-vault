#ifndef FILE_VAULT_MYSQL_H
#define FILE_VAULT_MYSQL_H

#include <mysql/mysql.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define user "test"
#define passwd "123456"
#define db "filevault"

MYSQL *mysql;

//Connect to MySQL, returns 0 upon success, returns -1 upon failure.
int connect_to_mysql();

//Close connection with MySQL.
void close_connection();

//Check if database has this table.
bool has_table(char *table_name);

//Create table.
int create_table(char *table_name);

#endif //FILE_VAULT_MYSQL_H