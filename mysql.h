#ifndef FILE_VAULT_MYSQL_H
#define FILE_VAULT_MYSQL_H

#include <mysql/mysql.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define user "root"
#define passwd "123456"
#define db "filevault"

// use "static" to hide from other source files
static MYSQL *mysql;

//Connect to MySQL, returns 0 upon success, returns -1 upon failure.
int connect_to_mysql();

//Close connection with MySQL.
void close_connection();

//Check if database has this table.
bool has_table(const char *table_name);

//Create table for a user, returns true upon success, returns false upon failure.
bool create_user_table(uid_t uid);

//execute MySQL create/update/delete query, returns true upon success, returns false upon failure.
bool execute_cud(const char *query);

#endif //FILE_VAULT_MYSQL_H